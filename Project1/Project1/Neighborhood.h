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
  virtual void update(const std::vector<Vector3>& pos_vectors) = 0;

  // fill neighbor particle ids into pids and return number of neighbor particles
  //virtual size_t search(const Vector3& pos, size_t* pids) const = 0;
  virtual std::vector<size_t> search(const Vector3& pos) const = 0;
};
} // namespace ms
