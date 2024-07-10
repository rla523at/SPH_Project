#pragma once
#include "Neighborhood.h"

//abbreviation
//pid	: particle index
//gcid	: geometry cell index

//Data Structure
namespace ms
{
struct Index_Vector
{
  size_t x = 0;
  size_t y = 0;
  size_t z = 0;
};

struct Domain
{
  float x_start = 0.0f;
  float x_end   = 0.0f;
  float y_start = 0.0f;
  float y_end   = 0.0f;
  float z_start = 0.0f;
  float z_end   = 0.0f;
};
} // namespace ms

namespace ms
{
class Uniform_Grid : public Neighborhood
{
public:
  Uniform_Grid(const Domain& domain, const float divide_length, const std::vector<Vector3>& pos_vectors);

public:
  void update(const std::vector<Vector3>& pos_vectors) override;

  // fill neighbor particle ids into pids and return number of neighbor particles
  //size_t              search(const Vector3& pos, size_t* pids) const override;
  std::vector<size_t> search(const Vector3& pos) const override;

private:
  Index_Vector grid_cell_index_vector(const Vector3& v_pos) const;
  size_t       grid_cell_index(const Vector3& v_pos) const;
  size_t       grid_cell_index(const Index_Vector& index_vector) const;

private:
  std::vector<std::vector<size_t>> _gcid_to_pids;
  std::vector<size_t>              _pid_to_gcid;

  Domain _domain;
  float  _divide_length = 0.0f;

  size_t _num_x_cell = 0;
  size_t _num_y_cell = 0;
  size_t _num_z_cell = 0;
};

class Neighborhood_Brute_Force : public Neighborhood
{
public:
  Neighborhood_Brute_Force(const size_t num_particles)
      : _num_particles(num_particles){};

public:
  void update(const std::vector<Vector3>& pos_vectors) override {};

  std::vector<size_t> search(const Vector3& pos) const override
  {
    std::vector<size_t> indexes(_num_particles);
    for (int i=0; i<_num_particles; ++i)
      indexes[i] = i;

    return indexes;
  }

  private:
  size_t _num_particles = 0;
};

} // namespace ms