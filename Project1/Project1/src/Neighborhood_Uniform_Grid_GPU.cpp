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

  _common_constnat_buffer = device_manager.create_constant_buffer_imuutable(&_constant_data);

  this->init_ngc_texture();

  this->init_GCFP_texture(fluid_particle_pos_vectors);
  _cptr_find_changed_GCFPT_ID_CS        = device_manager.create_CS(L"hlsl/find_changed_GCFPT_ID_CS.hlsl");
  _cptr_update_GCFPT_CS                 = device_manager.create_CS(L"hlsl/update_GCFPT_CS.hlsl");
  _cptr_update_GCFPT_CS_constant_buffer = device_manager.create_constant_buffer(16);

  this->init_nfp();
  device_manager.create_CS(L"hlsl/update_nfp_CS.hlsl");

  // temporary code
  _cptr_fp_pos_buffer         = _device_manager_ptr->create_structured_buffer(_constant_data.num_particle, fluid_particle_pos_vectors.data());
  _cptr_fp_pos_buffer_SRV     = _device_manager_ptr->create_SRV(_cptr_fp_pos_buffer.Get());
  _cptr_fp_pos_staging_buffer = _device_manager_ptr->create_staging_buffer_read(_cptr_fp_pos_buffer);
  // temporary code

  this->update_nfp();
}

void Neighborhood_Uniform_Grid_GPU::init_ngc_texture(void)
{
  // make inital data
  constexpr long long delta[3]          = {-1, 0, 1};
  constexpr UINT      estimated_num_ngc = 27;

  const auto num_cell   = _constant_data.num_cell;
  const auto num_x_cell = _constant_data.num_x_cell;
  const auto num_y_cell = _constant_data.num_y_cell;
  const auto num_z_cell = _constant_data.num_z_cell;

  std::vector<std::array<UINT, estimated_num_ngc>> ngc_texture(num_cell);
  std::vector<UINT>                                ngc_counts(num_cell);

  for (size_t i = 0; i < num_x_cell; ++i)
  {
    for (size_t j = 0; j < num_y_cell; ++j)
    {
      for (size_t k = 0; k < num_z_cell; ++k)
      {
        const auto gcid_vector = Grid_Cell_ID{i, j, k};
        const auto gc_index    = this->grid_cell_index(gcid_vector);

        auto& ngcs = ngc_texture[gc_index];

        UINT ngc_count = 0;
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

              const auto neighbor_gcid = this->grid_cell_index(neighbor_gcid_vector);
              ngcs[ngc_count++]        = static_cast<UINT>(neighbor_gcid);
            }
          }
        }

        ngc_counts[gc_index] = ngc_count;
      }
    }
  }

  const auto& device_manager = *_device_manager_ptr;

  // create ngc texture
  _cptr_ngc_texture     = device_manager.create_texutre_2D_immutable(estimated_num_ngc, num_cell, DXGI_FORMAT_R32_UINT, ngc_texture.data());
  _cptr_ngc_texture_SRV = device_manager.create_SRV(_cptr_ngc_texture.Get());

  // create ngc count buffer
  _cptr_ngc_count_buffer     = device_manager.create_structured_buffer(num_cell, ngc_counts.data());
  _cptr_ngc_count_buffer_SRV = device_manager.create_SRV(_cptr_ngc_count_buffer.Get());
}

void Neighborhood_Uniform_Grid_GPU::init_GCFP_texture(const std::vector<Vector3>& fluid_particle_pos_vectors)
{
  // make initial data
  const auto num_cell     = _constant_data.num_cell;
  const auto num_particle = _constant_data.num_particle;

  std::vector<std::array<UINT, 40>> GCFP_texture(num_cell);
  std::vector<UINT>                 GCFP_counts(num_cell);
  std::vector<GCFPT_ID>             GCFPT_IDs(num_particle);

  for (UINT i = 0; i < num_particle; ++i)
  {
    const auto& v_pos    = fluid_particle_pos_vectors[i];
    const auto  gc_id    = this->grid_cell_id(v_pos);
    const auto  gc_index = this->grid_cell_index(gc_id);

    GCFPT_ID id;
    id.gc_index   = gc_index;
    id.gcfp_index = GCFP_counts[gc_index];

    GCFP_texture[gc_index][id.gcfp_index] = i;
    GCFP_counts[gc_index]++;
    GCFPT_IDs[i] = id;
  }

  const auto& device_manager = *_device_manager_ptr;

  // create GCFP texture
  constexpr UINT estimated_num_fpids       = 40;
  const auto     estimated_num_update_data = _constant_data.num_particle / 10;

  _cptr_GCFP_texture     = device_manager.create_texutre_2D(estimated_num_fpids, num_cell, DXGI_FORMAT_R32_UINT, GCFP_texture.data());
  _cptr_GCFP_texture_SRV = device_manager.create_SRV(_cptr_GCFP_texture.Get());
  _cptr_GCFP_texture_UAV = device_manager.create_UAV(_cptr_GCFP_texture.Get());

  // create GCFP counter buffer
  _cptr_GCFP_count_buffer     = device_manager.create_structured_buffer(num_cell, GCFP_counts.data());
  _cptr_GCFP_count_buffer_SRV = device_manager.create_SRV(_cptr_GCFP_count_buffer.Get());
  _cptr_GCFP_count_buffer_UAV = device_manager.create_UAV(_cptr_GCFP_count_buffer.Get());

  // create GCFPT ID buffer
  _cptr_GCFPT_ID_buffer     = device_manager.create_structured_buffer(num_particle, GCFPT_IDs.data());
  _cptr_GCFPT_ID_buffer_SRV = device_manager.create_SRV(_cptr_GCFPT_ID_buffer.Get());
  _cptr_GCFPT_ID_buffer_UAV = device_manager.create_UAV(_cptr_GCFPT_ID_buffer.Get());

  // create changed GCFPT ID buffer
  _cptr_changed_GCFPT_ID_buffer = device_manager.create_structured_buffer<Changed_GCFPT_ID_Data>(estimated_num_update_data);
  _cptr_changed_GCFPT_ID_AC_UAV = device_manager.create_AC_UAV(_cptr_changed_GCFPT_ID_buffer.Get(), estimated_num_update_data);
}

void Neighborhood_Uniform_Grid_GPU::init_nfp(void)
{
  constexpr UINT estimated_neighbor = 200;

  const UINT width           = estimated_neighbor;
  const UINT height          = _constant_data.num_particle;
  const auto nfp_format      = DXGI_FORMAT_R32_UINT;
  const auto nfp_tvec_format = DXGI_FORMAT_R32G32B32A32_FLOAT; //R32G32B32는 UAV가 지원이 안됨!
  const auto nfp_dist_format = DXGI_FORMAT_R32_FLOAT;

  const auto& device_manager = *_device_manager_ptr;
  _cptr_nfp_texture          = device_manager.create_texutre_2D<UINT>(width, height, nfp_format);
  _cptr_nfp_texture_SRV      = device_manager.create_SRV(_cptr_nfp_texture.Get());
  _cptr_nfp_texture_UAV      = device_manager.create_UAV(_cptr_nfp_texture.Get());

  _cptr_nfp_tvec_texture     = device_manager.create_texutre_2D<Vector4>(width, height, nfp_tvec_format);
  _cptr_nfp_tvec_texture_SRV = device_manager.create_SRV(_cptr_nfp_tvec_texture.Get());
  _cptr_nfp_tvec_texture_UAV = device_manager.create_UAV(_cptr_nfp_tvec_texture.Get());

  _cptr_nfp_dist_texture     = device_manager.create_texutre_2D<float>(width, height, nfp_dist_format);
  _cptr_nfp_dist_texture_SRV = device_manager.create_SRV(_cptr_nfp_dist_texture.Get());
  _cptr_nfp_dist_texture_UAV = device_manager.create_UAV(_cptr_nfp_dist_texture.Get());

  _cptr_nfp_count_buffer     = device_manager.create_structured_buffer<UINT>(_constant_data.num_particle);
  _cptr_nfp_count_buffer_SRV = device_manager.create_SRV(_cptr_nfp_count_buffer.Get());
  _cptr_nfp_count_buffer_UAV = device_manager.create_UAV(_cptr_nfp_count_buffer.Get());

  // temporary code
  _cptr_nfp_staging_texture      = device_manager.create_staging_texture_read(_cptr_nfp_texture);
  _cptr_nfp_tvec_staging_texture = device_manager.create_staging_texture_read(_cptr_nfp_tvec_texture);
  _cptr_nfp_dist_staging_texture = device_manager.create_staging_texture_read(_cptr_nfp_dist_texture);
  _cptr_nfp_count_staging_buffer = device_manager.create_staging_buffer_read(_cptr_nfp_count_buffer);
  //temporary code
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
  return _ninfos[pid];
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

  _device_manager_ptr->CS_barrier();

  this->update_GCFP_texture();

  _device_manager_ptr->CS_barrier();

  this->update_nfp();

  _device_manager_ptr->CS_barrier();

  //temporary code

  //temporary code
}

void Neighborhood_Uniform_Grid_GPU::find_changed_GCFPT_ID(void)
{
  constexpr size_t num_SRV = 2;
  constexpr size_t num_UAV = 1;

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fp_pos_buffer_SRV.Get(),
    _cptr_GCFPT_ID_buffer_SRV.Get()};

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_changed_GCFPT_ID_AC_UAV.Get()};

  const auto cptr_context = _device_manager_ptr->context_cptr();
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_find_changed_GCFPT_ID_CS.Get(), nullptr, NULL);

  const UINT num_group_x = static_cast<UINT>(std::ceil(_constant_data.num_particle / 1024));
  cptr_context->Dispatch(num_group_x, 1, 1);
}

void Neighborhood_Uniform_Grid_GPU::update_GCFP_texture(void)
{
  const auto& device_manager = *_device_manager_ptr;

  const auto num_changed = device_manager.read_count(_cptr_changed_GCFPT_ID_AC_UAV);
  device_manager.upload(&num_changed, _cptr_update_GCFPT_CS_constant_buffer);

  constexpr size_t num_UAV      = 4;
  const auto       cptr_context = _device_manager_ptr->context_cptr();

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_GCFP_texture_UAV.Get(),
    _cptr_GCFP_count_buffer_UAV.Get(),
    _cptr_GCFPT_ID_buffer_UAV.Get(),
    _cptr_changed_GCFPT_ID_AC_UAV.Get()};

  cptr_context->CSSetConstantBuffers(0, 1, _cptr_update_GCFPT_CS_constant_buffer.GetAddressOf());
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_GCFPT_CS.Get(), nullptr, NULL);
  cptr_context->Dispatch(1, 1, 1);
}

void Neighborhood_Uniform_Grid_GPU::update_nfp(void)
{
  constexpr auto num_constant_buffer = 1;
  constexpr auto num_SRV             = 6;
  constexpr auto num_UAV             = 4;

  ID3D11Buffer* constant_buffers[num_constant_buffer] = {
    _common_constnat_buffer.Get()};

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fp_pos_buffer_SRV.Get(),
    _cptr_GCFP_texture_SRV.Get(),
    _cptr_GCFP_count_buffer_SRV.Get(),
    _cptr_GCFPT_ID_buffer_SRV.Get(),
    _cptr_ngc_texture_SRV.Get(),
    _cptr_ngc_count_buffer_SRV.Get()};

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_nfp_texture_UAV.Get(),
    _cptr_nfp_tvec_texture_UAV.Get(),
    _cptr_nfp_dist_texture_UAV.Get(),
    _cptr_nfp_count_buffer_UAV.Get()};

  const auto cptr_context = _device_manager_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_constant_buffer, constant_buffers);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_nfp_CS.Get(), nullptr, NULL);

  const auto num_group_x = static_cast<UINT>(std::ceil(_constant_data.num_particle / 1024));
  cptr_context->Dispatch(num_group_x, 1, 1);
}

void Neighborhood_Uniform_Grid_GPU::copy_to_ninfos(void)
{
  const auto cptr_context = _device_manager_ptr->context_cptr();

  cptr_context->CopyResource(_cptr_nfp_staging_texture.Get(), _cptr_nfp_texture.Get());
  cptr_context->CopyResource(_cptr_nfp_tvec_staging_texture.Get(), _cptr_nfp_tvec_texture.Get());
  cptr_context->CopyResource(_cptr_nfp_dist_staging_texture.Get(), _cptr_nfp_dist_texture.Get());
  cptr_context->CopyResource(_cptr_nfp_count_staging_buffer.Get(), _cptr_nfp_count_buffer.Get());

  auto nfp_index = _device_manager_ptr->download<UINT>(_cptr_nfp_staging_texture);
  auto nfp_tvec  = _device_manager_ptr->download<Vector4>(_cptr_nfp_tvec_texture);
  auto nfp_dist  = _device_manager_ptr->download<float>(_cptr_nfp_dist_staging_texture);
  auto nfp_count = _device_manager_ptr->download<UINT>(_cptr_nfp_count_buffer);

  for (UINT i = 0; i < _constant_data.num_particle; ++i)
  {
    auto& ninfo = _ninfos[i];

    const UINT nn = nfp_count[i];

    auto& indexes = nfp_index[i];
    auto& tvecs   = nfp_tvec[i];
    auto& dists   = nfp_dist[i];

    ninfo.indexes.resize(nn);
    std::copy(indexes.begin(), indexes.begin() + nn, ninfo.indexes.begin());

    ninfo.translate_vectors.resize(nn);
    for (UINT j=0; j< nn; ++j)
    {
      ninfo.translate_vectors[j].x = tvecs[j].x;
      ninfo.translate_vectors[j].y = tvecs[j].y;
      ninfo.translate_vectors[j].z = tvecs[j].z;
    }

    ninfo.distances.resize(nn);
    std::copy(dists.begin(), dists.begin() + nn, ninfo.distances.begin());
  }
}

} // namespace ms