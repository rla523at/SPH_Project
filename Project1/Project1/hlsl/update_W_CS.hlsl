#define NUM_THREAD 256

#include "Neighbor_Information.hlsli"
#include "cubic_spline_kernel.hlsli"

cbuffer CB : register(b1)
{
  uint g_estimated_num_nfp;
  uint g_num_fp;
}

StructuredBuffer<Neighbor_Information>  ninfo_buffer  : register(t0);
StructuredBuffer<uint>                  ncount_buffer : register(t1);

RWStructuredBuffer<float> W_buffer : register(u0);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;

  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i = 0; i < num_nfp; ++i)
  {
    const uint index1       = start_index + i;
    const uint nbr_fp_index = ninfo_buffer[index1].nbr_fp_index;    

    if (nbr_fp_index < fp_index)
      continue;
    
    const uint index2 = nbr_fp_index * g_estimated_num_nfp + ninfo_buffer[index1].neighbor_index;
    
    const float d = ninfo_buffer[index1].distance;
    const float W = W(d);
    
    W_buffer[index1] = W;
    W_buffer[index2] = W;    
  }
}