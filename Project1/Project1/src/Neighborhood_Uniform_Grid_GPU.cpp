#include "Neighborhood_Uniform_Grid_GPU.h"

#include "Debugger.h"
#include "Device_Manager.h"

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
  const float dx = _domain.x_end - _domain.x_start;
  const float dy = _domain.y_end - _domain.y_start;
  const float dz = _domain.z_end - _domain.z_start;

  _num_x_cell = static_cast<size_t>(std::ceil(dx / divide_length));
  _num_y_cell = static_cast<size_t>(std::ceil(dy / divide_length));
  _num_z_cell = static_cast<size_t>(std::ceil(dz / divide_length));

  this->init_gcid_to_ngcids(device_manager);

  //const size_t num_cells = _num_x_cell * _num_y_cell * _num_z_cell;
  //_gcid_to_fpids.resize(num_cells);

  //const size_t num_fluid_particle = fluid_particle_pos_vectors.size();
  //_fpid_to_gcid.resize(num_fluid_particle);
  //_fpid_to_neighbor_informations.resize(num_fluid_particle);

  //for (size_t fpid = 0; fpid < num_fluid_particle; ++fpid)
  //{
  //  const auto& v_pos = fluid_particle_pos_vectors[fpid];
  //  const auto  gcid  = this->grid_cell_index(v_pos);

  //  _gcid_to_fpids[gcid].push_back(fpid);

  //  _fpid_to_gcid[fpid] = gcid;

  //  auto& neighbor_informations = _fpid_to_neighbor_informations[fpid];
  //  auto& neighbor_fp_indexes   = neighbor_informations.indexes;
  //  auto& neighbor_fp_tvectors  = neighbor_informations.translate_vectors;
  //  auto& neighbor_fp_distances = neighbor_informations.distances;

  //  neighbor_fp_indexes.reserve(100);
  //  neighbor_fp_tvectors.reserve(100);
  //  neighbor_fp_distances.reserve(100);
  //}

  //const auto num_boundary_particle = boundary_particle_pos_vectors.size();
  //_bpid_to_gcid.resize(num_boundary_particle);
  //_bpid_to_neighbor_fpids.resize(num_boundary_particle);

  //for (size_t bpid = 0; bpid < num_boundary_particle; ++bpid)
  //{
  //  auto&       neighbor_fpids = _bpid_to_neighbor_fpids[bpid];
  //  const auto& v_pos          = boundary_particle_pos_vectors[bpid];
  //  const auto  gcid           = this->grid_cell_index(v_pos);

  //  _bpid_to_gcid[bpid] = gcid;

  //  neighbor_fpids.reserve(100);
  //}

  //this->update_fpid_to_neighbor_fpids(fluid_particle_pos_vectors);
  //this->update_bpid_to_neighbor_fpids(fluid_particle_pos_vectors, boundary_particle_pos_vectors);
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
        const auto gcid_vector = Index_Vector{i, j, k};
        const auto gcid        = this->grid_cell_index(gcid_vector);

        auto index = gcid * num_max_data_per_cell;

        for (size_t p = 0; p < 3; ++p)
        {
          for (size_t q = 0; q < 3; ++q)
          {
            for (size_t r = 0; r < 3; ++r)
            {
              Index_Vector neighbor_gcid_vector;
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

void Neighborhood_Uniform_Grid_GPU::init_gcid_to_ngcids(const Device_Manager& device_manager)
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
  cptr_device->CreateTexture2D(&desc, &init_data, _cptr_gcid_to_ngcids_texture.GetAddressOf());
  cptr_device->CreateShaderResourceView(_cptr_gcid_to_ngcids_texture.Get(), nullptr, _cptr_gcid_to_ngcids_SRV.GetAddressOf());
}

void Neighborhood_Uniform_Grid_GPU::init_gcid_to_fpids(const Device_Manager& device_manager)
{
  constexpr size_t estimated_max_fpids = 40;

  const auto num_cells = _num_x_cell * _num_y_cell * _num_z_cell;

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = static_cast<UINT>(estimated_max_fpids);
  desc.Height               = static_cast<UINT>(num_cells);
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = DXGI_FORMAT_R8_SINT;
  desc.SampleDesc.Count     = 1;
  desc.Usage                = D3D11_USAGE_DEFAULT;
  desc.BindFlags            = D3D11_BIND_UNORDERED_ACCESS;
  desc.CPUAccessFlags       = NULL;
  desc.MiscFlags            = NULL;

  const auto cptr_device = device_manager.device_cptr();
  cptr_device->CreateTexture2D(&desc, nullptr, _cptr_gcid_to_fpids_texture.GetAddressOf());
  cptr_device->CreateUnorderedAccessView(_cptr_gcid_to_fpids_texture.Get(), nullptr, _cptr_gcid_to_fpids_UAV.GetAddressOf());
}

size_t Neighborhood_Uniform_Grid_GPU::grid_cell_index(const Index_Vector& index_vector) const
{
  return index_vector.x + index_vector.y * _num_x_cell + index_vector.z * _num_x_cell * _num_y_cell;
}

bool Neighborhood_Uniform_Grid_GPU::is_valid_index(const Index_Vector& index_vector) const
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
  //const size_t num_fluid_particles = fluid_particle_pos_vectors.size();

  //// update fpid_to_gcid and gcid_to_fpids
  //for (size_t fpid = 0; fpid < num_fluid_particles; ++fpid)
  //{
  //  const auto& v_xi = fluid_particle_pos_vectors[fpid];

  //  const auto prev_gcid = _fpid_to_gcid[fpid];

  //  const auto cur_gcid = this->grid_cell_index(v_xi);

  //  //debug
  //  if (!this->is_valid_index(cur_gcid))
  //    continue;
  //  //debug

  //  if (prev_gcid != cur_gcid)
  //  {
  //    // update gcid_to_fpids
  //    auto& prev_pids = _gcid_to_fpids[prev_gcid];
  //    prev_pids.erase(std::find(prev_pids.begin(), prev_pids.end(), fpid));

  //    auto& new_pids = _gcid_to_fpids[cur_gcid];
  //    new_pids.push_back(fpid);

  //    //update fpid_to_gcid
  //    _fpid_to_gcid[fpid] = cur_gcid;
  //  }
  //}

  //this->update_fpid_to_neighbor_fpids(fluid_particle_pos_vectors);
  //this->update_bpid_to_neighbor_fpids(fluid_particle_pos_vectors, boundary_particle_pos_vectors);
}

} // namespace ms