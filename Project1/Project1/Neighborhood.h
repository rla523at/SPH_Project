#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

//abbreviation
namespace ms
{
using DirectX::SimpleMath::Vector3;
}

namespace ms
{
class Neighborhood
{
public:
  virtual void update(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors) = 0;

  virtual const std::vector<size_t>& search_for_fluid(const size_t fpid) const = 0;
  virtual const std::vector<size_t>& search_for_boundary(const size_t bpid) const = 0;
};
} // namespace ms
