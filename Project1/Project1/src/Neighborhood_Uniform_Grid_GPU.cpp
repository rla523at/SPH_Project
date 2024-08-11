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

struct GCFP_ID
{
  UINT gc_index   = 0;
  UINT gcfp_index = 0;
};

struct Changed_GCFPT_ID_Data
{
  GCFP_ID prev_id;
  UINT    cur_gc_index = 0;
};

}

namespace ms
{

Neighborhood_Uniform_Grid_GPU::Neighborhood_Uniform_Grid_GPU(
  const Domain&          domain,
  const float            divide_length,
  const Read_Buffer_Set& fluid_v_pos_RBS,
  const UINT             num_fp,
  const Device_Manager&  device_manager)
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

  _cptr_find_changed_GCFP_ID_CS = device_manager.create_CS(L"hlsl/find_changed_GCFP_ID_CS.hlsl");

  _cptr_update_GCFP_CS    = device_manager.create_CS(L"hlsl/update_GCFP_CS.hlsl");
  _cptr_update_GCFP_CS_CB = device_manager.create_CONB(16);

  _cptr_rearrange_GCFP_CS = device_manager.create_CS(L"hlsl/rearrange_GCFP_CS.hlsl");
}

void Neighborhood_Uniform_Grid_GPU::init_ngc_index_buffer(void)
{
  // make inital data
  constexpr long long delta[3] = {-1, 0, 1};

  const auto num_cell          = _common_CB_data.num_cell;
  const auto num_x_cell        = _common_CB_data.num_x_cell;
  const auto num_y_cell        = _common_CB_data.num_y_cell;
  const auto num_z_cell        = _common_CB_data.num_z_cell;

  std::vector<UINT> ngc_indexes(num_cell * g_estimated_num_ngc, -1);
  std::vector<UINT> ngc_counts(num_cell);

  for (size_t i = 0; i < num_x_cell; ++i)
  {
    for (size_t j = 0; j < num_y_cell; ++j)
    {
      for (size_t k = 0; k < num_z_cell; ++k)
      {
        const auto gcid_vector = Grid_Cell_ID{i, j, k};
        const auto gc_index    = this->grid_cell_index(gcid_vector);

        UINT ngc_count = 0;

        UINT start_index = gc_index * g_estimated_num_ngc;

        for (size_t p = 0; p < 3; ++p)
        {
          for (size_t q = 0; q < 3; ++q)
          {
            for (size_t r = 0; r < 3; ++r)
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

  _info.ngc_index_RBS = _DM_ptr->create_ISTRB_RBS(num_cell * g_estimated_num_ngc, ngc_indexes.data());
  _info.ngc_count_RBS = _DM_ptr->create_ISTRB_RBS(num_cell, ngc_counts.data());
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
    id.gc_index   = gc_index;
    id.gcfp_index = GCFP_counts[gc_index];

    GCFP_indexes[gc_index * estimated_num_gcfp + id.gcfp_index] = i;
    GCFP_counts[gc_index]++;
    GCFP_IDs[i] = id;
  }

  _info.fp_index_RWBS   = _DM_ptr->create_STRB_RWBS(num_cell * estimated_num_gcfp, GCFP_indexes.data());
  _info.GCFP_count_RWBS  = _DM_ptr->create_STRB_RWBS(num_cell, GCFP_counts.data());
  _info.GCFP_ID_RWBS     = _DM_ptr->create_STRB_RWBS(num_particle, GCFP_IDs.data());
  _changed_GCFP_ID_ACBS = _DM_ptr->create_ACBS<Changed_GCFPT_ID_Data>(num_particle);
}

Grid_Cell_ID Neighborhood_Uniform_Grid_GPU::grid_cell_id(const Vector3& v_pos) const
{
  const float dx = v_pos.x - _common_CB_data.domain_x_start;
  const float dy = v_pos.y - _common_CB_data.domain_y_start;
  const float dz = v_pos.z - _common_CB_data.domain_z_start;

  const size_t i = static_cast<size_t>(dx / _common_CB_data.divide_length);
  const size_t j = static_cast<size_t>(dy / _common_CB_data.divide_length);
  const size_t k = static_cast<size_t>(dz / _common_CB_data.divide_length);

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

void Neighborhood_Uniform_Grid_GPU::update(const Read_Buffer_Set& fluid_v_pos_RBS)
{
  PERFORMANCE_ANALYSIS_START;

  this->find_changed_GCFPT_ID(fluid_v_pos_RBS);
  this->update_GCFP();
  this->rearrange_GCFP();

  PERFORMANCE_ANALYSIS_END(update);
}

const Neighborhood_Information Neighborhood_Uniform_Grid_GPU::get_neighborhood_info(void) const
{
  return _info;
}

void Neighborhood_Uniform_Grid_GPU::find_changed_GCFPT_ID(const Read_Buffer_Set& fluid_v_pos_RBS)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr size_t num_constant_buffer = 1;
  constexpr size_t num_SRV             = 2;
  constexpr size_t num_UAV             = 1;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    fluid_v_pos_RBS.cptr_SRV.Get(),
    _info.GCFP_ID_RWBS.cptr_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _changed_GCFP_ID_ACBS.cptr_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_find_changed_GCFP_ID_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_common_CB_data.num_particle / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();

  PERFORMANCE_ANALYSIS_END(find_changed_GCFPT_ID);
}

void Neighborhood_Uniform_Grid_GPU::update_GCFP(void)
{
  PERFORMANCE_ANALYSIS_START;

  constexpr UINT num_threads = 256;

  const auto cptr_context = _DM_ptr->context_cptr();

  cptr_context->CopyStructureCount(_cptr_update_GCFP_CS_CB.Get(), 0, _changed_GCFP_ID_ACBS.cptr_UAV.Get());

  const auto& arg_RWBS = ms::Utility::get_indirect_args_from_struct_count(_changed_GCFP_ID_ACBS.cptr_UAV, num_threads);

  constexpr size_t num_constant_buffer = 2;
  constexpr size_t num_UAV             = 4;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get(),
    _cptr_update_GCFP_CS_CB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _info.fp_index_RWBS.cptr_UAV.Get(),
    _info.GCFP_count_RWBS.cptr_UAV.Get(),
    _info.GCFP_ID_RWBS.cptr_UAV.Get(),
    _changed_GCFP_ID_ACBS.cptr_UAV.Get(),
  };

  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);

  cptr_context->CSSetShader(_cptr_update_GCFP_CS.Get(), nullptr, NULL);
  cptr_context->DispatchIndirect(arg_RWBS.cptr_buffer.Get(), NULL);

  _DM_ptr->CS_barrier();

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
    _info.fp_index_RWBS.cptr_UAV.Get(),
    _info.GCFP_count_RWBS.cptr_UAV.Get(),
    _info.GCFP_ID_RWBS.cptr_UAV.Get(),
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

void Neighborhood_Uniform_Grid_GPU::print_performance_analysis_result(void)
{
#ifdef UNIFORM_GRID_PERFORMANCE_ANALYSIS
  std::cout << std::left;
  std::cout << "Neighborhood_Uniform_Grid_GPU Performance Analysis Result \n";
  std::cout << "======================================================================\n";
  std::cout << std::setw(60) << "_dt_sum_update" << _dt_sum_update << " ms\n";
  std::cout << "======================================================================\n";
  std::cout << std::setw(60) << "_dt_sum_find_changed_GCFPT_ID" << std::setw(8) << _dt_sum_find_changed_GCFPT_ID << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_update_GCFP" << std::setw(8) << _dt_sum_update_GCFP << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_rearrange_GCFP" << std::setw(8) << _dt_sum_rearrange_GCFP << " ms\n";
  std::cout << std::setw(60) << "_dt_sum_update_nfp" << std::setw(8) << _dt_sum_update_nfp << " ms\n";
  std::cout << "======================================================================\n\n";
#endif
}

} // namespace ms