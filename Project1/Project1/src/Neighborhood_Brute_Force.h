#pragma once
#include "Neighborhood.h"

namespace ms
{
class Neighborhood_Brute_Force : public Neighborhood
{
public:
  Neighborhood_Brute_Force(const UINT num_particles)
      : _num_particles(num_particles)
  {
    auto& pids = _information.indexes;

    pids.resize(_num_particles);
    for (UINT i = 0; i < _num_particles; ++i)
      pids[i] = i;
  };

public:
  void update(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors) override{};

  const Neighbor_Informations& search_for_fluid(const UINT pid) const override
  {
    return _information;
  }
  const std::vector<UINT>& search_for_boundary(const UINT bpid) const override
  {
    return _information.indexes;//temporary code
  };

private:
  UINT              _num_particles = 0;
  Neighbor_Informations _information;
};
} // namespace ms