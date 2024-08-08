#define NUM_THREAD 256

#include "cubic_spline_kernel.hlsli"
#include "uniform_grid_output.hlsli"

cbuffer CB : register(b1)
{
  float g_rho0;
  float g_m0;
  uint  g_num_fp;
  uint  g_estimated_num_nfp;
};

StructuredBuffer<float3>                pos_buffer              : register(t0);
StructuredBuffer<float>                 scailing_factor_buffer  : register(t1);
StructuredBuffer<Neighbor_Information>  ninfo_buffer            : register(t2);
StructuredBuffer<uint>                  ncount_buffer           : register(t3);

RWStructuredBuffer<float> density_buffer        : register(u0);
RWStructuredBuffer<float> pressure_buffer       : register(u1);
RWStructuredBuffer<float> density_error_buffer  : register(u2);


[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;  

  const float3 v_xi = pos_buffer[fp_index];

  float rho = 0.0;
  
  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i<num_nfp; ++i)
  {
    const uint ninfo_index  = start_index + i;
    const uint nfp_index    = ninfo_buffer[ninfo_index].fp_index;

    const float3  v_xj      = pos_buffer[nfp_index];
    const float   distance  = length(v_xi-v_xj);

    rho += W(distance);
  }

  rho *= g_m0;
  
  const float denisty_error = rho - g_rho0;

  density_buffer[fp_index]        = rho;
  density_error_buffer[fp_index]  = denisty_error;  
  pressure_buffer[fp_index]       +=  scailing_factor_buffer[0] * denisty_error;  
  pressure_buffer[fp_index]       = max(0.0, pressure_buffer[fp_index]);  
}