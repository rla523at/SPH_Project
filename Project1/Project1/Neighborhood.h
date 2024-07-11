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

  virtual const std::vector<size_t>& search(const size_t pid) const = 0;
};
} // namespace ms
