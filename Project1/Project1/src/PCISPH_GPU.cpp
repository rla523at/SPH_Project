#include "PCISPH_GPU.h"

#include "Debugger.h"
#include "Device_Manager.h"
#include "Kernel.h"
#include "Neighborhood_Uniform_Grid.h"
#include "Neighborhood_Uniform_Grid_GPU.h"
#include "Utility.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <omp.h>

#undef max

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

} // namespace ms

namespace ms
{
PCISPH_GPU::PCISPH_GPU(
  const Initial_Condition_Cubes& initial_condition,
  const Domain&                  solution_domain,
  const Device_Manager&          device_manager)
    : _domain(solution_domain)
{
  constexpr float pi                  = std::numbers::pi_v<float>;
  constexpr float smooth_length_param = 1.2f;

  _rest_density        = 1000.0f;
  _allow_density_error = _rest_density * 4.0e-2f;
  _min_iter            = 3;
  _max_iter            = 5;

  const auto& ic = initial_condition;

  _particle_radius                  = ic.particle_spacing;
  _smoothing_length                 = smooth_length_param * ic.particle_spacing;
  _fluid_particles.position_vectors = ic.cal_initial_position();

  const auto num_fluid_particle = _fluid_particles.position_vectors.size();

  _fluid_particles.pressures.resize(num_fluid_particle);
  _fluid_particles.densities.resize(num_fluid_particle, _rest_density);
  _fluid_particles.velocity_vectors.resize(num_fluid_particle);
  _fluid_particles.acceleration_vectors.resize(num_fluid_particle);
  _pressure_acceleration_vectors.resize(num_fluid_particle);
  _current_position_vectors.resize(num_fluid_particle);
  _current_velocity_vectors.resize(num_fluid_particle);

  _uptr_kernel = std::make_unique<Cubic_Spline_Kernel>(_smoothing_length);

  const float divide_length = _uptr_kernel->supprot_radius();
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid_GPU>(
    solution_domain,
    divide_length,
    _fluid_particles.position_vectors,
    device_manager);

  this->init_mass_and_scailing_factor();

  _max_density_errors.resize(omp_get_max_threads());

  //////////////////////////////////////////////////////////////////////////
  ms::Utility::init_for_utility_using_GPU(device_manager);

  _dt        = 1.0e-2f;
  _rho0      = 1000;
  _viscosity = 1.0e-2f;

  _DM_ptr = &device_manager;

  _num_fluid_particle = static_cast<UINT>(num_fluid_particle);

  //fluid initial state
  _fluid_v_pos_BS = _DM_ptr->create_structured_buffer_set(_num_fluid_particle, _fluid_particles.position_vectors.data());

  _cptr_fluid_v_cur_pos_buffer     = _DM_ptr->create_structured_buffer(_num_fluid_particle, _fluid_particles.position_vectors.data());
  _cptr_fluid_v_cur_pos_buffer_SRV = _DM_ptr->create_SRV(_cptr_fluid_v_cur_pos_buffer);

  _fluid_v_vel_BS = _DM_ptr->create_structured_buffer_set(_num_fluid_particle, _fluid_particles.velocity_vectors.data());

  //_cptr_fluid_v_vel_buffer         = _DM_ptr->create_structured_buffer(_num_fluid_particle, _fluid_particles.velocity_vectors.data());
  //_cptr_fluid_v_vel_buffer_SRV     = _DM_ptr->create_SRV(_cptr_fluid_v_vel_buffer);
  _cptr_fluid_v_cur_vel_buffer     = _DM_ptr->create_structured_buffer(_num_fluid_particle, _fluid_particles.velocity_vectors.data());
  _cptr_fluid_v_cur_vel_buffer_SRV = _DM_ptr->create_SRV(_cptr_fluid_v_cur_vel_buffer);

  _cptr_fluid_v_accel_buffer     = _DM_ptr->create_structured_buffer(_num_fluid_particle, _fluid_particles.acceleration_vectors.data());
  _cptr_fluid_v_accel_buffer_SRV = _DM_ptr->create_SRV(_cptr_fluid_v_accel_buffer);
  _cptr_fluid_v_accel_buffer_UAV = _DM_ptr->create_UAV(_cptr_fluid_v_accel_buffer);

  _cptr_fluid_v_a_pressure_buffer     = _DM_ptr->create_structured_buffer(_num_fluid_particle, _pressure_acceleration_vectors.data());
  _cptr_fluid_v_a_pressure_buffer_SRV = _DM_ptr->create_SRV(_cptr_fluid_v_a_pressure_buffer);
  _cptr_fluid_v_a_pressure_buffer_UAV = _DM_ptr->create_UAV(_cptr_fluid_v_a_pressure_buffer);

  _cptr_fluid_density_buffer     = _DM_ptr->create_structured_buffer(_num_fluid_particle, _fluid_particles.densities.data());
  _cptr_fluid_density_buffer_SRV = _DM_ptr->create_SRV(_cptr_fluid_density_buffer);

  _cptr_fluid_pressure_buffer     = _DM_ptr->create_structured_buffer(_num_fluid_particle, _fluid_particles.pressures.data());
  _cptr_fluid_pressure_buffer_UAV = _DM_ptr->create_UAV(_cptr_fluid_pressure_buffer);

  // kernel
  _h = _smoothing_length;
  {
    Cubic_Spline_Kernel_CB_Data CB_data = {};
    CB_data.h                           = _h;
    CB_data.coefficient                 = 3.0f / (2.0f * pi * _h * _h * _h);

    _cptr_cubic_spline_kerenel_CB = _DM_ptr->create_CB_imuutable(&CB_data);
  }

  // number density
  _cptr_number_density_buffer     = _DM_ptr->create_structured_buffer<float>(_num_fluid_particle);
  _cptr_number_density_buffer_SRV = _DM_ptr->create_SRV(_cptr_number_density_buffer);
  _cptr_number_density_buffer_UAV = _DM_ptr->create_UAV(_cptr_number_density_buffer);

  _cptr_update_number_density_CS = _DM_ptr->create_CS(L"hlsl/cal_number_density_CS.hlsl");
  {
    Cal_Number_Density_CS_CB_Data CB_data;
    CB_data.estimated_num_nfp         = g_estimated_num_nfp;
    CB_data.num_fluid_particle        = _num_fluid_particle;
    _cptr_update_number_density_CS_CB = _DM_ptr->create_CB_imuutable(&CB_data);
  }

  // init mass (after kernel, after number density)
  _m0 = this->cal_mass();

  // scailing factor(after mass)
  _cptr_scailing_factor_buffer     = _DM_ptr->create_structured_buffer<float>(1);
  _cptr_scailing_factor_buffer_UAV = _DM_ptr->create_UAV(_cptr_scailing_factor_buffer);

  _cptr_cal_scailing_factor_CS = _DM_ptr->create_CS(L"hlsl/cal_scailing_factor_CS.hlsl");
  {
    REQUIRE(_m0 != 0, "mass should be initialized first");

    Cal_Scailing_Factor_CS_CB_Data CB_data;
    CB_data.beta              = _dt * _dt * _m0 * _m0 * 2 / (_rho0 * _rho0);
    CB_data.estimated_num_nfp = g_estimated_num_nfp;

    _cptr_cal_scailing_factor_CS_CB = _DM_ptr->create_CB_imuutable(&CB_data);
  }

  // init fluid acceleration
  _cptr_init_fluid_acceleration_CS = _DM_ptr->create_CS(L"hlsl/init_fluid_acceleration_CS.hlsl");
  {
    REQUIRE(_m0 != 0, "mass should be initialized first");

    Init_Fluid_Acceleration_CS_CB_Data CB_data = {};
    CB_data.v_a_gravity                        = Vector3(0.0f, -9.8f, 0.0f);
    CB_data.m0                                 = _m0;
    CB_data.viscosity_constant                 = 10.0f * _m0 * _viscosity;
    CB_data.regularization_term                = 0.01f * _h * _h;
    CB_data.num_fluid_particle                 = _num_fluid_particle;
    CB_data.estimated_num_nfp                  = g_estimated_num_nfp;

    _cptr_init_fluid_acceleration_CS_CB = _DM_ptr->create_CB_imuutable(&CB_data);
  }

  // init pressure and a_pressure
  _cptr_init_pressure_and_a_pressure_CS = _DM_ptr->create_CS(L"hlsl/init_pressure_and_a_pressure.hlsl");
  {
    Init_Pressure_and_a_pressure_CS_CB_Data CB_data = {};
    CB_data.num_fluid_particle                      = _num_fluid_particle;

    _cptr_init_pressure_and_a_pressure_CS_CB = _DM_ptr->create_CB_imuutable(&CB_data);
  }

  // predict vel and pos
  _cptr_predict_vel_and_pos_CS = _DM_ptr->create_CS(L"hlsl/predict_vel_and_pos_CS.hlsl");
  {
    Predict_Vel_and_Pos_CS_CB_Data CB_data = {};
    CB_data.num_fp                         = _num_fluid_particle;
    CB_data.dt                             = _dt;

    _cptr_predict_vel_and_pos_CS_CB = _DM_ptr->create_CB_imuutable(&CB_data);
  }
}

PCISPH_GPU::~PCISPH_GPU() = default;

void PCISPH_GPU::update(void)
{
  _DM_ptr->write(_fluid_particles.position_vectors.data(), _fluid_v_pos_BS.cptr_buffer); //temporary

  _uptr_neighborhood->update(_fluid_v_pos_BS.cptr_buffer);

  _scailing_factor = this->cal_scailing_factor();
  this->init_fluid_acceleration();
  this->init_pressure_and_a_pressure();

  float  density_error = _rest_density;
  size_t num_iter      = 0;

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CopyResource(_cptr_fluid_v_cur_pos_buffer.Get(), _fluid_v_pos_BS.cptr_buffer.Get());
  cptr_context->CopyResource(_cptr_fluid_v_cur_vel_buffer.Get(), _fluid_v_vel_BS.cptr_buffer.Get());

  //temporary
  _DM_ptr->read(_current_position_vectors.data(), _cptr_fluid_v_cur_pos_buffer);
  _DM_ptr->read(_current_velocity_vectors.data(), _cptr_fluid_v_cur_vel_buffer);
  //temporary

  //_current_position_vectors = _fluid_particles.position_vectors;
  //_current_velocity_vectors = _fluid_particles.velocity_vectors;

  while (_allow_density_error < density_error || num_iter < _min_iter)
  {
    this->predict_velocity_and_position();
    density_error = this->predict_density_and_update_pressure_and_cal_error();
    this->cal_pressure_acceleration();

    ++num_iter;

    if (_max_iter < num_iter)
      break;
  }

  // 마지막으로 update된 pressure에 의한 accleration을 반영
  this->predict_velocity_and_position();
  this->apply_boundary_condition();

  _time += _dt;
}

float PCISPH_GPU::particle_radius(void) const
{
  return _particle_radius;
}

size_t PCISPH_GPU::num_fluid_particle(void) const
{
  return _fluid_particles.num_particles();
}

const Vector3* PCISPH_GPU::fluid_particle_position_data(void) const
{
  return _fluid_particles.position_vectors.data();
}

const float* PCISPH_GPU::fluid_particle_density_data(void) const
{
  return _fluid_particles.densities.data();
}

void PCISPH_GPU::init_fluid_acceleration(void)
{
  //temporary
  _DM_ptr->write(_fluid_particles.densities.data(), _cptr_fluid_density_buffer);
  _DM_ptr->write(_fluid_particles.velocity_vectors.data(), _fluid_v_vel_BS.cptr_buffer);
  //temporary

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 2;
  constexpr UINT num_SRV    = 4;
  constexpr UINT num_UAV    = 1;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CB.Get(),
    _cptr_init_fluid_acceleration_CS_CB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fluid_density_buffer_SRV.Get(),
    _fluid_v_vel_BS.cptr_SRV.Get(),
    _uptr_neighborhood->nfp_info_buffer_SRV_cptr().Get(),
    _uptr_neighborhood->nfp_count_buffer_SRV_cptr().Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_fluid_v_accel_buffer_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_init_fluid_acceleration_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_fluid_particle, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  //temporary
  _DM_ptr->read(_fluid_particles.acceleration_vectors.data(), _cptr_fluid_v_accel_buffer);
  //temporary
}

void PCISPH_GPU::init_pressure_and_a_pressure(void)
{
  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 1;
  constexpr UINT num_UAV    = 2;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_init_pressure_and_a_pressure_CS_CB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_fluid_pressure_buffer_UAV.Get(),
    _cptr_fluid_v_a_pressure_buffer_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_init_pressure_and_a_pressure_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_fluid_particle, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  //temporary
  _DM_ptr->read(_fluid_particles.pressures.data(), _cptr_fluid_pressure_buffer);
  _DM_ptr->read(_pressure_acceleration_vectors.data(), _cptr_fluid_v_a_pressure_buffer);
  //temporary
}

void PCISPH_GPU::predict_velocity_and_position(void)
{
  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 1;
  constexpr UINT num_SRV    = 4;
  constexpr UINT num_UAV    = 2;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_predict_vel_and_pos_CS_CB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fluid_v_cur_pos_buffer_SRV.Get(),
    _cptr_fluid_v_cur_vel_buffer_SRV.Get(),
    _cptr_fluid_v_accel_buffer_SRV.Get(),
    _cptr_fluid_v_a_pressure_buffer_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fluid_v_pos_BS.cptr_UAV.Get(),
    _fluid_v_vel_BS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_predict_vel_and_pos_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_fluid_particle, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  //temporary
  _DM_ptr->read(_fluid_particles.position_vectors.data(), _fluid_v_pos_BS.cptr_buffer);
  _DM_ptr->read(_fluid_particles.velocity_vectors.data(), _fluid_v_vel_BS.cptr_buffer);
  //temporary

  //  const auto num_fluid_particles = _fluid_particles.num_particles();
  //
  //  auto&       position_vectors     = _fluid_particles.position_vectors;
  //  auto&       velocity_vectors     = _fluid_particles.velocity_vectors;
  //  const auto& acceleration_vectors = _fluid_particles.acceleration_vectors;
  //
  //#pragma omp parallel for
  //  for (int i = 0; i < num_fluid_particles; i++)
  //  {
  //    const auto& v_p_cur = _current_position_vectors[i];
  //    const auto& v_v_cur = _current_velocity_vectors[i];
  //
  //    auto&      v_p = position_vectors[i];
  //    auto&      v_v = velocity_vectors[i];
  //    const auto v_a = acceleration_vectors[i] + _pressure_acceleration_vectors[i];
  //
  //    v_v = v_v_cur + v_a * _dt;
  //    v_p = v_p_cur + v_v * _dt;
  //  }
  //
  //  // this->apply_boundary_condition();
  //  // 매번 boundary condition 적용해도 dt를 늘릴 수 없음
}

float PCISPH_GPU::predict_density_and_update_pressure_and_cal_error(void)
{
  const float h    = _smoothing_length;
  const float rho0 = _rest_density;
  const float m0   = _mass_per_particle;

  const auto  num_fluid_particle = _fluid_particles.num_particles();
  auto&       densities          = _fluid_particles.densities;
  auto&       pressures          = _fluid_particles.pressures;
  const auto& position_vectors   = _fluid_particles.position_vectors;

  std::fill(_max_density_errors.begin(), _max_density_errors.end(), -1.0f);

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    const auto thread_id = omp_get_thread_num();
    auto&      max_error = _max_density_errors[thread_id];

    auto&       rho  = densities[i];
    auto&       p    = pressures[i];
    const auto& v_xi = position_vectors[i];

    rho = 0.0;

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes      = neighbor_informations.indexes;

    const auto num_neighbor = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto& v_xj     = position_vectors[neighbor_indexes[j]];
      const auto  distance = (v_xi - v_xj).Length();

      rho += _uptr_kernel->W(distance);
    }

    rho *= m0;

    const float density_error = rho - rho0;

    p += _scailing_factor * density_error;
    p = std::max(p, 0.0f);

    // const float density_error = std::max(0.0f, rho - rho0);
    // p += _scailing_factor * density_error;

    max_error = (std::max)(max_error, density_error);
  }

  return *std::max_element(_max_density_errors.begin(), _max_density_errors.end());
}

void PCISPH_GPU::cal_pressure_acceleration(void)
{
  // viscosity constant
  const float m0 = _mass_per_particle;
  const float h  = _smoothing_length;

  // references
  const auto& densities        = _fluid_particles.densities;
  const auto& pressures        = _fluid_particles.pressures;
  const auto& position_vectors = _fluid_particles.position_vectors;

  const size_t num_fluid_particle = _fluid_particles.num_particles();

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    auto& v_a_pressure = _pressure_acceleration_vectors[i];
    v_a_pressure       = Vector3(0.0f, 0.0f, 0.0f);

    const float rhoi = densities[i];
    const float pi   = pressures[i];
    const auto& v_xi = position_vectors[i];

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes      = neighbor_informations.indexes;

    const auto num_neighbor = neighbor_indexes.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      if (i == neighbor_index)
        continue;

      const float rhoj = densities[neighbor_index];
      const float pj   = pressures[neighbor_index];
      const auto& v_xj = position_vectors[neighbor_index];

      const auto v_xij    = v_xi - v_xj;
      const auto distnace = v_xij.Length();

      // distance가 0이면 grad_q 계산시 0으로 나누어서 문제가 됨
      if (distnace == 0.0f)
        continue;

      // cal grad_kernel
      const auto v_grad_q      = 1.0f / (h * distnace) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distnace) * v_grad_q;

      // cal v_grad_pressure
      const auto coeff           = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
      const auto v_grad_pressure = coeff * v_grad_kernel;

      // update acceleration
      v_a_pressure += v_grad_pressure;
    }

    v_a_pressure *= -m0;
  }
}

void PCISPH_GPU::apply_boundary_condition(void)
{
  constexpr float cor  = 0.5f; // Coefficient Of Restitution
  constexpr float cor2 = 0.5f; // Coefficient Of Restitution

  const auto num_fluid_particles = _fluid_particles.num_particles();

  const float radius       = _smoothing_length;
  const float wall_x_start = _domain.x_start + radius;
  const float wall_x_end   = _domain.x_end - radius;
  const float wall_y_start = _domain.y_start + radius;
  const float wall_y_end   = _domain.y_end - radius;
  const float wall_z_start = _domain.z_start + radius;
  const float wall_z_end   = _domain.z_end - radius;

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particles; ++i)
  {
    auto& v_p = _fluid_particles.position_vectors[i];
    auto& v_v = _fluid_particles.velocity_vectors[i];

    if (v_p.x < wall_x_start && v_v.x < 0.0f)
    {
      v_v.x *= -cor2;
      v_p.x = wall_x_start;
    }

    if (v_p.x > wall_x_end && v_v.x > 0.0f)
    {
      v_v.x *= -cor2;
      v_p.x = wall_x_end;
    }

    if (v_p.y < wall_y_start && v_v.y < 0.0f)
    {
      v_v.y *= -cor;
      v_p.y = wall_y_start;
    }

    if (v_p.y > wall_y_end && v_v.y > 0.0f)
    {
      v_v.y *= -cor;
      v_p.y = wall_y_end;
    }

    if (v_p.z < wall_z_start && v_v.z < 0.0f)
    {
      v_v.z *= -cor2;
      v_p.z = wall_z_start;
    }

    if (v_p.z > wall_z_end && v_v.z > 0.0f)
    {
      v_v.z *= -cor2;
      v_p.z = wall_z_end;
    }
  }
}

void PCISPH_GPU::init_mass_and_scailing_factor(void)
{
  size_t max_index = 0;

  // init mass
  {
    const auto num_fluid_particle = _fluid_particles.num_particles();
    float      max_number_density = 0.0f;

    for (size_t i = 0; i < num_fluid_particle; i++)
    {
      float number_density = 0.0;

      const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
      const auto& neighbor_distances    = neighbor_informations.distances;
      const auto  num_neighbor          = neighbor_distances.size();

      for (size_t j = 0; j < num_neighbor; j++)
      {
        const float dist = neighbor_distances[j];
        number_density += _uptr_kernel->W(dist);
      }

      if (max_number_density < number_density)
      {
        max_number_density = number_density;
        max_index          = i;
      }
    }

    _mass_per_particle = _rest_density / max_number_density;
  }

  // init scailing factor
  {
    const float rho0 = _rest_density;
    const float m0   = _mass_per_particle;
    const float beta = _dt * _dt * m0 * m0 * 2 / (rho0 * rho0);
    const float h    = _smoothing_length;

    const auto& neighbor_informations      = _uptr_neighborhood->search_for_fluid(max_index);
    const auto& neighbor_translate_vectors = neighbor_informations.translate_vectors;
    const auto& neighbor_distances         = neighbor_informations.distances;
    const auto  num_neighbor               = neighbor_translate_vectors.size();

    Vector3 v_sum_grad_kernel = {0.0f, 0.0f, 0.0f};
    float   size_sum          = 0.0f;
    for (size_t i = 0; i < num_neighbor; ++i)
    {
      const float distance = neighbor_distances[i];

      // ignore my self
      if (distance == 0)
        continue;

      const auto& v_xij = neighbor_translate_vectors[i];

      const auto v_grad_q      = 1.0f / (h * distance) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distance) * v_grad_q;

      v_sum_grad_kernel += v_grad_kernel;
      size_sum += v_grad_kernel.Dot(v_grad_kernel);
    }

    const float sum_dot_sum = v_sum_grad_kernel.Dot(v_sum_grad_kernel);

    _scailing_factor = 1.0f / (beta * (sum_dot_sum + size_sum));
  }
}

float PCISPH_GPU::cal_scailing_factor(void)
{
  this->update_number_density();
  const auto cptr_max_index_buffer     = Utility::find_max_index_float(_cptr_number_density_buffer, _num_fluid_particle);
  const auto cptr_max_index_buffer_SRV = _DM_ptr->create_SRV(cptr_max_index_buffer);
  const auto cptr_nfp_info_buffer_SRV  = _uptr_neighborhood->nfp_info_buffer_SRV_cptr();
  const auto cptr_nfp_count_buffer_SRV = _uptr_neighborhood->nfp_count_buffer_SRV_cptr();

  constexpr UINT num_CB  = 2;
  constexpr UINT num_SRV = 3;
  constexpr UINT num_UAV = 1;

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_cubic_spline_kerenel_CB.Get(),
    _cptr_cal_scailing_factor_CS_CB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    cptr_max_index_buffer_SRV.Get(),
    cptr_nfp_info_buffer_SRV.Get(),
    cptr_nfp_count_buffer_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_scailing_factor_buffer_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_cal_scailing_factor_CS.Get(), nullptr, NULL);

  cptr_context->Dispatch(1, 1, 1);

  _DM_ptr->CS_barrier();

  return _DM_ptr->read_front<float>(_cptr_scailing_factor_buffer);
}

float PCISPH_GPU::cal_number_density(const size_t fluid_particle_id) const
{
  float number_density = 0.0;

  const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(fluid_particle_id);
  const auto& neighbor_distances    = neighbor_informations.distances;
  const auto  num_neighbor          = neighbor_distances.size();

  for (size_t j = 0; j < num_neighbor; j++)
  {
    const float dist = neighbor_distances[j];
    number_density += _uptr_kernel->W(dist);
  }

  return number_density;
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
    _cptr_cubic_spline_kerenel_CB.Get(),
    _cptr_update_number_density_CS_CB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    cptr_nfp_info_buffer_SRV.Get(),
    cptr_nfp_count_buffer_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_number_density_buffer_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_number_density_CS.Get(), nullptr, NULL);

  const auto num_group_x = ms::Utility::ceil(_num_fluid_particle, num_thread);
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();
}

float PCISPH_GPU::cal_mass(void)
{
  this->update_number_density();
  const auto max_value_buffer   = Utility::find_max_value_float(_cptr_number_density_buffer, _num_fluid_particle);
  const auto max_number_density = _DM_ptr->read_front<float>(max_value_buffer);
  return _rho0 / max_number_density;
}

} // namespace ms