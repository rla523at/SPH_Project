#define NUM_THREAD 128
#define NUM_MAX_GROUP 65535
#define N 2

#include "uniform_grid_output.hlsli"
#include "Cubic_Spline_Kernel.hlsli"

cbuffer Constant_Buffer : register(b1)
{
  uint  g_estimated_num_NFP;
  uint  g_num_FP;
};

StructuredBuffer<Neighbor_Information>  ninfo_buffer   : register(t0);
StructuredBuffer<uint>                  ncount_buffer  : register(t1);

RWStructuredBuffer<float> result_buffer : register(u0);

groupshared float shared_sum_W[NUM_THREAD];

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint fp_index = Gid.x + Gid.y*NUM_MAX_GROUP; 

  if(g_num_FP <= fp_index) 
    return;

  const uint start_index  = fp_index * g_estimated_num_NFP;

  float sum_W = 0.0;

  const uint num_nfp = ncount_buffer[fp_index];

  for (uint i=0; i<N; ++i)
  {
    const uint index = Gindex * N + i;

    if (num_nfp <= index)
        break;

    const uint ninfo_index  = fp_index * g_estimated_num_NFP + index;

    const Neighbor_Information Ninfo = ninfo_buffer[ninfo_index];

    sum_W += W(Ninfo.distance);
  }

  shared_sum_W[Gindex] = sum_W;

  GroupMemoryBarrierWithGroupSync(); 

  if (Gindex == 0)
  {
    float number_density = 0;
    
    const uint n = min(NUM_THREAD, num_nfp);
  
    for (uint i=0; i <n; ++i)
      number_density += shared_sum_W[i];

    result_buffer[fp_index] = number_density;
  }
}