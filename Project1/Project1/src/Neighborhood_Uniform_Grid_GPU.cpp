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
  : _domain(domain), _divide_length(divide_length)
{
  _device_manager_ptr = &device_manager;

  const float dx = _domain.x_end - _domain.x_start;
  const float dy = _domain.y_end - _domain.y_start;
  const float dz = _domain.z_end - _domain.z_start;

  _num_x_cell   = static_cast<size_t>(std::ceil(dx / divide_length));
  _num_y_cell   = static_cast<size_t>(std::ceil(dy / divide_length));
  _num_z_cell   = static_cast<size_t>(std::ceil(dz / divide_length));
  _num_particle = fluid_particle_pos_vectors.size();

  const auto num_cell = _num_x_cell * _num_y_cell * _num_z_cell;

  this->init_GCFP_texture(GCFP_texture.data()->data(), GCFP_counter_buffer.data());

  this->init_ngc_texture(device_manager);
  this->init_fpid_to_ninfo(device_manager);

  // temporary
  _cptr_fp_pos_buffer         = _device_manager_ptr->create_structured_buffer(_num_particle, fluid_particle_pos_vectors.data());
  _cptr_fp_pos_buffer_SRV     = _device_manager_ptr->create_SRV(_cptr_fp_pos_buffer.Get());
  _cptr_fp_pos_staging_buffer = _device_manager_ptr->create_staging_buffer_read(_cptr_fp_pos_buffer);
}

std::vector<int> Neighborhood_Uniform_Grid_GPU::make_gcid_to_ngcids_initial_data(void)
{
  constexpr long long delta[3]              = {-1, 0, 1};
  constexpr size_t    num_max_data_per_cell = 27;

  const auto num_cells = _num_x_cell * _num_y_cell * _num_z_cell;
  const auto num_data  = num_cells * num_max_data_per_cell;

  std::vector<int> neighbor_gcids(num_data, -1);

  for (size_t i = 0; i < _num_x_cell; ++i)
  {
    for (size_t j = 0; j < _num_y_cell; ++j)
    {
      for (size_t k = 0; k < _num_z_cell; ++k)
      {
        const auto gcid_vector = Grid_Cell_ID{i, j, k};
        const auto gcid        = this->grid_cell_index(gcid_vector);

        auto index = gcid * num_max_data_per_cell;

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
              neighbor_gcids[index++]  = static_cast<int>(neighbor_gcid);
            }
          }
        }
      }
    }
  }

  return neighbor_gcids;
}

void Neighborhood_Uniform_Grid_GPU::init_ngc_texture(const Device_Manager& device_manager)
{
  constexpr size_t num_max_neighbor_cell = 27;

  const auto num_cells = _num_x_cell * _num_y_cell * _num_z_cell;

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = static_cast<UINT>(num_max_neighbor_cell);
  desc.Height               = static_cast<UINT>(num_cells);
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = DXGI_FORMAT_R8_SINT;
  desc.SampleDesc.Count     = 1;
  desc.Usage                = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags       = NULL;
  desc.MiscFlags            = NULL;

  const auto gcid_to_neighbor_gcids = this->make_gcid_to_ngcids_initial_data();

  D3D11_SUBRESOURCE_DATA init_data;
  init_data.pSysMem          = gcid_to_neighbor_gcids.data();
  init_data.SysMemPitch      = 0;
  init_data.SysMemSlicePitch = 0;

  const auto cptr_device = device_manager.device_cptr();
  cptr_device->CreateTexture2D(&desc, &init_data, _cptr_ngc_texture.GetAddressOf());
  cptr_device->CreateShaderResourceView(_cptr_ngc_texture.Get(), nullptr, _cptr_ngc_texture_SRV.GetAddressOf());
}

void Neighborhood_Uniform_Grid_GPU::init_GCFP_texture(const std::vector<Vector3>& fluid_particle_pos_vectors)
{
  const auto num_cell = _num_x_cell * _num_y_cell * _num_z_cell;

  // make initial data
  std::vector<std::array<UINT, 40>> GCFP_texture(num_cell);
  std::vector<UINT>                 GCFP_counter_buffer(num_cell);
  std::vector<GCFPT_ID>             GCFPT_ID_buffer(_num_particle);

  for (size_t i = 0; i < _num_particle; ++i)
  {
    const auto& v_pos    = fluid_particle_pos_vectors[i];
    const auto  gc_id    = this->grid_cell_id(v_pos);
    const auto  gc_index = this->grid_cell_index(gc_id);

    GCFPT_ID id;
    id.gc_index   = gc_index;
    id.gcfp_index = GCFP_counter_buffer[gc_index];

    GCFP_texture[gc_index][id.gcfp_index] = i;
    GCFP_counter_buffer[gc_index]++;
    GCFPT_ID_buffer[i] = id;
  }

  const auto& device_manager = *_device_manager_ptr;

  // init GCFP_texture related
  constexpr size_t estimated_num_fpids       = 40;
  const auto       estimated_num_update_data = _num_particle / 10;

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = static_cast<UINT>(estimated_num_fpids);
  desc.Height               = static_cast<UINT>(num_cell);
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = DXGI_FORMAT_R32_UINT;
  desc.SampleDesc.Count     = 1;
  desc.Usage                = D3D11_USAGE_DEFAULT;
  desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  desc.CPUAccessFlags       = NULL;
  desc.MiscFlags            = NULL;

  D3D11_SUBRESOURCE_DATA init_data = {};
  init_data.pSysMem                = GCFP_texture.data();
  init_data.SysMemPitch            = static_cast<UINT>(estimated_num_fpids);
  init_data.SysMemSlicePitch       = 0;

  // create GCFP texture
  const auto cptr_device = device_manager.device_cptr();
  cptr_device->CreateTexture2D(&desc, nullptr, _cptr_GCFP_texture.GetAddressOf());
  _cptr_GCFP_texture_SRV = device_manager.create_SRV(_cptr_GCFP_texture.Get());
  _cptr_GCFP_texture_UAV = device_manager.create_UAV(_cptr_GCFP_texture.Get());

  // create GCFP counter buffer
  _cptr_GCFP_counter_buffer     = device_manager.create_structured_buffer(num_cell, GCFP_counter_buffer.data());
  _cptr_GCFP_counter_SRV        = device_manager.create_SRV(_cptr_GCFP_counter_buffer.Get());
  _cptr_GCFP_counter_buffer_UAV = device_manager.create_UAV(_cptr_GCFP_counter_buffer.Get());

  // create update GCFPT CS
  _cptr_update_GCFPT_CS                 = device_manager.create_CS(L"hlsl/update_GCFPT_CS.hlsl");
  _cptr_update_GCFPT_CS_constant_buffer = device_manager.create_constant_buffer(16);

  // create GCFPT ID buffer
  _cptr_GCFPT_ID_buffer     = device_manager.create_structured_buffer(_num_particle, GCFPT_ID_buffer.data());
  _cptr_GCFPT_ID_buffer_SRV = device_manager.create_SRV(_cptr_GCFPT_ID_buffer.Get());
  _cptr_GCFPT_ID_buffer_UAV = device_manager.create_UAV(_cptr_GCFPT_ID_buffer.Get());

  // create changed GCFPT ID buffer
  _cptr_changed_GCFPT_ID_buffer = device_manager.create_structured_buffer<Changed_GCFPT_ID_Data>(estimated_num_update_data);
  _cptr_changed_GCFPT_ID_AC_UAV = device_manager.create_AC_UAV(_cptr_changed_GCFPT_ID_buffer.Get(), estimated_num_update_data);

  _cptr_count_staging_buffer = device_manager.create_staging_buffer_count(); //device manager가 가지고 있게 만들자.

  // create find changed GCFPT ID CS
  _cptr_find_changed_GCFPT_ID_CS = device_manager.create_CS(L"hlsl/find_changed_GCFPT_ID_CS.hlsl");
}

void Neighborhood_Uniform_Grid_GPU::init_fpid_to_ninfo(const Device_Manager& device_manager)
{
  constexpr auto data_size = sizeof(Neighbor_Informations_GPU);
  const auto     num_data  = _num_particle;

  _cptr_ninfo_buffer                     = device_manager.create_structured_buffer<Neighbor_Informations_GPU>(num_data);
  _cptr_ninfo_buffer_SRV                 = device_manager.create_SRV(_cptr_ninfo_buffer.Get());
  _cptr_ninfo_buffer_UAV                 = device_manager.create_UAV(_cptr_ninfo_buffer.Get());
  _cptr_fp_index_to_ninfo_staging_buffer = device_manager.create_staging_buffer_read(_cptr_ninfo_buffer);
}

Grid_Cell_ID Neighborhood_Uniform_Grid_GPU::grid_cell_id(const Vector3& v_pos) const
{
  const float dx = v_pos.x - _domain.x_start;
  const float dy = v_pos.y - _domain.y_start;
  const float dz = v_pos.z - _domain.z_start;

  const size_t i = static_cast<size_t>(dx / _divide_length);
  const size_t j = static_cast<size_t>(dy / _divide_length);
  const size_t k = static_cast<size_t>(dz / _divide_length);

  return {i, j, k};
}

size_t Neighborhood_Uniform_Grid_GPU::grid_cell_index(const Grid_Cell_ID& index_vector) const
{
  return index_vector.x + index_vector.y * _num_x_cell + index_vector.z * _num_x_cell * _num_y_cell;
}

bool Neighborhood_Uniform_Grid_GPU::is_valid_index(const Grid_Cell_ID& index_vector) const
{
  if (_num_x_cell <= index_vector.x)
    return false;

  if (_num_y_cell <= index_vector.y)
    return false;

  if (_num_z_cell <= index_vector.z)
    return false;

  return true;
}

const Neighbor_Informations& Neighborhood_Uniform_Grid_GPU::search_for_fluid(const size_t pid) const
{
  return _fpid_to_neighbor_informations[pid];
}

const std::vector<size_t>& Neighborhood_Uniform_Grid_GPU::search_for_boundary(const size_t bpid) const
{
  return _bpid_to_neighbor_fpids[bpid];
}

void Neighborhood_Uniform_Grid_GPU::update(
  const std::vector<Vector3>& fluid_particle_pos_vectors,
  const std::vector<Vector3>& boundary_particle_pos_vectors)
{
  const auto cptr_context = _device_manager_ptr->context_cptr();

  // temporary code
  _device_manager_ptr->upload(fluid_particle_pos_vectors.data(), _cptr_fp_pos_staging_buffer);
  cptr_context->CopyResource(_cptr_fp_pos_buffer.Get(), _cptr_fp_pos_staging_buffer.Get());
  // temporary code

  this->find_changed_GCFPT_ID();

  _device_manager_ptr->CS_barrier();

  this->update_GCFP_texture();

  _device_manager_ptr->CS_barrier();

  // this->update_fpid_to_neighbor_fpids(fluid_particle_pos_vectors);
  // this->update_bpid_to_neighbor_fpids(fluid_particle_pos_vectors, boundary_particle_pos_vectors);
}

void Neighborhood_Uniform_Grid_GPU::find_changed_GCFPT_ID(void)
{
  const auto cptr_context = _device_manager_ptr->context_cptr();

  const UINT num_group_x = static_cast<UINT>(std::ceil(_num_particle / 1024));

  constexpr size_t num_SRV = 2;
  constexpr size_t num_UAV = 1;

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    _cptr_fp_pos_buffer_SRV.Get(),
    _cptr_GCFPT_ID_buffer_SRV.Get()};

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_changed_GCFPT_ID_AC_UAV.Get()};

  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_find_changed_GCFPT_ID_CS.Get(), nullptr, NULL);
  cptr_context->Dispatch(num_group_x, 1, 1);
}

void Neighborhood_Uniform_Grid_GPU::update_GCFP_texture(void)
{
  const auto& device_manager = *_device_manager_ptr;

  const auto num_changed = device_manager.read_count(_cptr_changed_GCFPT_ID_AC_UAV, _cptr_count_staging_buffer);
  device_manager.upload(&num_changed, _cptr_update_GCFPT_CS_constant_buffer);

  constexpr size_t num_UAV      = 4;
  const auto       cptr_context = _device_manager_ptr->context_cptr();

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    _cptr_GCFP_texture_UAV.Get(),
    _cptr_GCFP_counter_buffer_UAV.Get(),
    _cptr_GCFPT_ID_buffer_UAV.Get(),
    _cptr_changed_GCFPT_ID_AC_UAV.Get()};

  cptr_context->CSSetConstantBuffers(0, 1, _cptr_update_GCFPT_CS_constant_buffer.GetAddressOf());
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);
  cptr_context->CSSetShader(_cptr_update_GCFPT_CS.Get(), nullptr, NULL);
  cptr_context->Dispatch(1, 1, 1);
}

} // namespace ms