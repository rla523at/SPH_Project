#pragma once
namespace ms
{
struct Domain
{
  float x_start = 0.0f;
  float x_end   = 0.0f;
  float y_start = 0.0f;
  float y_end   = 0.0f;
  float z_start = 0.0f;
  float z_end   = 0.0f;

  float dx(void) const
  {
    return x_end - x_start;
  }
  float dy(void) const
  {
    return y_end - y_start;
  }
  float dz(void) const
  {
    return z_end - z_start;
  }
};
} // namespace ms