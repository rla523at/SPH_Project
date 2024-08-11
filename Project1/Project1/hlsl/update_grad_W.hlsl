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

RWStructuredBuffer<float> grad_W_buffer : register(u0);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index = DTid.x;
  const uint start_index = fp_index * g_estimated_num_nfp;

  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i = 0; i < num_nfp; ++i)
  {
    const uint index1 = start_index + i;
    const uint nbr_fp_index = ninfo_buffer[index1].nbr_fp_index;

    if (nbr_fp_index < fp_index)
      continue;   
    
    const float d = ninfo_buffer[index1].distance;
    
    if (d == 0.0f) 
      continue;

    const float3 v_xij    = ninfo_buffer[index1].v_xij;      
    const float3 v_grad_q = 1.0f / (g_h * d) * v_xij;
    const float3 v_grad_W = dWdq(d) * v_grad_q;

    const uint index2 = nbr_fp_index * g_estimated_num_nfp + ninfo_buffer[index1].neighbor_index;
    
    grad_W_buffer[index1] = v_grad_W;
    grad_W_buffer[index2] = -v_grad_W;
  }
}