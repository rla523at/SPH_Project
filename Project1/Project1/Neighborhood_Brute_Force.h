#pragma once
#include "Neighborhood.h"

namespace ms
{
class Neighborhood_Brute_Force : public Neighborhood
{
public:
  Neighborhood_Brute_Force(const size_t num_particles)
      : _num_particles(num_particles){};

public:
  void update(const std::vector<Vector3>& pos_vectors) override{};

  std::vector<size_t> search(const Vector3& pos) const override
  {
    std::vector<size_t> indexes(_num_particles);
    for (int i = 0; i < _num_particles; ++i)
      indexes[i] = i;

    return indexes;
  }
      
  size_t search(const Vector3& pos, size_t* pids) const override
  {
    for (int i = 0; i < _num_particles; ++i)
      pids[i] = i;

    return _num_particles;
  }

private:
  size_t _num_particles = 0;
};
}