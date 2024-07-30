#include "Neighborhood_Uniform_Grid.h"

#include "Debugger.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <omp.h>

namespace ms
{

Neighborhood_Uniform_Grid::Neighborhood_Uniform_Grid(
  const Domain&               domain,
  const float                 divide_length,
  const std::vector<Vector3>& fluid_particle_pos_vectors,
  const std::vector<Vector3>& boundary_particle_pos_vectors)
    : _domain(domain), _divide_length(divide_length)
{
  const float dx = _domain.x_end - _domain.x_start;
  const float dy = _domain.y_end - _domain.y_start;
  const float dz = _domain.z_end - _domain.z_start;

  _num_x_cell = static_cast<size_t>(std::ceil(dx / divide_length));
  _num_y_cell = static_cast<size_t>(std::ceil(dy / divide_length));
  _num_z_cell = static_cast<size_t>(std::ceil(dz / divide_length));

  this->init_gcid_to_neighbor_gcids();

  const size_t num_cells = _num_x_cell * _num_y_cell * _num_z_cell;
  _gcid_to_fpids.resize(num_cells);

  const size_t num_fluid_particle = fluid_particle_pos_vectors.size();
  _fpid_to_gcid.resize(num_fluid_particle);
  _fpid_to_neighbor_informations.resize(num_fluid_particle);

  for (size_t fpid = 0; fpid < num_fluid_particle; ++fpid)
  {
    const auto& v_pos = fluid_particle_pos_vectors[fpid];
    const auto  gcid  = this->grid_cell_index(v_pos);

    _gcid_to_fpids[gcid].push_back(fpid);

    _fpid_to_gcid[fpid] = gcid;

    auto& neighbor_informations = _fpid_to_neighbor_informations[fpid];
    auto& neighbor_fp_indexes   = neighbor_informations.indexes;
    auto& neighbor_fp_tvectors  = neighbor_informations.translate_vectors;
    auto& neighbor_fp_distances = neighbor_informations.distances;

    neighbor_fp_indexes.reserve(100);
    neighbor_fp_tvectors.reserve(100);
    neighbor_fp_distances.reserve(100);
  }

  const auto num_boundary_particle = boundary_particle_pos_vectors.size();
  _bpid_to_gcid.resize(num_boundary_particle);
  _bpid_to_neighbor_fpids.resize(num_boundary_particle);

  for (size_t bpid = 0; bpid < num_boundary_particle; ++bpid)
  {
    auto&       neighbor_fpids = _bpid_to_neighbor_fpids[bpid];
    const auto& v_pos          = boundary_particle_pos_vectors[bpid];
    const auto  gcid           = this->grid_cell_index(v_pos);

    _bpid_to_gcid[bpid] = gcid;

    neighbor_fpids.reserve(100);
  }

  this->update_fpid_to_neighbor_fpids(fluid_particle_pos_vectors);
  this->update_bpid_to_neighbor_fpids(fluid_particle_pos_vectors, boundary_particle_pos_vectors);
}

size_t Neighborhood_Uniform_Grid::search(const Vector3& pos, size_t* pids) const
{
  const auto gcid_vector = this->grid_cell_index_vector(pos);

  const long long num_xy_cell = _num_x_cell * _num_y_cell;

  const long long delta[3] = {-1, 0, 1};

  size_t index = 0;

  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      for (int k = 0; k < 3; ++k)
      {
        Index_Vector neighbor_gcid_vector;
        neighbor_gcid_vector.x = gcid_vector.x + delta[i];
        neighbor_gcid_vector.y = gcid_vector.y + delta[j];
        neighbor_gcid_vector.z = gcid_vector.z + delta[k];

        if (!this->is_valid_index(neighbor_gcid_vector))
          continue;

        const auto neighbor_gcid = grid_cell_index(neighbor_gcid_vector);

        const auto&  neighbor_pids    = _gcid_to_fpids[neighbor_gcid];
        const size_t num_neighbor_pid = neighbor_pids.size();

        for (size_t i = 0; i < num_neighbor_pid; ++i)
          pids[index + i] = neighbor_pids[i];

        index += num_neighbor_pid;
      }
    }
  }

  return index;
}

const Neighbor_Informations& Neighborhood_Uniform_Grid::search_for_fluid(const size_t pid) const
{
  return _fpid_to_neighbor_informations[pid];
}

const std::vector<size_t>& Neighborhood_Uniform_Grid::search_for_boundary(const size_t bpid) const
{
  return _bpid_to_neighbor_fpids[bpid];
}

void Neighborhood_Uniform_Grid::update(
  const std::vector<Vector3>& fluid_particle_pos_vectors,
  const std::vector<Vector3>& boundary_particle_pos_vectors)
{
  const size_t num_fluid_particles = fluid_particle_pos_vectors.size();

  // update fpid_to_gcid and gcid_to_fpids
  for (size_t fpid = 0; fpid < num_fluid_particles; ++fpid)
  {
    const auto& v_xi = fluid_particle_pos_vectors[fpid];

    const auto prev_gcid = _fpid_to_gcid[fpid];

    const auto cur_gcid = this->grid_cell_index(v_xi);

    //debug
    if (!this->is_valid_index(cur_gcid))
      continue;
    //debug

    if (prev_gcid != cur_gcid)
    {
      // update gcid_to_fpids
      auto& prev_pids = _gcid_to_fpids[prev_gcid];
      prev_pids.erase(std::find(prev_pids.begin(), prev_pids.end(), fpid));

      auto& new_pids = _gcid_to_fpids[cur_gcid];
      new_pids.push_back(fpid);

      //update fpid_to_gcid
      _fpid_to_gcid[fpid] = cur_gcid;
    }
  }

  this->update_fpid_to_neighbor_fpids(fluid_particle_pos_vectors);
  this->update_bpid_to_neighbor_fpids(fluid_particle_pos_vectors, boundary_particle_pos_vectors);
}

Index_Vector Neighborhood_Uniform_Grid::grid_cell_index_vector(const Vector3& v_pos) const
{
  const float dx = v_pos.x - _domain.x_start;
  const float dy = v_pos.y - _domain.y_start;
  const float dz = v_pos.z - _domain.z_start;

  const size_t i = static_cast<size_t>(dx / _divide_length);
  const size_t j = static_cast<size_t>(dy / _divide_length);
  const size_t k = static_cast<size_t>(dz / _divide_length);

  return {i, j, k};
}

size_t Neighborhood_Uniform_Grid::grid_cell_index(const Vector3& v_pos) const
{
  const float dx = v_pos.x - _domain.x_start;
  const float dy = v_pos.y - _domain.y_start;
  const float dz = v_pos.z - _domain.z_start;

  const size_t i = static_cast<size_t>(dx / _divide_length);
  const size_t j = static_cast<size_t>(dy / _divide_length);
  const size_t k = static_cast<size_t>(dz / _divide_length);

  return i + j * _num_x_cell + k * _num_x_cell * _num_y_cell;
}

size_t Neighborhood_Uniform_Grid::grid_cell_index(const Index_Vector& index_vector) const
{
  return index_vector.x + index_vector.y * _num_x_cell + index_vector.z * _num_x_cell * _num_y_cell;
}

bool Neighborhood_Uniform_Grid::is_valid_index(const Index_Vector& index_vector) const
{
  if (_num_x_cell <= index_vector.x)
    return false;

  if (_num_y_cell <= index_vector.y)
    return false;

  if (_num_z_cell <= index_vector.z)
    return false;

  return true;
}

bool Neighborhood_Uniform_Grid::is_valid_index(const size_t gcid) const
{
  return gcid < _gcid_to_neighbor_gcids.size();
}

void Neighborhood_Uniform_Grid::update_fpid_to_neighbor_fpids(const std::vector<Vector3>& fluid_particle_pos_vectors)
{
  const size_t num_particles = fluid_particle_pos_vectors.size();

  static std::vector<size_t> total_debug; //debug
  std::vector<size_t> debug(num_particles);//debug

#pragma omp parallel for
  for (int i = 0; i < num_particles; ++i)
  {
    auto& informations = _fpid_to_neighbor_informations[i];
    auto& indexes      = informations.indexes;
    auto& tvectors     = informations.translate_vectors;
    auto& distances    = informations.distances;

    indexes.clear();
    tvectors.clear();
    distances.clear();

    const auto& v_xi = fluid_particle_pos_vectors[i];

    const auto gcid = this->grid_cell_index(v_xi);

    //debug
    if (!this->is_valid_index(gcid))
      continue;
    //debug

    const auto& neighbor_gcids = _gcid_to_neighbor_gcids[gcid];

    for (auto gcid : neighbor_gcids)
    {
      const auto& fpids = _gcid_to_fpids[gcid];
      for (auto neighbor_fp_id : fpids)
      {
        const auto& v_xj = fluid_particle_pos_vectors[neighbor_fp_id];

        const auto v_xij    = v_xi - v_xj;
        const auto distance = v_xij.Length();

        if (_divide_length < distance)
          continue;

        indexes.push_back(neighbor_fp_id);
        tvectors.push_back(v_xij);
        distances.push_back(distance);
      }
    }

    debug[i] = indexes.size();
  }
  
  total_debug.push_back(*std::max_element(debug.begin(), debug.end()));
  print_max(total_debug);
}

void Neighborhood_Uniform_Grid::update_bpid_to_neighbor_fpids(
  const std::vector<Vector3>& fluid_particle_pos_vectors,
  const std::vector<Vector3>& boundary_particle_pos_vectors)
{
  const size_t num_particles = boundary_particle_pos_vectors.size();

#pragma omp parallel for
  for (int bpid = 0; bpid < num_particles; ++bpid)
  {
    auto& neighbor_fpids = _bpid_to_neighbor_fpids[bpid];
    neighbor_fpids.clear();

    const auto& v_xi = boundary_particle_pos_vectors[bpid];

    const auto  gcid           = _bpid_to_gcid[bpid];
    const auto& neighbor_gcids = _gcid_to_neighbor_gcids[gcid];

    for (auto gcid : neighbor_gcids)
    {
      const auto& fpids = _gcid_to_fpids[gcid];
      for (auto fpid : fpids)
      {
        const auto& v_xj = fluid_particle_pos_vectors[fpid];
        if (_divide_length < (v_xi - v_xj).Length())
          continue;

        neighbor_fpids.push_back(fpid);
      }
    }
  }
}

void Neighborhood_Uniform_Grid::init_gcid_to_neighbor_gcids(void)
{
  constexpr long long delta[3] = {-1, 0, 1};

  const auto num_cells = _num_x_cell * _num_y_cell * _num_z_cell;
  _gcid_to_neighbor_gcids.resize(num_cells);

  for (size_t i = 0; i < _num_x_cell; ++i)
  {
    for (size_t j = 0; j < _num_y_cell; ++j)
    {
      for (size_t k = 0; k < _num_z_cell; ++k)
      {
        const auto gcid_vector = Index_Vector{i, j, k};
        const auto gcid        = this->grid_cell_index(gcid_vector);

        auto& neighbor_gcids = _gcid_to_neighbor_gcids[gcid];

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
              neighbor_gcids.push_back(neighbor_gcid);
            }
          }
        }
      }
    }
  }
}

} // namespace ms