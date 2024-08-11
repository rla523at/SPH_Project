#define NUM_THREAD 256

#include "cubic_spline_kernel.hlsli"
#include "Neighbor_Information.hlsli"

cbuffer CB : register(b1)
{
  float3  g_v_a_gravity;
  float   g_m0;
  float   g_viscosity_constant;
  float   g_regularization_term;
  uint    g_num_fluid_particle;
  uint    g_estimated_num_nfp;
};

StructuredBuffer<float>                 density_buffer              : register(t0);
StructuredBuffer<Neighbor_Information>  ninfo_buffer                : register(t2);
StructuredBuffer<uint>                  ninfo_count_buffer          : register(t3);
StructuredBuffer<float3>                grad_W_buffer               : register(t4);
StructuredBuffer<float>                 laplacian_vel_coeff_buffer  : register(t5);

RWStructuredBuffer<float3> acceleration_buffer : register(u0);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fluid_particle <= DTid.x)
    return;

  float3 v_a_viscosity = float3(0,0,0);

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;

  const uint num_nfp  = ninfo_count_buffer[fp_index];
  for (uint i=0; i<num_nfp; ++i)
  {
    const uint index1       = start_index + i;
    const uint nbr_fp_index = ninfo_buffer[index1].nbr_fp_index;

    if (fp_index == nbr_fp_index)
      continue;

    const float d = ninfo_buffer[index1].distance;

    if (d == 0.0f) 
      continue;
    
    const float  rhoj                 = density_buffer[nbr_fp_index];
    const float  coeff                = laplacian_vel_coeff_buffer[index1];
    const float3 v_grad_kernel        = grad_W_buffer[index1];
    const float3 v_laplacian_velocity = coeff / rhoj * v_grad_kernel;

    v_a_viscosity += v_laplacian_velocity;
  }

  acceleration_buffer[fp_index] = g_viscosity_constant * v_a_viscosity + g_v_a_gravity;
}