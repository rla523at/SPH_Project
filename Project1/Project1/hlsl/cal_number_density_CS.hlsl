#define NUM_THREAD 256

#include "Cubic_Spline_Kernel.hlsli"

struct Neighbor_Information
{
  uint    fp_index;
  float3  tvector;
  float   distance;
};

cbuffer Constant_Buffer : register(b1)
{
  uint  g_estimated_num_nfp;
};

StructuredBuffer<Neighbor_Information>  nfp_info_buffer   : register(t0);
StructuredBuffer<uint>                  nfp_count_buffer  : register(t1);

RWStructuredBuffer<float> result_buffer : register(u0);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;

  float number_density = 0.0;

  const uint num_nfp = nfp_count_buffer[fp_index];
  for (uint i=0; i<num_nfp; ++i)
  {
    const float distance = nfp_info_buffer[start_index + i].distance;
    number_density += W(distance);
  }

  result_buffer[fp_index] = number_density;
}