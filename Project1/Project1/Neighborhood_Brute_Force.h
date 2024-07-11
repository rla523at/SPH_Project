#pragma once
#include "Neighborhood.h"

namespace ms
{
class Neighborhood_Brute_Force : public Neighborhood
{
public:
  Neighborhood_Brute_Force(const size_t num_particles)
      : _num_particles(num_particles)
  {
    _pids.resize(_num_particles);
    for (int i = 0; i < _num_particles; ++i)
      _pids[i] = i;
  };

public:
  void update(const std::vector<Vector3>& pos_vectors) override{};

  const std::vector<size_t>& search(const size_t pid) const override
  {
    return _pids;
  }

private:
  size_t              _num_particles = 0;
  std::vector<size_t> _pids;
};
} // namespace ms