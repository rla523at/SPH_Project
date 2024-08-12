#define NUM_THREAD 256

#include "Neighbor_Information.hlsli"

cbuffer CB : register(b0)
{
  uint  g_estimated_num_nfp;
  uint  g_num_fp;
  float g_regularization_term;
  
}

StructuredBuffer<Neighbor_Information>  ninfo_buffer    : register(t0);
StructuredBuffer<uint>                  ncount_buffer   : register(t1);
StructuredBuffer<float3>                velocity_buffer : register(t2);

RWStructuredBuffer<float> laplacian_vel_coeff_buffer : register(u0);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;

  const float3 v_vi = velocity_buffer[fp_index];

  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i = 0; i < num_nfp; ++i)
  {
    const uint index1       = start_index + i;
    const uint nbr_fp_index = ninfo_buffer[index1].nbr_fp_index;

    if (nbr_fp_index <= fp_index)
      continue;

    const float d = ninfo_buffer[index1].distance;

    if (d == 0.0f) 
      continue;

    const float3 v_vj   = velocity_buffer[nbr_fp_index];    
    const float3 v_xij  = ninfo_buffer[index1].v_xij;
    const float  d2     = ninfo_buffer[index1].distnace2;    
    const float3 v_vij  = v_vi - v_vj;
    
    const float coeff = dot(v_vij, v_xij) / (d2 + g_regularization_term);

    const uint index2 = nbr_fp_index * g_estimated_num_nfp + ninfo_buffer[index1].neighbor_index;
    
    laplacian_vel_coeff_buffer[index1] = coeff;
    laplacian_vel_coeff_buffer[index2] = coeff;    
  }
}