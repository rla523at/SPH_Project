#include "Kernel.h"

#include "../../_lib/_header/msexception/Exception.h"
#include <numbers>

namespace ms
{
Cubic_Spline_Kernel::Cubic_Spline_Kernel(const float smoothing_length)
    : _h(smoothing_length)
{
  constexpr float pi = std::numbers::pi_v<float>;

  _coefficient = 3.0f / (2.0f * pi * _h * _h * _h);
}

float Cubic_Spline_Kernel::W(const float distance) const
{
  constexpr float c1 = 2.0f / 3.0f;
  constexpr float c2 = 1.0f / 6.0f;

  REQUIRE(distance >= 0.0f, "dist should be positive");

  const float q = distance / _h;

  if (q < 1.0f)
  {
    const float q2 = q * q;
    const float q3 = q2 * q;

    return _coefficient * (c1 - q2 + 0.5f * q3);  
  }
  else if (q < 2.0f)
    return _coefficient * std::pow(2.0f - q, 3.0f) * c2;
  else // q >= 2.0f
    return 0.0f;
}

float Cubic_Spline_Kernel::dWdq(const float distance) const
{
  REQUIRE(distance >= 0.0f, "dist should be positive");

  const float q = distance / _h;

  if (q < 1.0f)
    return _coefficient * (-2.0f * q + 1.5f * q * q);
  else if (q < 2.0f)
  {
    const float two_q = 2.0f - q;
    return _coefficient * -0.5f * two_q * two_q;
  }
  else // q >= 2.0f
    return 0.0f;
}

float Cubic_Spline_Kernel::supprot_length(void) const
{
  return 2.0f * _h;
}

} // namespace ms
