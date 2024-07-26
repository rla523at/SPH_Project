#include "Debugger.h"

namespace ms
{
template <>
void print(const Vector3& v)
{
  std::cout << v.x << " " << v.y << " " << v.z << "\n";
}

bool is_nan(const Vector3& v)
{
  return v.x != v.x || v.y != v.y || v.z != v.z;
}

}