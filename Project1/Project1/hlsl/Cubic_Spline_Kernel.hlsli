cbuffer Cubic_Spline_Kernel_Constant : register(b0)
{
  float g_h;
  float g_coefficient;
};

static const float two_three = 2.0 / 3.0;
static const float one_six = 1.0 / 6.0;

float W(const float distance)
{  
  const float q = distance / g_h;
  
  if (q < 1.0f)
  {
    const float q2 = q * q;
    const float q3 = q2 * q;

    return g_coefficient * (two_three - q2 + 0.5f * q3);
  }
  else if (q < 2.0f)
    return g_coefficient * pow(2.0f - q, 3.0f) * one_six;
  else // q >= 2.0f
    return 0.0f;
}

float dWdq(const float distance)
{
  const float q = distance / g_h;
  
  if (q < 1.0f)
    return g_coefficient * (-2.0f * q + 1.5f * q * q);
  else if (q < 2.0f)
  {
    const float two_q = 2.0f - q;
    return g_coefficient * -0.5f * two_q * two_q;
  }
  else // q >= 2.0f
    return 0.0f;
}