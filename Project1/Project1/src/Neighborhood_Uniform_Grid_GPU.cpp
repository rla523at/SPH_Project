#include "Neighborhood_Uniform_Grid_GPU.h"

#include "Debugger.h"
#include "Device_Manager.h"

#include <array>

namespace ms
{

Neighborhood_Uniform_Grid_GPU::Neighborhood_Uniform_Grid_GPU(
  const Domain&               domain,
  const float                 divide_length,
  const std::vector<Vector3>& fluid_particle_pos_vectors,
  const Device_Manager&       device_manager)
    : _DM_ptr(&device_manager)
{
  const float dx = domain.x_end - domain.x_start;
  const float dy = domain.y_end - domain.y_start;
  const float dz = domain.z_end - domain.z_start;

  _common_CB_data.num_x_cell         = static_cast<UINT>(std::ceil(dx / divide_length));
  _common_CB_data.num_y_cell         = static_cast<UINT>(std::ceil(dy / divide_length));
  _common_CB_data.num_z_cell         = static_cast<UINT>(std::ceil(dz / divide_length));
  _common_CB_data.num_cell           = _common_CB_data.num_x_cell * _common_CB_data.num_y_cell * _common_CB_data.num_z_cell;
  _common_CB_data.num_particle       = static_cast<UINT>(fluid_particle_pos_vectors.size());
  _common_CB_data.estimated_num_ngc  = g_estimated_num_ngc;
  _common_CB_data.estimated_num_gcfp = g_estimated_num_gcfp;
  _common_CB_data.estimated_num_nfp  = g_estimated_num_nfp;
  _common_CB_data.domain_x_start     = domain.x_start;
  _common_CB_data.domain_y_start     = domain.y_start;
  _common_CB_data.domain_z_start     = domain.z_start;
  _common_CB_data.divide_length      = divide_length;

  _cptr_common_CB = device_manager.create_CB_imuutable(&_common_CB_data);

  this->init_ngc_index_buffer();

  this->init_GCFP_buffer(fluid_particle_pos_vectors);
  _cptr_find_changed_GCFPT_ID_CS = device_manager.create_CS(L"hlsl/find_changed_GCFPT_ID_CS.hlsl");
  _cptr_update_GCFP_CS           = device_manager.create_CS(L"hlsl/update_GCFPT_CS.hlsl");
  _cptr_update_GCFP_CS_CB        = device_manager.create_CB(16);
  _cptr_rearrange_GCFP_CS        = device_manager.create_CS(L"hlsl/rearrange_GCFP_CS.hlsl");

  this->init_nfp();
  _cptr_update_nfp_CS = device_manager.create_CS(L"hlsl/update_nfp_CS.hlsl");

  // temporary code
  _cptr_fp_pos_buffer         = _DM_ptr->create_structured_buffer(_common_CB_data.num_particle, fluid_particle_pos_vectors.data());
  _cptr_fp_pos_buffer_SRV     = _DM_ptr->create_SRV(_cptr_fp_pos_buffer.Get());
  // temporary code

  this->update_nfp();

  // temporary
  _ninfoss.resize(_common_CB_data.num_particle);
  this->copy_to_ninfos();
  // temporary
}

void Neighborhood_Uniform_Grid_GPU::init_ngc_index_buffer(void)
{
  // make inital data
  constexpr long long delta[3]          = {-1, 0, 1};
  constexpr UINT      estimated_num_ngc = 27;

  const auto num_cell          = _common_CB_data.num_cell;
  const auto num_x_cell        = _common_CB_data.num_x_cell;
  const auto num_y_cell        = _common_CB_data.num_y_cell;
  const auto num_z_cell        = _common_CB_data.num_z_cell;
  const auto estiamted_num_ngc = _common_CB_data.estimated_num_ngc;

  const auto num_data = num_cell * estimated_num_ngc;

  std::vector<UINT> ngc_indexes(num_data, -1);
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

        UINT start_index = gc_index * estiamted_num_ngc;

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

  const auto& dm = *_DM_ptr;

  _cptr_ngc_index_buffer     = dm.create_structured_buffer_immutable(num_data, ngc_indexes.data());
  _cptr_ngc_index_buffer_SRV = dm.create_SRV(_cptr_ngc_index_buffer.Get());

  _cptr_ngc_count_buffer     = dm.create_structured_buffer(num_cell, ngc_counts.data());
  _cptr_ngc_count_buffer_SRV = dm.create_SRV(_cptr_ngc_count_buffer.Get());
}

void Neighborhood_Uniform_Grid_GPU::init_GCFP_buffer(const std::vector<Vector3>& fluid_particle_pos_vectors)
{
  // make initial data
  const auto num_cell           = _common_CB_data.num_cell;
  const auto num_particle       = _common_CB_data.num_particle;
  const auto estimated_num_gcfp = _common_CB_data.estimated_num_gcfp;

  std::vector<UINT>    GCFP_indexes(num_cell * estimated_num_gcfp, -1);
  std::vector<UINT>    GCFP_counts(num_cell);
  std::vector<GCFP_ID> GCFP_IDs(num_particle);

  for (UINT i = 0; i < num_particle; ++i)
  {
    const auto& v_pos    = fluid_particle_pos_vectors[i];
    const auto  gc_id    = this->grid_cell_id(v_pos);
    const auto  gc_index = this->grid_cell_index(gc_id);

    GCFP_ID id;
    id.gc_index   = gc_index;
    id.gcfp_index = GCFP_counts[gc_index];

    GCFP_indexes[gc_index * estimated_num_gcfp + id.gcfp_index] = i;
    GCFP_counts[gc_index]++;
    GCFP_IDs[i] = id;
  }

  const auto& dm                        = *_DM_ptr;
  const auto  estimated_num_update_data = _common_CB_data.num_particle;

  _cptr_fp_index_buffer     = dm.create_structured_buffer(num_cell * estimated_num_gcfp, GCFP_indexes.data());
  _cptr_fp_index_buffer_SRV = dm.create_SRV(_cptr_fp_index_buffer.Get());
  _cptr_fp_index_buffer_UAV = dm.create_UAV(_cptr_fp_index_buffer.Get());

  _cptr_GCFP_count_buffer     = dm.create_structured_buffer(num_cell, GCFP_counts.data());
  _cptr_GCFP_count_buffer_SRV = dm.create_SRV(_cptr_GCFP_count_buffer.Get());
  _cptr_GCFP_count_buffer_UAV = dm.create_UAV(_cptr_GCFP_count_buffer.Get());

  _cptr_GCFP_ID_buffer     = dm.create_structured_buffer(num_particle, GCFP_IDs.data());
  _cptr_GCFP_ID_buffer_SRV = dm.create_SRV(_cptr_GCFP_ID_buffer.Get());
  _cptr_GCFP_ID_buffer_UAV = dm.create_UAV(_cptr_GCFP_ID_buffer.Get());

  _cptr_changed_GCFP_ID_buffer  = dm.create_structured_buffer<Changed_GCFPT_ID_Data>(estimated_num_update_data);
  _cptr_changed_GCFPT_ID_AC_UAV = dm.create_AC_UAV(_cptr_changed_GCFP_ID_buffer.Get(), estimated_num_update_data);
}

void Neighborhood_Uniform_Grid_GPU::init_nfp(void)
{
  constexpr UINT estimated_neighbor = 200;
  const auto     num_data           = _common_CB_data.num_particle * estimated_neighbor;

  _cptr_nfp_info_buffer     = _DM_ptr->create_structured_buffer<Neighbor_Information>(num_data);
  _cptr_nfp_info_buffer_SRV = _DM_ptr->create_SRV(_cptr_nfp_info_buffer.Get());
  _cptr_nfp_info_buffer_UAV = _DM_ptr->create_UAV(_cptr_nfp_info_buffer.Get());

  _cptr_nfp_count_buffer     = _DM_ptr->create_structured_buffer<UINT>(_common_CB_data.num_particle);
  _cptr_nfp_count_buffer_SRV = _DM_ptr->create_SRV(_cptr_nfp_count_buffer.Get());
  _cptr_nfp_count_buffer_UAV = _DM_ptr->create_UAV(_cptr_nfp_count_buffer.Get());

  // temporary code
  _cptr_nfp_info_staging_buffer  = _DM_ptr->create_staging_buffer_read(_cptr_nfp_info_buffer);
  _cptr_nfp_count_staging_buffer = _DM_ptr->create_staging_buffer_read(_cptr_nfp_count_buffer);
  // temporary code
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

const Neighbor_Informations& Neighborhood_Uniform_Grid_GPU::search_for_fluid(const size_t pid) const
{
  return _ninfoss[pid];
}

ComPtr<ID3D11ShaderResourceView> Neighborhood_Uniform_Grid_GPU::nfp_info_buffer_SRV_cptr(void) const
{
  return _cptr_nfp_info_buffer_SRV;
}

ComPtr<ID3D11ShaderResourceView> Neighborhood_Uniform_Grid_GPU::nfp_count_buffer_SRV_cptr(void) const
{
  return _cptr_nfp_count_buffer_SRV;
}

void Neighborhood_Uniform_Grid_GPU::update(const ComPtr<ID3D11Buffer> _cptr_fluid_v_pos_buffer)
{
  // temporary code
  _cptr_fp_pos_buffer     = _cptr_fluid_v_pos_buffer;
  _cptr_fp_pos_buffer_SRV = _DM_ptr->create_SRV(_cptr_fp_pos_buffer);
  // temporary code

  this->find_changed_GCFPT_ID();
  this->update_GCFP_buffer();
  this->rearrange_GCFP();
  this->update_nfp();

  // temporary code
  this->copy_to_ninfos();
  // temporary code
}

void Neighborhood_Uniform_Grid_GPU::find_changed_GCFPT_ID(void)
{
  constexpr size_t num_constant_buffer = 1;
  constexpr size_t num_SRV             = 2;
  constexpr size_t num_UAV             = 1;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fp_pos_buffer_SRV.Get(),
    _cptr_GCFP_ID_buffer_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_changed_GCFPT_ID_AC_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_find_changed_GCFPT_ID_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_common_CB_data.num_particle / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::update_GCFP_buffer(void)
{
  const auto& device_manager = *_DM_ptr;
  const auto  cptr_context   = _DM_ptr->context_cptr();

  cptr_context->CopyStructureCount(_cptr_update_GCFP_CS_CB.Get(), 0, _cptr_changed_GCFPT_ID_AC_UAV.Get());

  constexpr size_t num_constant_buffer = 2;
  constexpr size_t num_UAV             = 4;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get(),
    _cptr_update_GCFP_CS_CB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_fp_index_buffer_UAV.Get(),
    _cptr_GCFP_count_buffer_UAV.Get(),
    _cptr_GCFP_ID_buffer_UAV.Get(),
    _cptr_changed_GCFPT_ID_AC_UAV.Get(),
  };

  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_GCFP_CS.Get(), nullptr, NULL);
  cptr_context->Dispatch(1, 1, 1);

  _DM_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::rearrange_GCFP(void)
{
  constexpr UINT num_constant_buffer = 1;
  constexpr UINT num_UAV             = 3;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_fp_index_buffer_UAV.Get(),
    _cptr_GCFP_count_buffer_UAV.Get(),
    _cptr_GCFP_ID_buffer_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_rearrange_GCFP_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_common_CB_data.num_cell / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::update_nfp(void)
{
  constexpr auto num_constant_buffer = 1;
  constexpr auto num_SRV             = 6;
  constexpr auto num_UAV             = 2;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_CB.Get()};

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fp_pos_buffer_SRV.Get(),
    _cptr_fp_index_buffer_SRV.Get(),
    _cptr_GCFP_count_buffer_SRV.Get(),
    _cptr_GCFP_ID_buffer_SRV.Get(),
    _cptr_ngc_index_buffer_SRV.Get(),
    _cptr_ngc_count_buffer_SRV.Get()};

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_nfp_info_buffer_UAV.Get(),
    _cptr_nfp_count_buffer_UAV.Get()};

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_nfp_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_common_CB_data.num_particle / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _DM_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::copy_to_ninfos(void)
{
  auto nfp_info  = _DM_ptr->read<Neighbor_Information>(_cptr_nfp_info_staging_buffer, _cptr_nfp_info_buffer);
  auto nfp_count = _DM_ptr->read<UINT>(_cptr_nfp_count_staging_buffer, _cptr_nfp_count_buffer);

  for (UINT i = 0; i < _common_CB_data.num_particle; ++i)
  {
    size_t index = i * _common_CB_data.estimated_num_nfp;

    auto& ninfos = _ninfoss[i];

    const UINT nn = nfp_count[i];

    ninfos.indexes.resize(nn);
    ninfos.translate_vectors.resize(nn);
    ninfos.distances.resize(nn);

    for (UINT j = 0; j < nn; ++j)
    {
      const auto& ninfo = nfp_info[index + j];

      ninfos.indexes[j]           = ninfo.fp_index;
      ninfos.translate_vectors[j] = ninfo.translate_vector;
      ninfos.distances[j]         = ninfo.distance;
    }
  }
}

} // namespace ms