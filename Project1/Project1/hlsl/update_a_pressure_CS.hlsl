#define NUM_THREAD 256
#define NUM_MAX_GROUP 65535

#include "cubic_spline_kernel.hlsli"
#include "uniform_grid_output.hlsli"

cbuffer CB : register(b1)
{
  float g_m0;
  uint  g_num_fp;
  uint  g_estimated_num_nfp;
};

StructuredBuffer<float>                 density_buffer  : register(t0);
StructuredBuffer<float>                 pressure_buffer : register(t1);
StructuredBuffer<float3>                v_pos_buffer    : register(t2);
StructuredBuffer<Neighbor_Information>  ninfo_buffer    : register(t3);
StructuredBuffer<uint>                  ncount_buffer   : register(t4);

RWStructuredBuffer<float3> v_a_pressure_buffer : register(u0);

groupshared float3 shared_v_grad_pressure[NUM_THREAD];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint fp_index = Gid.x + Gid.y*NUM_MAX_GROUP; 

  if (g_num_fp <= fp_index)
    return;

  const uint num_nfp = ncount_buffer[fp_index];
  
  float3 v_grad_pressure = float3(0,0,0);

  if (Gindex < num_nfp)
  {
    const uint ninfo_index  = fp_index * g_estimated_num_nfp + Gindex;
    const uint nfp_index    = ninfo_buffer[ninfo_index].fp_index;
    
    const float   rhoi  = density_buffer[fp_index];
    const float   pi    = pressure_buffer[fp_index];
    const float3  v_xi  = v_pos_buffer[fp_index];

    const float   rhoj  = density_buffer[nfp_index];
    const float   pj    = pressure_buffer[nfp_index];
    const float3  v_xj  = v_pos_buffer[nfp_index];

    const float3  v_xij = v_xi - v_xj;
    const float   distance = length(v_xij);
    
    if (distance != 0.0)
    {
      const float3 v_grad_q       = 1.0f / (g_h * distance) * v_xij;
      const float3 v_grad_kernel  = dWdq(distance) * v_grad_q;
      const float  coeff          = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
      
      v_grad_pressure = coeff * v_grad_kernel;
    }
  }

  shared_v_grad_pressure[Gindex] = v_grad_pressure;

  GroupMemoryBarrierWithGroupSync();

  if (Gindex == 0)
  {
    float3 v_a_pressure = float3(0,0,0);
    
    for (uint i=0; i <num_nfp; ++i)
      v_a_pressure += shared_v_grad_pressure[i];
  
    v_a_pressure_buffer[fp_index] = -g_m0 * v_a_pressure; 
  }    
}