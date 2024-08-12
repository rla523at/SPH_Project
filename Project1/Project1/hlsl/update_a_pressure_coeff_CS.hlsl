#define NUM_THREAD 256

#include "Neighbor_Information.hlsli"

cbuffer CB : register(b0)
{
  uint  g_estimated_num_nfp;
  uint  g_num_fp;
};

StructuredBuffer<float>                 density_buffer  : register(t0);
StructuredBuffer<float>                 pressure_buffer : register(t1);
StructuredBuffer<Neighbor_Information>  ninfo_buffer    : register(t2);
StructuredBuffer<uint>                  ncount_buffer   : register(t3);

RWStructuredBuffer<float> a_pressure_coeff_buffer : register(u0);

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;
  
  const float   rhoi  = density_buffer[fp_index];
  const float   pi    = pressure_buffer[fp_index];

  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i< num_nfp; ++i)
  {
    const uint index1       = start_index + i;
    const uint nbr_fp_index = ninfo_buffer[index1].nbr_fp_index;

    if (nbr_fp_index <= fp_index )
      continue;

    const float   rhoj  = density_buffer[nbr_fp_index];
    const float   pj    = pressure_buffer[nbr_fp_index];
    const float   coeff = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));

    const uint index2 = nbr_fp_index * g_estimated_num_nfp + ninfo_buffer[index1].neighbor_index;

    a_pressure_coeff_buffer[index1] = coeff;
    a_pressure_coeff_buffer[index2] = coeff;
  }
}