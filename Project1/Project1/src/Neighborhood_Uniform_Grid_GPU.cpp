#include "Neighborhood_Uniform_Grid_GPU.h"

#include "Debugger.h"
#include "Device_Manager.h"
#include "Utility.h"

#include <array>
#include <iomanip>

#ifdef UNIFORM_GRID_PERFORMANCE_ANALYSIS
#define PERFORMANCE_ANALYSIS_START Utility::set_time_point()
#define PERFORMANCE_ANALYSIS_END(func_name) _dt_sum_##func_name += Utility::cal_dt()
#else
#define PERFORMANCE_ANALYSIS_START
#define PERFORMANCE_ANALYSIS_END(sum_dt)
#endif

namespace ms
{

Neighborhood_Uniform_Grid_GPU::Neighborhood_Uniform_Grid_GPU(
  const Domain&          domain,
  const float            divide_length,
  const Read_Buffer_Set& fluid_v_pos_RBS,
  const UINT             num_fp,
  Device_Manager&        device_manager)
    : _DM_ptr(&device_manager)
{
  const float dx = domain.x_end - domain.x_start;
  const float dy = domain.y_end - domain.y_start;
  const float dz = domain.z_end - domain.z_start;

  _common_CB_data.num_x_cell         = static_cast<UINT>(std::ceil(dx / divide_length));
  _common_CB_data.num_y_cell         = static_cast<UINT>(std::ceil(dy / divide_length));
  _common_CB_data.num_z_cell         = static_cast<UINT>(std::ceil(dz / divide_length));
  _common_CB_data.num_cell           = _common_CB_data.num_x_cell * _common_CB_data.num_y_cell * _common_CB_data.num_z_cell;
  _common_CB_data.num_particle       = num_fp;
  _common_CB_data.estimated_num_ngc  = g_estimated_num_ngc;
  _common_CB_data.estimated_num_gcfp = g_estimated_num_gcfp;
  _common_CB_data.estimated_num_nfp  = g_estimated_num_nfp;
  _common_CB_data.domain_x_start     = domain.x_start;
  _common_CB_data.domain_y_start     = domain.y_start;
  _common_CB_data.domain_z_start     = domain.z_start;
  _common_CB_data.divide_length      = divide_length;

  _cptr_common_CB = device_manager.create_ICONB(&_common_CB_data);

  this->init_ngc_index_buffer();
  this->init_GCFP_buffer(fluid_v_pos_RBS);

  _ninfo_RWBS  = _DM_ptr->create_STRB_RWBS<Neighbor_Information>(num_fp * g_estimated_num_nfp);
  _ncount_RWBS = _DM_ptr->create_STRB_RWBS<UINT>(num_fp);

  _cptr_update_GCFP_CS    = device_manager.create_CS(L"hlsl/update_GCFP_CS.hlsl");
  _cptr_update_GCFP_CS_CB = device_manager.create_CONB(16);

  _cptr_rearrange_GCFP_CS = device_manager.create_CS(L"hlsl/rearrange_GCFP_CS.hlsl");

  _cptr_update_nfp_CS = device_manager.create_CS(L"hlsl/update_nfp_CS.hlsl");

  this->update_nfp(fluid_v_pos_RBS);
}

void Neighborhood_Uniform_Grid_GPU::init_ngc_index_buffer(void)
{
  // make inital data
  constexpr int delta[3] = {-1, 0, 1};

  const auto num_cell          = _common_CB_data.num_cell;
  const auto num_x_cell        = _common_CB_data.num_x_cell;
  const auto num_y_cell        = _common_CB_data.num_y_cell;
  const auto num_z_cell        = _common_CB_data.num_z_cell;
  const auto estiamted_num_ngc = _common_CB_data.estimated_num_ngc;

  std::vector<UINT> ngc_indexes(num_cell * g_estimated_num_ngc, -1);
  std::vector<UINT> ngc_counts(num_cell);

  for (UINT i = 0; i < num_x_cell; ++i)
  {
    for (UINT j = 0; j < num_y_cell; ++j)
    {
      for (UINT k = 0; k < num_z_cell; ++k)
      {
        const auto gcid_vector = Grid_Cell_ID{i, j, k};
        const auto gc_index    = this->grid_cell_index(gcid_vector);

        UINT ngc_count = 0;

        UINT start_index = gc_index * estiamted_num_ngc;

        for (UINT p = 0; p < 3; ++p)
        {
          for (UINT q = 0; q < 3; ++q)
          {
            for (UINT r = 0; r < 3; ++r)
            {
              Grid_Cell_ID neighbor_gcid_vector;
              neighbor_gcid_vector.x = gcid_vector.x + delta[p];
              neighbor_gcid_vector.y = gcid_vector.y + delta[q];
              neighbor_gcid_vector.z = gcid_vector.z + delta[r];

              if (!this->is_valid_index(neighbor_gcid_vector))
                continue;

              ngc_indexes[start_index + ngc_count] = this->grid_cell_index(neighbor_gcid_vector);
              ++ngc_count;
            }
          }
        }

        ngc_counts[gc_index] = ngc_count;
      }
    }
  }

  _ngc_index_RBS = _DM_ptr->create_ISTRB_RBS(num_cell * g_estimated_num_ngc, ngc_indexes.data());
  _ngc_count_RBS = _DM_ptr->create_ISTRB_RBS(num_cell, ngc_counts.data());
}

void Neighborhood_Uniform_Grid_GPU::init_GCFP_buffer(const Read_Buffer_Set& fluid_v_pos_RBS)
{
  // temporary
  const auto fp_pos_vectors = _DM_ptr->read<Vector3>(fluid_v_pos_RBS.cptr_buffer);
  // temporary

  // make initial data
  const auto num_cell           = _common_CB_data.num_cell;
  const auto num_particle       = _common_CB_data.num_particle;
  const auto estimated_num_gcfp = _common_CB_data.estimated_num_gcfp;

  std::vector<UINT>    GCFP_indexes(num_cell * estimated_num_gcfp, -1);
  std::vector<UINT>    GCFP_counts(num_cell);
  std::vector<GCFP_ID> GCFP_IDs(num_particle);

  for (UINT i = 0; i < num_particle; ++i)
  {
    const auto& v_pos    = fp_pos_vectors[i];
    const auto  gc_id    = this->grid_cell_id(v_pos);
    const auto  gc_index = this->grid_cell_index(gc_id);

    GCFP_ID id;
    id.GCid       = gc_id;
    id.gcfp_index = GCFP_counts[gc_index];

    GCFP_indexes[gc_index * estimated_num_gcfp + id.gcfp_index] = i;
    GCFP_counts[gc_index]++;
    GCFP_IDs[i] = id;
  }

  _fp_index_RWBS   = _DM_ptr->create_STRB_RWBS(num_cell * estimated_num_gcfp, GCFP_indexes.data());
  _GCFP_count_RWBS = _DM_ptr->create_STRB_RWBS(num_cell, GCFP_counts.data());
  _GCFP_ID_RWBS    = _DM_ptr->create_STRB_RWBS(num_particle, GCFP_IDs.data());
}

Grid_Cell_ID Neighborhood_Uniform_Grid_GPU::grid_cell_id(const Vector3& v_pos) const
{
  const float dx = v_pos.x - _common_CB_data.domain_x_start;
  const float dy = v_pos.y - _common_CB_data.domain_y_start;
  const float dz = v_pos.z - _common_CB_data.domain_z_start;

  const UINT i = static_cast<UINT>(dx / _common_CB_data.divide_length);
  const UINT j = static_cast<UINT>(dy / _common_CB_data.divide_length);
  const UINT k = static_cast<UINT>(dz / _common_CB_data.divide_length);

  return {i, j, k};
}

UINT Neighborhood_Uniform_Grid_GPU::grid_cell_index(const Grid_Cell_ID& index_vector) const
{
  const auto num_x_cell = _common_CB_data.num_x_cell;
  const auto num_y_cell = _common_CB_data.num_y_cell;

  return static_cast<UINT>(index_vector.x + index_vector.y * num_x_cell + index_vector.z * num_x_cell * num_y_cell);
}

bool Neighborhood_Uniform_Grid_GPU::is_valid_index(const Grid_Cell_ID& index_vector) const
{
  const auto num_x_cell = _common_CB_data.num_x_cell;
  const auto num_y_cell = _common_CB_data.num_y_cell;
  const auto num_z_cell = _common_CB_data.num_z_cell;

  if (num_x_cell <= index_vector.x)
    return false;

  if (num_y_cell <= index_vector.y)
    return false;

  if (num_z_cell <= index_vector.z)
    return false;

  return true;
}

ComPtr<ID3D11ShaderResourceView> Neighborhood_Uniform_Grid_GPU::nfp_info_buffer_SRV_cptr(void) const
{
  return _ninfo_RWBS.cptr_SRV;
}

ComPtr<ID3D11ShaderResourceView> Neighborhood_Uniform_Grid_GPU::nfp_count_buffer_SRV_cptr(void) const
{
  return _ncount_RWBS.cptr_SRV;
}

void Neighborhood_Uniform_Grid_GPU::update(const Read_Buffer_Set& fluid_v_pos_RBS)
{
  PERFORMANCE_ANALYSIS_START;

  this->update_GCFP(fluid_v_pos_RBS);
  this->rearrange_GCFP();
  this->update_nfp(fluid_v_pos_RBS);

  PERFORMANCE_ANALYSIS_END(update);
}

const Read_Write_Buffer_Set& Neighborhood_Uniform_Grid_GPU::get_ninfo_BS(void) const
{
  return _ninfo_RWBS;
}

const Read_Write_Buffer_Set& Neighborhood_Uniform_Grid_GPU::get_ncount_BS(void) const
{
  return _ncount_RWBS;
}

void Neighborhood_Uniform_Grid_GPU::update_GCFP(const Read_Buffer_Set& fluid_v_pos_RBS)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_thread = 32;

  _DM_ptr->set_CONB(0, _cptr_common_CB);
  _DM_ptr->bind_CONBs_to_CS(0, 1);

  _DM_ptr->set_SRV(0, fluid_v_pos_RBS.cptr_SRV);
  _DM_ptr->bind_SRVs_to_CS(0, 1);

  _DM_ptr->set_UAV(0, _fp_index_RWBS.cptr_UAV);
  _DM_ptr->set_UAV(1, _GCFP_count_RWBS.cptr_UAV);
  _DM_ptr->set_UAV(2, _GCFP_ID_RWBS.cptr_UAV);
  _DM_ptr->bind_UAVs_to_CS(0, 3);

  _DM_ptr->set_shader_CS(_cptr_update_GCFP_CS);

  const auto num_group_x = Utility::ceil(_common_CB_data.num_particle, num_thread);
  _DM_ptr->dispatch(num_group_x, 1, 1);

  PERFORMANCE_ANALYSIS_END(update_GCFP);
}

void Neighborhood_Uniform_Grid_GPU::rearrange_GCFP(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_constant_buffer = 1;
  constexpr UINT num_UAV             = 3;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _fp_index_RWBS.cptr_UAV.Get(),
    _GCFP_count_RWBS.cptr_UAV.Get(),
    _GCFP_ID_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_rearrange_GCFP_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_common_CB_data.num_cell / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(rearrange_GCFP);
}

void Neighborhood_Uniform_Grid_GPU::update_nfp(const Read_Buffer_Set& fluid_v_pos_RBS)
{
  PERFORMANCE_ANALYSIS_START;
  constexpr UINT max_group = 65535;

  constexpr auto num_constant_buffer = 1;
  constexpr auto num_SRV             = 6;
  constexpr auto num_UAV             = 2;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get()};

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    fluid_v_pos_RBS.cptr_SRV.Get(),
    _fp_index_RWBS.cptr_SRV.Get(),
    _GCFP_count_RWBS.cptr_SRV.Get(),
    _GCFP_ID_RWBS.cptr_SRV.Get(),
    _ngc_index_RBS.cptr_SRV.Get(),
    _ngc_count_RBS.cptr_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _ninfo_RWBS.cptr_UAV.Get(),
    _ncount_RWBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_nfp_CS.Get(), nullptr, NULL);

  UINT num_group_x = _common_CB_data.num_particle;
  UINT num_group_y = 1;
  UINT num_group_z = 1;

  if (max_group < _common_CB_data.num_particle)
  {
    num_group_x = max_group;
    num_group_y = Utility::ceil(_common_CB_data.num_particle, max_group);
  }

  _DM_ptr->dispatch(num_group_x, num_group_y, num_group_z);

  PERFORMANCE_ANALYSIS_END(update_nfp);

  ////debug
  //const auto debug_GCFP_count = _DM_ptr->read<UINT>(_GCFP_count_RWBS.cptr_buffer);
  //print_max(debug_GCFP_count);
  ////debug
}

void Neighborhood_Uniform_Grid_GPU::print_performance_analysis_result(void)
{
#ifdef UNIFORM_GRID_PERFORMANCE_ANALYSIS
  std::cout << std::left;
  std::cout << "Neighborhood_Uniform_Grid_GPU Performance Analysis Result \n";
  std::cout << "======================================================================\n";
  std::cout << std::setw(60) << "_dt_sum_update" << _dt_sum_update << " ms\n";
  std::cout << "======================================================================\n";
  std::cout << std::setw(60) << "_dt_sum_update_GCFP" << std::setw(8) << _dt_sum_update_GCFP << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_rearrange_GCFP" << std::setw(8) << _dt_sum_rearrange_GCFP << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_update_nfp" << std::setw(8) << _dt_sum_update_nfp << " ms\n";
  std::cout << "======================================================================\n\n";
#endif
}

void Neighborhood_Uniform_Grid_GPU::print_avg_performance_analysis_result(const UINT num_frame)
{
#ifdef UNIFORM_GRID_PERFORMANCE_ANALYSIS
  std::cout << std::left;
  std::cout << "Neighborhood_Uniform_Grid_GPU Performance Analysis Result Per Frame \n";
  std::cout << "================================================================================\n";
  std::cout << std::setw(60) << "_dt_avg_update" << std::setw(13) << _dt_sum_update / num_frame << " ms\n";
  std::cout << "================================================================================\n";
  std::cout << std::setw(60) << "_dt_avg_update_GCFP" << std::setw(13) << _dt_sum_update_GCFP / num_frame << " ms\n";
  std::cout << std::setw(60) << "_dt_avg_rearrange_GCFP" << std::setw(13) << _dt_sum_rearrange_GCFP / num_frame << " ms\n";
  std::cout << std::setw(60) << "_dt_avg_update_nfp" << std::setw(13) << _dt_sum_update_nfp / num_frame << " ms\n";
  std::cout << "================================================================================\n\n";
#endif
}

} // namespace ms