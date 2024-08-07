#define NUM_THREAD 256

#include "cubic_spline_kernel.hlsli"
#include "uniform_grid_output.hlsli"

cbuffer CB : register(b1)
{
  float g_m0;
  uint  g_num_fp;
  uint  g_estimated_num_nfp;
};

////debug
//  struct debug_struct
//  {
//    float   coeff;
//    float3 v_grad_pressure;
//  };
////debug

StructuredBuffer<float>                 density_buffer  : register(t0);
StructuredBuffer<float>                 pressure_buffer : register(t1);
StructuredBuffer<float3>                v_pos_buffer    : register(t2);
StructuredBuffer<Neighbor_Information>  ninfo_buffer    : register(t3);
StructuredBuffer<uint>                  ncount_buffer   : register(t4);

RWStructuredBuffer<float3> v_a_pressure_buffer : register(u0);
//RWStructuredBuffer<debug_struct> debug_buffer : register(u1);//debug

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;
  
  const float   rhoi  = density_buffer[fp_index];
  const float   pi    = pressure_buffer[fp_index];
  const float3  v_xi  = v_pos_buffer[fp_index];

  float3 v_a_pressure = float3(0.0, 0.0, 0.0);

  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i< num_nfp; ++i)
  {
    const uint ninfo_index  = start_index + i;
    const uint nfp_index    = ninfo_buffer[ninfo_index].fp_index;

    if (fp_index == nfp_index)
      continue;

    const float   rhoj  = density_buffer[nfp_index];
    const float   pj    = pressure_buffer[nfp_index];
    const float3  v_xj  = v_pos_buffer[nfp_index];

    const float3  v_xij = v_xi - v_xj;
    const float   distance = length(v_xij);
    
    if (distance == 0.0)
      continue;

    const float3 v_grad_q       = 1.0f / (g_h * distance) * v_xij;
    const float3 v_grad_kernel  = dWdq(distance) * v_grad_q;

    const float   coeff           = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
    const float3  v_grad_pressure = coeff * v_grad_kernel;

    v_a_pressure += v_grad_pressure;

    //debug_buffer[ninfo_index].coeff = coeff;//debug
    //debug_buffer[ninfo_index].v_grad_pressure = v_grad_pressure;//debug
  }

  v_a_pressure *= -g_m0;

  v_a_pressure_buffer[fp_index] = v_a_pressure;
}