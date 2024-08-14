#pragma once

#include "Abbreviation.h"

#include <vector>

namespace ms
{
struct Neighbor_Informations
{
  std::vector<UINT>  indexes;
  std::vector<Vector3> translate_vectors;
  std::vector<float>   distances;
};
} // namespace ms

namespace ms
{
class Neighborhood
{
public:
  virtual void update(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors) = 0;

  virtual const Neighbor_Informations& search_for_fluid(const UINT fpid) const    = 0;
  virtual const std::vector<UINT>&   search_for_boundary(const UINT bpid) const = 0;
};
} // namespace ms
