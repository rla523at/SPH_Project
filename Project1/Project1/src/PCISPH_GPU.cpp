#include "PCISPH_GPU.h"

#include "Debugger.h"
#include "Device_Manager.h"
#include "Kernel.h"
#include "Neighborhood_Uniform_Grid.h"
#include "Neighborhood_Uniform_Grid_GPU.h"
#include "Utility.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <omp.h>

#undef max

#ifdef PCISPH_GPU_PERFORMANCE_ANALYSIS
#define PERFORMANCE_ANALYSIS_START Utility::set_time_point()
#define PERFORMANCE_ANALYSIS_END(func_name) _dt_sum_##func_name += Utility::cal_dt()
#else
#define PERFORMANCE_ANALYSIS_START
#define PERFORMANCE_ANALYSIS_END(sum_dt)
#endif

namespace ms
{
struct Cubic_Spline_Kernel_CB_Data
{
  float h           = 0.0f;
  float coefficient = 0.0f;
  float padding[2];
};

struct Cal_Number_Density_CS_CB_Data
{
  UINT  estimated_num_nfp  = 0;
  UINT  num_fluid_particle = 0;
  float padding[2];
};

struct Cal_Scailing_Factor_CS_CB_Data
{
  UINT  estimated_num_nfp = 0;
  float beta              = 0.0f;
  float padding[2];
};

struct Init_Fluid_Acceleration_CS_CB_Data
{
  Vector3 v_a_gravity;
  float   m0                  = 0.0f;
  float   viscosity_constant  = 0.0f;
  float   regularization_term = 0.0f;
  UINT    num_fluid_particle  = 0;
  UINT    estimated_num_nfp   = 0;
};

struct Init_Pressure_and_a_pressure_CS_CB_Data
{
  UINT  num_fluid_particle = 0;
  float padding[3];
};

struct Predict_Vel_and_Pos_CS_CB_Data
{
  UINT  num_fp = 0;
  float dt     = 0.0f;
  float padding[2];
};

struct Predict_Density_Error_and_Update_Pressure_CS_CB_Data
{
  float rho0              = 0.0f;
  float m0                = 0.0f;
  UINT  num_fp            = 0;
  UINT  estimated_num_nfp = 0;
};

struct Update_a_pressure_CS_CB_Data
{
  float m0                = 0.0f;
  UINT  num_fp            = 0;
  UINT  estimated_num_nfp = 0;
  float padding           = 0.0f;
};

struct Apply_BC_CS_CB_Data
{
  float cor          = 0.0f;
  float wall_x_start = 0.0f;
  float wall_x_end   = 0.0f;
  float wall_y_start = 0.0f;
  float wall_y_end   = 0.0f;
  float wall_z_start = 0.0f;
  float wall_z_end   = 0.0f;
  UINT  num_fp       = 0;
};

} // namespace ms

namespace ms
{
PCISPH_GPU::PCISPH_GPU(
  const Initial_Condition_Cubes& initial_condition,
  const Domain&                  solution_domain,
  Device_Manager&          device_manager)
{
  constexpr float dt                        = 1.0e-2f;
  constexpr float viscosity                 = 1.0e-2f;
  constexpr UINT  min_iter                  = 1;
  constexpr UINT  max_iter                  = 3;
  constexpr float smooth_length_param       = 1.2f;
  constexpr float allow_density_error_param = 4.0e-2f;
  constexpr float rho0                      = 1000.0f; //rest density
  constexpr float pi                        = std::numbers::pi_v<float>;

  ms::Utility::init_for_utility_using_GPU(device_manager);

  _dt                  = dt;
  _allow_density_error = rho0 * allow_density_error_param;
  _particle_radius     = initial_condition.particle_spacing;
  _min_iter            = min_iter;
  _max_iter            = max_iter;
  _DM_ptr              = &device_manager;

  // fluid initial state
  const auto& IC           = initial_condition;
  const auto  init_pos     = IC.cal_initial_position();
  const auto  init_density = std::vector<float>(init_pos.size(), rho0);

  _num_FP = static_cast<UINT>(init_pos.size());

  _fluid_v_pos_RWBS          = _DM_ptr->create_STRB_RWBS(_num_FP, init_pos.data());
  _fluid_v_cur_pos_RWBS      = _DM_ptr->create_STRB_RWBS(_num_FP, init_pos.data());
  _fluid_v_vel_RWBS          = _DM_ptr->create_STRB_RWBS<Vector3>(_num_FP);
  _fluid_v_cur_vel_RWBS      = _DM_ptr->create_STRB_RWBS<Vector3>(_num_FP);
  _fluid_v_accel_RWBS        = _DM_ptr->create_STRB_RWBS<Vector3>(_num_FP);
  _fluid_v_a_pressure_RWBS   = _DM_ptr->create_STRB_RWBS<Vector3>(_num_FP);
  _fluid_density_RWBS        = _DM_ptr->create_STRB_RWBS(_num_FP, init_density.data());
  _fluid_pressure_RWBS       = _DM_ptr->create_STRB_RWBS<float>(_num_FP);
  _fluid_density_error_RWBS  = _DM_ptr->create_STRB_RWBS<float>(_num_FP);
  _fluid_number_density_RWBS = _DM_ptr->create_STRB_RWBS<float>(_num_FP);
  _scailing_factor_RWBS      = _DM_ptr->create_STRB_RWBS<float>(1);

  const float h = smooth_length_param * IC.particle_spacing; //smoothing length

  // kernel
  {
    Cubic_Spline_Kernel_CB_Data CB_data = {};

    CB_data.h           = h;
    CB_data.coefficient = 3.0f / (2.0f * pi * h * h * h);

    _cptr_cubic_spline_kerenel_CONB = _DM_ptr->create_ICONB(&CB_data);
  }

  //Neighborhoood Search - Uniform Grid
  const float support_radius = 2.0f * h; // cubic spline kernel
  const float divide_length  = support_radius;

  _uptr_neighborhood = std::make_unique<Neighborhood_Uniform_Grid_GPU>(
    solution_domain,
    divide_length,
    _fluid_v_pos_RWBS,
    _num_FP,
    device_manager);

  // update number density
  _cptr_update_number_density_CS = _DM_ptr->create_CS(L"hlsl/update_number_density_CS.hlsl");
  {
    Cal_Number_Density_CS_CB_Data CB_data;
    CB_data.estimated_num_nfp           = g_estimated_num_nfp;
    CB_data.num_fluid_particle          = _num_FP;
    _cptr_update_number_density_CS_CONB = _DM_ptr->create_ICONB(&CB_data);
  }

  // init mass (after kernel, after number density)
  const auto m0 = this->cal_mass(rho0);

  // update scailing factor
  _cptr_update_scailing_factor_CS = _DM_ptr->create_CS(L"hlsl/update_scailing_factor_CS.hlsl");
  {

    Cal_Scailing_Factor_CS_CB_Data CB_data;
    CB_data.beta              = _dt * _dt * m0 * m0 * 2 / (rho0 * rho0);
    CB_data.estimated_num_nfp = g_estimated_num_nfp;

    _cptr_cal_scailing_factor_CS_CONB = _DM_ptr->create_ICONB(&CB_data);
  }

  // init fluid acceleration
  _cptr_init_fluid_acceleration_CS = _DM_ptr->create_CS(L"hlsl/init_fluid_acceleration_CS.hlsl");
  {
    Init_Fluid_Acceleration_CS_CB_Data CB_data = {};
    CB_data.v_a_gravity                        = Vector3(0.0f, -9.8f, 0.0f);
    CB_data.m0                                 = m0;
    CB_data.viscosity_constant                 = 10.0f * m0 * viscosity;
    CB_data.regularization_term                = 0.01f * h * h;
    CB_data.num_fluid_particle                 = _num_FP;
    CB_data.estimated_num_nfp                  = g_estimated_num_nfp;

    _cptr_init_fluid_acceleration_CS_CONB = _DM_ptr->create_ICONB(&CB_data);
  }

  // init pressure and a_pressure
  _cptr_init_pressure_and_a_pressure_CS = _DM_ptr->create_CS(L"hlsl/init_pressure_and_a_pressure.hlsl");
  {
    Init_Pressure_and_a_pressure_CS_CB_Data CB_data = {};
    CB_data.num_fluid_particle                      = _num_FP;

    _cptr_init_pressure_and_a_pressure_CS_CONB = _DM_ptr->create_ICONB(&CB_data);
  }

  // predict vel and pos
  _cptr_predict_vel_and_pos_CS = _DM_ptr->create_CS(L"hlsl/predict_vel_and_pos_CS.hlsl");
  {
    Predict_Vel_and_Pos_CS_CB_Data CB_data = {};
    CB_data.num_fp                         = _num_FP;
    CB_data.dt                             = _dt;

    _cptr_predict_vel_and_pos_CS_CONB = _DM_ptr->create_ICONB(&CB_data);
  }

  // predict density error and update pressure
  _cptr_predict_density_error_and_update_pressure_CS = _DM_ptr->create_CS(L"hlsl/predict_density_error_and_update_pressure_CS.hlsl");
  {
    Predict_Density_Error_and_Update_Pressure_CS_CB_Data data = {};

    data.rho0              = rho0;
    data.m0                = m0;
    data.num_fp            = _num_FP;
    data.estimated_num_nfp = g_estimated_num_nfp;

    _cptr_predict_density_error_and_update_pressure_CS_CONB = _DM_ptr->create_ICONB(&data);
  }

  // update a_pressure
  _cptr_update_a_pressure_CS = _DM_ptr->create_CS(L"hlsl/update_a_pressure_CS.hlsl");
  {
    Update_a_pressure_CS_CB_Data data = {};

    data.m0                = m0;
    data.num_fp            = _num_FP;
    data.estimated_num_nfp = g_estimated_num_nfp;

    _cptr_update_a_pressure_CS_CONB = _DM_ptr->create_ICONB(&data);
  }

  // aply BC
  _cptr_apply_BC_CS = _DM_ptr->create_CS(L"hlsl/apply_BC_CS.hlsl");
  {
    Apply_BC_CS_CB_Data data = {};

    data.cor          = 0.5f; // Coefficient Of Restitution
    data.wall_x_start = solution_domain.x_start + _particle_radius;
    data.wall_x_end   = solution_domain.x_end - _particle_radius;
    data.wall_y_start = solution_domain.y_start + _particle_radius;
    data.wall_y_end   = solution_domain.y_end - _particle_radius;
    data.wall_z_start = solution_domain.z_start + _particle_radius;
    data.wall_z_end   = solution_domain.z_end - _particle_radius;
    data.num_fp       = _num_FP;

    _cptr_apply_BC_CS_CONB = _DM_ptr->create_ICONB(&data);
  }

  //opt
  _cptr_fluid_density_error_intermediate_buffer = _DM_ptr->create_STRB<float>(_num_FP);
  _cptr_max_density_error_STGB                  = _DM_ptr->create_STGB_read(sizeof(float));
  //opt
}

PCISPH_GPU::~PCISPH_GPU() = default;

void PCISPH_GPU::update(void)
{
  PERFORMANCE_ANALYSIS_START;

  this->update_neighborhood();
  this->update_scailing_factor();
  this->init_fluid_acceleration();
  this->init_pressure_and_a_pressure();
  this->copy_cur_pos_and_vel();

  for (UINT i = 0; i < _max_iter; ++i)
  {
    this->predict_velocity_and_position();
    this->predict_density_error_and_update_pressure();
    this->update_a_pressure();
  }

  this->predict_velocity_and_position();
  this->apply_BC();

  _time += _dt;

  PERFORMANCE_ANALYSIS_END(update);

  //////////////////////////////////////////////////////////////////////////
  if (_time > 4.0)
  {
    Neighborhood_Uniform_Grid_GPU::print_performance_analysis_result();
    print_performance_analysis_result();
    exit(523);
  }
  //////////////////////////////////////////////////////////////////////////
}

float PCISPH_GPU::particle_radius(void) const
{
  return _particle_radius;
}

size_t PCISPH_GPU::num_fluid_particle(void) const
{
  return _num_FP;
}

const Read_Write_Buffer_Set& PCISPH_GPU::get_fluid_v_pos_BS(void) const
{
  return _fluid_v_pos_RWBS;
}

const Read_Write_Buffer_Set& PCISPH_GPU::get_fluid_density_BS(void) const
{
  return _fluid_density_RWBS;
}

void PCISPH_GPU::update_neighborhood(void)
{
  PERFORMANCE_ANALYSIS_START;

  _uptr_neighborhood->update(_fluid_v_pos_RWBS);

  PERFORMANCE_ANALYSIS_END(update_neighborhood);
}

void PCISPH_GPU::init_fluid_acceleration(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 2;
  constexpr UINT num_SRV    = 4;
  constexpr UINT num_UAV    = 1;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CONB.Get(),
    _cptr_init_fluid_acceleration_CS_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _fluid_density_RWBS.cptr_SRV.Get(),
    _fluid_v_vel_RWBS.cptr_SRV.Get(),
    _uptr_neighborhood->nfp_info_buffer_SRV_cptr().Get(),
    _uptr_neighborhood->nfp_count_buffer_SRV_cptr().Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_v_accel_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_init_fluid_acceleration_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(init_fluid_acceleration);
}

void PCISPH_GPU::init_pressure_and_a_pressure(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 1;
  constexpr UINT num_UAV    = 2;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_init_pressure_and_a_pressure_CS_CONB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_pressure_RWBS.cptr_UAV.Get(),
    _fluid_v_a_pressure_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_init_pressure_and_a_pressure_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(init_pressure_and_a_pressure);
}

void PCISPH_GPU::copy_cur_pos_and_vel(void)
{
  PERFORMANCE_ANALYSIS_START;

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CopyResource(_fluid_v_cur_pos_RWBS.cptr_buffer.Get(), _fluid_v_pos_RWBS.cptr_buffer.Get());
  cptr_context->CopyResource(_fluid_v_cur_vel_RWBS.cptr_buffer.Get(), _fluid_v_vel_RWBS.cptr_buffer.Get());

  PERFORMANCE_ANALYSIS_END(copy_cur_pos_and_vel);
}

void PCISPH_GPU::predict_velocity_and_position(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 1;
  constexpr UINT num_SRV    = 4;
  constexpr UINT num_UAV    = 2;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_predict_vel_and_pos_CS_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _fluid_v_cur_pos_RWBS.cptr_SRV.Get(),
    _fluid_v_cur_vel_RWBS.cptr_SRV.Get(),
    _fluid_v_accel_RWBS.cptr_SRV.Get(),
    _fluid_v_a_pressure_RWBS.cptr_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_v_pos_RWBS.cptr_UAV.Get(),
    _fluid_v_vel_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_predict_vel_and_pos_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(predict_velocity_and_position);
}

void PCISPH_GPU::predict_density_error_and_update_pressure(void)
{
  PERFORMANCE_ANALYSIS_START;

  const auto& ninfo_BS  = _uptr_neighborhood->get_ninfo_BS();
  const auto& ncount_BS = _uptr_neighborhood->get_ncount_BS();

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 2;
  constexpr UINT num_SRV    = 4;
  constexpr UINT num_UAV    = 3;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CONB.Get(),
    _cptr_predict_density_error_and_update_pressure_CS_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _fluid_v_pos_RWBS.cptr_SRV.Get(),
    _scailing_factor_RWBS.cptr_SRV.Get(),
    _uptr_neighborhood->nfp_info_buffer_SRV_cptr().Get(),
    _uptr_neighborhood->nfp_count_buffer_SRV_cptr().Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_density_RWBS.cptr_UAV.Get(),
    _fluid_pressure_RWBS.cptr_UAV.Get(),
    _fluid_density_error_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_predict_density_error_and_update_pressure_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(predict_density_error_and_update_pressure);
}

float PCISPH_GPU::cal_max_density_error(void)
{
  PERFORMANCE_ANALYSIS_START;

  const auto max_value_buffer  = ms::Utility::find_max_value_float_opt(_fluid_density_error_RWBS.cptr_buffer, _cptr_fluid_density_error_intermediate_buffer, _num_FP);
  const auto max_density_error = _DM_ptr->read_front<float>(max_value_buffer);

  PERFORMANCE_ANALYSIS_END(cal_max_density_error);

  return max_density_error;

  //_DM_ptr->copy_front(max_value_buffer, _cptr_max_density_error_STGB, sizeof(float));
  //return _DM_ptr->read_front_STGB<float>(_cptr_max_density_error_STGB);
}

void PCISPH_GPU::update_a_pressure(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_thread = 512;
  constexpr UINT num_CB     = 2;
  constexpr UINT num_SRV    = 5;
  constexpr UINT num_UAV    = 1;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CONB.Get(),
    _cptr_update_a_pressure_CS_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _fluid_density_RWBS.cptr_SRV.Get(),
    _fluid_pressure_RWBS.cptr_SRV.Get(),
    _fluid_v_pos_RWBS.cptr_SRV.Get(),
    _uptr_neighborhood->nfp_info_buffer_SRV_cptr().Get(),
    _uptr_neighborhood->nfp_count_buffer_SRV_cptr().Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_v_a_pressure_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_a_pressure_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(update_a_pressure);
}

void PCISPH_GPU::apply_BC(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 1;
  constexpr UINT num_UAV    = 2;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_apply_BC_CS_CONB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_v_pos_RWBS.cptr_UAV.Get(),
    _fluid_v_vel_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_apply_BC_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(apply_BC);
}

void PCISPH_GPU::update_scailing_factor(void)
{
  PERFORMANCE_ANALYSIS_START;

  this->update_number_density();
  const auto cptr_max_index_buffer     = Utility::find_max_index_float(_fluid_number_density_RWBS.cptr_buffer, _num_FP);
  const auto cptr_max_index_buffer_SRV = _DM_ptr->create_SRV(cptr_max_index_buffer);
  const auto cptr_nfp_info_buffer_SRV  = _uptr_neighborhood->nfp_info_buffer_SRV_cptr();
  const auto cptr_nfp_count_buffer_SRV = _uptr_neighborhood->nfp_count_buffer_SRV_cptr();

  constexpr UINT num_CB  = 2;
  constexpr UINT num_SRV = 3;
  constexpr UINT num_UAV = 1;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CONB.Get(),
    _cptr_cal_scailing_factor_CS_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    cptr_max_index_buffer_SRV.Get(),
    cptr_nfp_info_buffer_SRV.Get(),
    cptr_nfp_count_buffer_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _scailing_factor_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_scailing_factor_CS.Get(), nullptr, NULL);

  cptr_context->Dispatch(1, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(update_scailing_factor);
}

void PCISPH_GPU::update_number_density(void)
{
  const auto cptr_nfp_info_buffer_SRV  = _uptr_neighborhood->nfp_info_buffer_SRV_cptr();
  const auto cptr_nfp_count_buffer_SRV = _uptr_neighborhood->nfp_count_buffer_SRV_cptr();

  constexpr size_t num_CB     = 2;
  constexpr size_t num_SRV    = 2;
  constexpr size_t num_UAV    = 1;
  constexpr UINT   num_thread = 256;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CONB.Get(),
    _cptr_update_number_density_CS_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    cptr_nfp_info_buffer_SRV.Get(),
    cptr_nfp_count_buffer_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_number_density_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_number_density_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_FP, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();
}

float PCISPH_GPU::cal_mass(const float rho0)
{
  this->update_number_density();
  const auto max_value_buffer   = Utility::find_max_value_float(_fluid_number_density_RWBS.cptr_buffer, _num_FP);
  const auto max_number_density = _DM_ptr->read_front<float>(max_value_buffer);
  return rho0 / max_number_density;
}

void PCISPH_GPU::print_performance_analysis_result(void)
{
#ifdef PCISPH_GPU_PERFORMANCE_ANALYSIS
  std::cout << std::left;
  std::cout << "PCISPH_GPU Performance Analysis Result \n";
  std::cout << "======================================================================\n";
  std::cout << std::setw(60) << "_dt_sum_update" << std::setw(8) << _dt_sum_update << " ms\n";
  std::cout << "======================================================================\n";
  std::cout << std::setw(60) << "_dt_sum_update_neighborhood" << std::setw(8) << _dt_sum_update_neighborhood << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_update_scailing_factor" << std::setw(8) << _dt_sum_update_scailing_factor << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_init_fluid_acceleration" << std::setw(8) << _dt_sum_init_fluid_acceleration << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_init_pressure_and_a_pressure" << std::setw(8) << _dt_sum_init_pressure_and_a_pressure << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_copy_cur_pos_and_vel" << std::setw(8) << _dt_sum_copy_cur_pos_and_vel << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_predict_velocity_and_position" << std::setw(8) << _dt_sum_predict_velocity_and_position << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_predict_density_error_and_update_pressure" << std::setw(8) << _dt_sum_predict_density_error_and_update_pressure << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_cal_max_density_error" << std::setw(8) << _dt_sum_cal_max_density_error << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_update_a_pressure" << std::setw(8) << _dt_sum_update_a_pressure << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_apply_BC" << std::setw(8) << _dt_sum_apply_BC << " ms\n";
  std::cout << "======================================================================\n";
#endif
}

} // namespace ms