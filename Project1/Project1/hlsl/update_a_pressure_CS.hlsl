#define NUM_THREAD 512

#include "Neighbor_Information.hlsli"

cbuffer CB : register(b0)
{
  float g_m0;
  uint  g_num_fp;
  uint  g_estimated_num_nfp;
};

StructuredBuffer<Neighbor_Information>  ninfo_buffer            : register(t0);
StructuredBuffer<uint>                  ncount_buffer           : register(t1);
StructuredBuffer<float3>                v_grad_W_buffer         : register(t2);
StructuredBuffer<float>                 a_pressure_coeff_buffer : register(t3);

RWStructuredBuffer<float3> v_a_pressure_buffer : register(u0);

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;
  
  float3 v_a_pressure = float3(0.0, 0.0, 0.0);

  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i< num_nfp; ++i)
  {
    const uint index1     = start_index + i;
    const uint nbr_fp_index  = ninfo_buffer[index1].nbr_fp_index;

    if (fp_index == nbr_fp_index)
      continue;

    const float d = ninfo_buffer[index1].distance;
    
    if (d == 0.0)
      continue;

    const float3  v_grad_kernel   = v_grad_W_buffer[index1];
    const float   coeff           = a_pressure_coeff_buffer[index1];
    const float3  v_grad_pressure = coeff * v_grad_kernel;

    v_a_pressure += v_grad_pressure;
  }

  v_a_pressure *= -g_m0;

  v_a_pressure_buffer[fp_index] = v_a_pressure;
}