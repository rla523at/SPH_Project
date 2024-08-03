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
  const std::vector<Vector3>& boundary_particle_pos_vectors,
  const Device_Manager&       device_manager)
    : _device_manager_ptr(&device_manager)
{
  const float dx = domain.x_end - domain.x_start;
  const float dy = domain.y_end - domain.y_start;
  const float dz = domain.z_end - domain.z_start;

  _constant_data.num_x_cell     = static_cast<UINT>(std::ceil(dx / divide_length));
  _constant_data.num_y_cell     = static_cast<UINT>(std::ceil(dy / divide_length));
  _constant_data.num_z_cell     = static_cast<UINT>(std::ceil(dz / divide_length));
  _constant_data.num_cell       = _constant_data.num_x_cell * _constant_data.num_y_cell * _constant_data.num_z_cell;
  _constant_data.num_particle   = static_cast<UINT>(fluid_particle_pos_vectors.size());
  _constant_data.domain_x_start = domain.x_start;
  _constant_data.domain_y_start = domain.y_start;
  _constant_data.domain_z_start = domain.z_start;
  _constant_data.divide_length  = divide_length;

  _cptr_common_constnat_buffer = device_manager.create_constant_buffer_imuutable(&_constant_data);

  this->init_ngc_index_buffer();

  this->init_GCFP_buffer(fluid_particle_pos_vectors);
  _cptr_find_changed_GCFPT_ID_CS       = device_manager.create_CS(L"hlsl/find_changed_GCFPT_ID_CS.hlsl");
  _cptr_update_GCFP_CS                 = device_manager.create_CS(L"hlsl/update_GCFPT_CS.hlsl");
  _cptr_update_GCFP_CS_constant_buffer = device_manager.create_constant_buffer(16);
  _cptr_rearrange_GCFP_CS              = device_manager.create_CS(L"hlsl/rearrange_GCFP_CS.hlsl");

  this->init_nfp();
  _cptr_update_nfp_CS = device_manager.create_CS(L"hlsl/update_nfp_CS.hlsl");

  // temporary code
  _cptr_fp_pos_buffer         = _device_manager_ptr->create_structured_buffer(_constant_data.num_particle, fluid_particle_pos_vectors.data());
  _cptr_fp_pos_buffer_SRV     = _device_manager_ptr->create_SRV(_cptr_fp_pos_buffer.Get());
  _cptr_fp_pos_staging_buffer = _device_manager_ptr->create_staging_buffer_write(_cptr_fp_pos_buffer);
  // temporary code

  this->update_nfp();

  // temporary
  _ninfoss.resize(_constant_data.num_particle);
  this->copy_to_ninfos();
  // temporary
}

void Neighborhood_Uniform_Grid_GPU::init_ngc_index_buffer(void)
{
  // make inital data
  constexpr long long delta[3]          = {-1, 0, 1};
  constexpr UINT      estimated_num_ngc = 27;

  const auto num_cell          = _constant_data.num_cell;
  const auto num_x_cell        = _constant_data.num_x_cell;
  const auto num_y_cell        = _constant_data.num_y_cell;
  const auto num_z_cell        = _constant_data.num_z_cell;
  const auto estiamted_num_ngc = _constant_data.estimated_num_ngc;

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

  const auto& device_manager = *_device_manager_ptr;

  // create ngc texture
  _cptr_ngc_index_buffer     = device_manager.create_structured_buffer_immutable(num_data, ngc_indexes.data());
  _cptr_ngc_index_buffer_SRV = device_manager.create_SRV(_cptr_ngc_index_buffer.Get());

  // create ngc count buffer
  _cptr_ngc_count_buffer     = device_manager.create_structured_buffer(num_cell, ngc_counts.data());
  _cptr_ngc_count_buffer_SRV = device_manager.create_SRV(_cptr_ngc_count_buffer.Get());
}

void Neighborhood_Uniform_Grid_GPU::init_GCFP_buffer(const std::vector<Vector3>& fluid_particle_pos_vectors)
{
  // make initial data
  const auto num_cell           = _constant_data.num_cell;
  const auto num_particle       = _constant_data.num_particle;
  const auto estimated_num_gcfp = _constant_data.estimated_num_gcfp;

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

  const auto& device_manager            = *_device_manager_ptr;
  const auto  estimated_num_update_data = _constant_data.num_particle;

  // create fp_index index buffer
  _cptr_fp_index_buffer     = device_manager.create_structured_buffer(num_cell * estimated_num_gcfp, GCFP_indexes.data());
  _cptr_fp_index_buffer_SRV = device_manager.create_SRV(_cptr_fp_index_buffer.Get());
  _cptr_fp_index_buffer_UAV = device_manager.create_UAV(_cptr_fp_index_buffer.Get());

  // create GCFP counter buffer
  _cptr_GCFP_count_buffer     = device_manager.create_structured_buffer(num_cell, GCFP_counts.data());
  _cptr_GCFP_count_buffer_SRV = device_manager.create_SRV(_cptr_GCFP_count_buffer.Get());
  _cptr_GCFP_count_buffer_UAV = device_manager.create_UAV(_cptr_GCFP_count_buffer.Get());

  // create GCFPT ID buffer
  _cptr_GCFP_ID_buffer     = device_manager.create_structured_buffer(num_particle, GCFP_IDs.data());
  _cptr_GCFP_ID_buffer_SRV = device_manager.create_SRV(_cptr_GCFP_ID_buffer.Get());
  _cptr_GCFP_ID_buffer_UAV = device_manager.create_UAV(_cptr_GCFP_ID_buffer.Get());

  // create changed GCFPT ID buffer
  _cptr_changed_GCFP_ID_buffer  = device_manager.create_structured_buffer<Changed_GCFPT_ID_Data>(estimated_num_update_data);
  _cptr_changed_GCFPT_ID_AC_UAV = device_manager.create_AC_UAV(_cptr_changed_GCFP_ID_buffer.Get(), estimated_num_update_data);
}

void Neighborhood_Uniform_Grid_GPU::init_nfp(void)
{
  constexpr UINT estimated_neighbor = 200;
  const auto     num_data           = _constant_data.num_particle * estimated_neighbor;

  _cptr_nfp_info_buffer     = _device_manager_ptr->create_structured_buffer<Neighbor_Information>(num_data);
  _cptr_nfp_info_buffer_SRV = _device_manager_ptr->create_SRV(_cptr_nfp_info_buffer.Get());
  _cptr_nfp_info_buffer_UAV = _device_manager_ptr->create_UAV(_cptr_nfp_info_buffer.Get());

  _cptr_nfp_count_buffer     = _device_manager_ptr->create_structured_buffer<UINT>(_constant_data.num_particle);
  _cptr_nfp_count_buffer_SRV = _device_manager_ptr->create_SRV(_cptr_nfp_count_buffer.Get());
  _cptr_nfp_count_buffer_UAV = _device_manager_ptr->create_UAV(_cptr_nfp_count_buffer.Get());

  // temporary code
  _cptr_nfp_info_staging_buffer  = _device_manager_ptr->create_staging_buffer_read(_cptr_nfp_info_buffer);
  _cptr_nfp_count_staging_buffer = _device_manager_ptr->create_staging_buffer_read(_cptr_nfp_count_buffer);
  // temporary code
}

Grid_Cell_ID Neighborhood_Uniform_Grid_GPU::grid_cell_id(const Vector3& v_pos) const
{
  const float dx = v_pos.x - _constant_data.domain_x_start;
  const float dy = v_pos.y - _constant_data.domain_y_start;
  const float dz = v_pos.z - _constant_data.domain_z_start;

  const size_t i = static_cast<size_t>(dx / _constant_data.divide_length);
  const size_t j = static_cast<size_t>(dy / _constant_data.divide_length);
  const size_t k = static_cast<size_t>(dz / _constant_data.divide_length);

  return {i, j, k};
}

UINT Neighborhood_Uniform_Grid_GPU::grid_cell_index(const Grid_Cell_ID& index_vector) const
{
  const auto num_x_cell = _constant_data.num_x_cell;
  const auto num_y_cell = _constant_data.num_y_cell;

  return static_cast<UINT>(index_vector.x + index_vector.y * num_x_cell + index_vector.z * num_x_cell * num_y_cell);
}

bool Neighborhood_Uniform_Grid_GPU::is_valid_index(const Grid_Cell_ID& index_vector) const
{
  const auto num_x_cell = _constant_data.num_x_cell;
  const auto num_y_cell = _constant_data.num_y_cell;
  const auto num_z_cell = _constant_data.num_z_cell;

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

const std::vector<size_t>& Neighborhood_Uniform_Grid_GPU::search_for_boundary(const size_t bpid) const
{
  return _bpid_to_neighbor_fpids[bpid];
}

void Neighborhood_Uniform_Grid_GPU::update(
  const std::vector<Vector3>& fluid_particle_pos_vectors,
  const std::vector<Vector3>& boundary_particle_pos_vectors)
{
  // temporary code
  const auto cptr_context = _device_manager_ptr->context_cptr();

  _device_manager_ptr->upload(fluid_particle_pos_vectors.data(), _cptr_fp_pos_staging_buffer);
  cptr_context->CopyResource(_cptr_fp_pos_buffer.Get(), _cptr_fp_pos_staging_buffer.Get());
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
  //
  struct Debug_Data
  {
    UINT cur_gc_index;
    UINT prev_gc_index;
  };
  auto debug_buffer = _device_manager_ptr->create_structured_buffer<Debug_Data>(_constant_data.num_particle);
  auto debug_UAV    = _device_manager_ptr->create_UAV(debug_buffer.Get());
  //

  constexpr size_t num_constant_buffer = 1;
  constexpr size_t num_SRV             = 2;
  constexpr size_t num_UAV             = 2;

  // auto before_count = _device_manager_ptr->read_count(_cptr_changed_GCFPT_ID_AC_UAV); // debug

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_constnat_buffer.Get()};

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fp_pos_buffer_SRV.Get(),
    _cptr_GCFP_ID_buffer_SRV.Get()};

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_changed_GCFPT_ID_AC_UAV.Get(),
    debug_UAV.Get()};

  const auto cptr_context = _device_manager_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_find_changed_GCFPT_ID_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_constant_data.num_particle / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  // const auto after_count = _device_manager_ptr->read_count(_cptr_changed_GCFPT_ID_AC_UAV);                 // debug
  // const auto debug_data  = _device_manager_ptr->read<Debug_Data>(debug_buffer);                            // debug
  // const auto read_data   = _device_manager_ptr->read<Changed_GCFPT_ID_Data>(_cptr_changed_GCFP_ID_buffer); // debug

  // int debug = 0;
  _device_manager_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::update_GCFP_buffer(void)
{
  const auto& device_manager = *_device_manager_ptr;
  const auto  cptr_context   = _device_manager_ptr->context_cptr();

  cptr_context->CopyStructureCount(_cptr_update_GCFP_CS_constant_buffer.Get(), 0, _cptr_changed_GCFPT_ID_AC_UAV.Get());

  constexpr size_t num_constant_buffer = 2;
  constexpr size_t num_UAV             = 5;

  // debug start
  struct Debug_Strcut
  {
    GCFP_ID prev_id;
    UINT    prev_id_GCFP_index;
    UINT    GCFP_count;
    UINT    prev_id_fp_index;
  };

  auto debug_buffer = _device_manager_ptr->create_structured_buffer<Debug_Strcut>(_constant_data.num_particle);
  auto debug_UAV    = _device_manager_ptr->create_UAV(debug_buffer.Get());
  // debug end

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_constnat_buffer.Get(),
    _cptr_update_GCFP_CS_constant_buffer.Get()};

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_fp_index_buffer_UAV.Get(),
    _cptr_GCFP_count_buffer_UAV.Get(),
    _cptr_GCFP_ID_buffer_UAV.Get(),
    _cptr_changed_GCFPT_ID_AC_UAV.Get(),
    debug_UAV.Get()};

  // debug
  // std::cout << "before \n";
  // const auto result = device_manager.read<UINT>(_cptr_GCFP_count_buffer);
  // auto       before_count           = _device_manager_ptr->read<UINT>(_cptr_update_GCFP_CS_constant_buffer);
  // const auto GCFP_ID_before         = device_manager.read<GCFP_ID>(_cptr_GCFP_ID_buffer);
  // const auto changed_GCFP_ID_result = _device_manager_ptr->read<Changed_GCFPT_ID_Data>(_cptr_changed_GCFP_ID_buffer);
  // const auto GCFP_count_before      = device_manager.read<UINT>(_cptr_GCFP_count_buffer);
  // const auto fp_index_before        = device_manager.read<UINT>(_cptr_fp_index_buffer);

  // const auto debug = 0;
  // debug

  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_GCFP_CS.Get(), nullptr, NULL);
  cptr_context->Dispatch(1, 1, 1);

  //// debug
  //{
  //  // std::cout << "after \n";
  //  const auto after_count      = _device_manager_ptr->read_count(_cptr_changed_GCFPT_ID_AC_UAV);
  //  const auto GCFP_count_after = device_manager.read<UINT>(_cptr_GCFP_count_buffer);
  //  const auto GCFP_ID_after    = device_manager.read<GCFP_ID>(_cptr_GCFP_ID_buffer);
  //  const auto fp_index_after   = device_manager.read<UINT>(_cptr_fp_index_buffer);
  //  const auto debug_data       = device_manager.read<Debug_Strcut>(debug_buffer);
  //  // print_sort_and_count(fp_index);
  //  // const auto index_for_25 = std::find(fp_index.begin(), fp_index.end(), 25) - fp_index.begin();
  //  const auto stop  = 0;
  //  const auto debug = 0;
  //  // std::cout << "\n\n";
  //}
  //// debug

  _device_manager_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::rearrange_GCFP(void)
{
  //// debug
  //const auto before_GCFP_id    = _device_manager_ptr->read<GCFP_ID>(_cptr_GCFP_ID_buffer);
  //const auto before_GCFP_count = _device_manager_ptr->read<UINT>(_cptr_GCFP_count_buffer);
  //print_sort_and_count(before_GCFP_count);
  //// debug


  constexpr UINT num_constant_buffer = 1;
  constexpr UINT num_UAV             = 3;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_constnat_buffer.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_fp_index_buffer_UAV.Get(),
    _cptr_GCFP_count_buffer_UAV.Get(),
    _cptr_GCFP_ID_buffer_UAV.Get(),
  };

  const auto cptr_context = _device_manager_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_rearrange_GCFP_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_constant_data.num_cell / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _device_manager_ptr->CS_barrier();

  //// debug
  //const auto after_GCFP_id    = _device_manager_ptr->read<GCFP_ID>(_cptr_GCFP_ID_buffer);
  //const auto after_GCFP_count = _device_manager_ptr->read<UINT>(_cptr_GCFP_count_buffer);
  //print_sort_and_count(after_GCFP_count);
  //const auto stop = 0;
  //// debug
}

void Neighborhood_Uniform_Grid_GPU::update_nfp(void)
{
  //// debug
  //const auto GCFP_id    = _device_manager_ptr->read<GCFP_ID>(_cptr_GCFP_ID_buffer);
  //const auto GCFP_count = _device_manager_ptr->read<UINT>(_cptr_GCFP_count_buffer);
  //print_sort_and_count(GCFP_count);
  //const auto stop = 0;
  //// debug

  constexpr auto num_constant_buffer = 1;
  constexpr auto num_SRV             = 6;
  constexpr auto num_UAV             = 2;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _cptr_common_constnat_buffer.Get()};

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

  const auto cptr_context = _device_manager_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_nfp_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_constant_data.num_particle / 1024.0f));
  cptr_context->Dispatch(num_group_x, 1, 1);

  _device_manager_ptr->CS_barrier();
}

void Neighborhood_Uniform_Grid_GPU::copy_to_ninfos(void)
{
  auto nfp_info  = _device_manager_ptr->read<Neighbor_Information>(_cptr_nfp_info_staging_buffer, _cptr_nfp_info_buffer);
  auto nfp_count = _device_manager_ptr->read<UINT>(_cptr_nfp_count_staging_buffer, _cptr_nfp_count_buffer);

  for (UINT i = 0; i < _constant_data.num_particle; ++i)
  {
    size_t index = i * _constant_data.estimated_num_nfp;

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