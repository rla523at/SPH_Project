#define NUM_THREAD 256

#include "uniform_grid_output.hlsli"
#include "cubic_spline_kernel.hlsli"

cbuffer CB : register(b1)
{
  float3  g_v_a_gravity;
  float   g_m0;
  float   g_viscosity_constant;
  float   g_regularization_term;
  uint    g_num_fluid_particle;
  uint    g_estimated_num_nfp;
};

StructuredBuffer<float>                 density_buffer      : register(t0);
StructuredBuffer<float3>                velocity_buffer     : register(t1);
StructuredBuffer<Neighbor_Information>  ninfo_buffer        : register(t2);
StructuredBuffer<uint>                  ninfo_count_buffer  : register(t3);

RWStructuredBuffer<float3> acceleration_buffer : register(u0);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fluid_particle <= DTid.x)
    return;

  float3 v_a_viscosity = float3(0,0,0);

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;

  const float   rhoi = density_buffer[fp_index];
  const float3  v_vi = velocity_buffer[fp_index];

  const uint num_nfp  = ninfo_count_buffer[fp_index];
  for (uint i=0; i<num_nfp; ++i)
  {
    const uint ninfo_index  = start_index + i;
    const uint nfp_index    = ninfo_buffer[ninfo_index].fp_index;

    if (fp_index == nfp_index)
      continue;

    const float   rhoj = density_buffer[nfp_index];
    const float3  v_vj = velocity_buffer[nfp_index];
    
    const float3  v_xij     = ninfo_buffer[ninfo_index].tvector;
    const float   distance  = ninfo_buffer[ninfo_index].distance;
    const float   distance2 = distance * distance;

    if (distance == 0.0f) 
      continue;

    const float3 v_grad_q       = 1.0f / (g_h * distance) * v_xij;
    const float3 v_grad_kernel  = dWdq(distance) * v_grad_q;

    const float3 v_vij = v_vi - v_vj;
    const float  coeff = dot(v_vij, v_xij) / (rhoj * (distance2 + g_regularization_term));

    const float3 v_laplacian_velocity = coeff * v_grad_kernel;

    v_a_viscosity += v_laplacian_velocity;
  }

  acceleration_buffer[fp_index] = g_viscosity_constant * v_a_viscosity + g_v_a_gravity;
}