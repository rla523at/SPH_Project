#define NUM_THREAD 128
#define NUM_MAX_GROUP 65535
#define N 2

#include "uniform_grid_output.hlsli"
#include "cubic_spline_kernel.hlsli"

cbuffer CB : register(b1)
{
  float3  g_v_a_gravity;
  float   g_m0;
  float   g_viscosity_constant;
  float   g_regularization_term;
  uint    g_num_FP;
  uint    g_estimated_num_NFP;
};

StructuredBuffer<float>                 density_buffer      : register(t0);
StructuredBuffer<float3>                velocity_buffer     : register(t1);
StructuredBuffer<Neighbor_Information>  ninfo_buffer        : register(t2);
StructuredBuffer<uint>                  ninfo_count_buffer  : register(t3);

RWStructuredBuffer<float3> acceleration_buffer : register(u0);

groupshared float3 shared_v_laplacian_vel[NUM_THREAD];

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint fp_index = Gid.x + Gid.y*NUM_MAX_GROUP; 

  if (g_num_FP <= fp_index)
    return;  

  const uint num_nfp = ninfo_count_buffer[fp_index];
  
  const float   rhoi = density_buffer[fp_index];
  const float3  v_vi = velocity_buffer[fp_index];

  float3 v_laplacian_vel = float3(0,0,0);

  for (uint i=0; i<N; ++i)
  {
    const uint index = Gindex * N + i;

    if (num_nfp <= index)
        break;

    const uint ninfo_index  = fp_index * g_estimated_num_NFP + index;

    const Neighbor_Information Ninfo = ninfo_buffer[ninfo_index];

    const uint nbr_fp_index = Ninfo.fp_index;

    const float   rhoj = density_buffer[nbr_fp_index];
    const float3  v_vj = velocity_buffer[nbr_fp_index];
    
    const float3  v_xij     = Ninfo.tvector;
    const float   distance  = Ninfo.distance;
    const float   distance2 = distance * distance;

    if (distance == 0.0f) 
      continue;

    const float3 v_grad_q  = 1.0f / (g_h * distance) * v_xij;
    const float3 v_grad_W  = dWdq(distance) * v_grad_q;

    const float3 v_vij = v_vi - v_vj;
    const float  coeff = dot(v_vij, v_xij) / (rhoj * (distance2 + g_regularization_term));

    v_laplacian_vel += coeff * v_grad_W;
  }

  shared_v_laplacian_vel[Gindex] = v_laplacian_vel;

  GroupMemoryBarrierWithGroupSync(); 

  if (Gindex == 0)
  {
    float3 v_a_viscosity = float3(0,0,0);
    
    const uint n = min(NUM_THREAD, num_nfp);
  
    for (uint i=0; i <n; ++i)
      v_a_viscosity += shared_v_laplacian_vel[i];

    acceleration_buffer[fp_index] = g_viscosity_constant * v_a_viscosity + g_v_a_gravity;
  }

}