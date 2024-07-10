#include "Neighborhood_Uniform_Grid.h"

#include <algorithm>
#include <cmath>

namespace ms
{

Neighborhood_Uniform_Grid::Neighborhood_Uniform_Grid(const Domain& domain, const float divide_length, const std::vector<Vector3>& pos_vectors)
    : _domain(domain), _divide_length(divide_length)
{
  const float dx = _domain.x_end - _domain.x_start;
  const float dy = _domain.y_end - _domain.y_start;
  const float dz = _domain.z_end - _domain.z_start;

  _num_x_cell = static_cast<size_t>(std::ceil(dx / divide_length));
  _num_y_cell = static_cast<size_t>(std::ceil(dy / divide_length));
  _num_z_cell = static_cast<size_t>(std::ceil(dz / divide_length));

  const size_t num_cells = _num_x_cell * _num_y_cell * _num_z_cell;
  _gcid_to_pids.resize(num_cells);

  const size_t num_particles = pos_vectors.size();
  _pid_to_gcid.resize(num_particles);

  for (size_t pid = 0; pid < num_particles; ++pid)
  {
    const auto& v_pos = pos_vectors[pid];

    const auto gcid = this->grid_cell_index(v_pos);

    _pid_to_gcid[pid] = gcid;
    _gcid_to_pids[gcid].push_back(pid);
  }
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

        const auto&  neighbor_pids    = _gcid_to_pids[neighbor_gcid];
        const size_t num_neighbor_pid = neighbor_pids.size();

        for (size_t i = 0; i < num_neighbor_pid; ++i)
          pids[index + i] = neighbor_pids[i];

        index += num_neighbor_pid;
      }
    }
  }

  return index;
}

std::vector<size_t> Neighborhood_Uniform_Grid::search(const Vector3& pos) const
{
  constexpr size_t    num_expect_neighbor = 50;
  constexpr long long delta[3] = {-1, 0, 1};

  const auto gcid_vector = this->grid_cell_index_vector(pos);

  std::vector<size_t> result;
  result.reserve(num_expect_neighbor);

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

        const auto&  neighbor_pids    = _gcid_to_pids[neighbor_gcid];
        const size_t num_neighbor_pid = neighbor_pids.size();

        for (size_t i = 0; i < num_neighbor_pid; ++i)
          result.push_back(neighbor_pids[i]);
      }
    }
  }

  return result;
}

void Neighborhood_Uniform_Grid::update(const std::vector<Vector3>& pos_vectors)
{
  const size_t num_particles = pos_vectors.size();

  for (size_t pid = 0; pid < num_particles; ++pid)
  {
    const auto prev_gcid = _pid_to_gcid[pid];

    const auto& v_pos    = pos_vectors[pid];
    const auto  cur_gcid = this->grid_cell_index(v_pos);

    if (prev_gcid != cur_gcid)
    {
      //erase data in previous
      auto& prev_pids = _gcid_to_pids[prev_gcid];
      prev_pids.erase(std::find(prev_pids.begin(), prev_pids.end(), pid));

      //insert data in new
      auto& new_pids = _gcid_to_pids[cur_gcid];
      new_pids.push_back(pid);

      //update gcid
      _pid_to_gcid[pid] = cur_gcid;
    }
  }
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

} // namespace ms