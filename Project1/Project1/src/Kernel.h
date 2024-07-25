#pragma once

namespace ms
{

class Cubic_Spline_Kernel
{
public:
  Cubic_Spline_Kernel(const float smoothing_length);

public:
  float W(const float distance) const;
  float dWdq(const float distance) const;
  float supprot_length(void) const;

private:
  float _h           = 0.0f;
  float _coefficient = 0.0f;
};

} // namespace ms
