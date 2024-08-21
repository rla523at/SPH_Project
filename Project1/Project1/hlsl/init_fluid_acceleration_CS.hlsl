#define NUM_THREAD 128
#define NUM_MAX_GROUP 65535
#define NUM_FP_CHUNK_MAX 64
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

StructuredBuffer<float>                 density_buffer          : register(t0);
StructuredBuffer<float3>                velocity_buffer         : register(t1);
StructuredBuffer<Neighbor_Information>  ninfo_buffer            : register(t2);
StructuredBuffer<uint>                  ncount_buffer           : register(t3);
StructuredBuffer<uint>                  nbr_chunk_buffer        : register(t4);
StructuredBuffer<uint>                  nbr_chunk_count_buffer  : register(t5);

RWStructuredBuffer<float3> acceleration_buffer : register(u0);

groupshared float3 shared_v_laplacian_vel[256];
groupshared uint   shared_nbr_sum[NUM_FP_CHUNK_MAX];

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint chunk_index = Gid.x + Gid.y*NUM_MAX_GROUP; 
  
  if (nbr_chunk_count_buffer[0] <= chunk_index)
    return;
  
  //chunk에 속한 FP_index
  const uint FP_index_begin = (chunk_index == 0) ? 0 : nbr_chunk_buffer[chunk_index-1];
  const uint FP_index_end   = nbr_chunk_buffer[chunk_index];
  const uint num_FP_chunk   = FP_index_end - FP_index_begin;

  //cal nbr sum
  if (Gindex == 0)
  {
    shared_nbr_sum[0] = ncount_buffer[FP_index_begin];

    for (uint i=1; i<num_FP_chunk; ++i)
      shared_nbr_sum[i] = shared_nbr_sum[i-1] + ncount_buffer[FP_index_begin +i];
  }
    
  GroupMemoryBarrierWithGroupSync();

  // Thread별로 내가 어떤 FP_index에 어떤 nbr_index를 계산하는지 알아야 됨
  uint FP_index[N];
  uint nbr_index[N];
  
  uint nbr_sum_prev = 0;
  for (uint i=0; i<N; ++i)
  {
    const uint chunk_neighbor_index = N * Gindex + i;

    for (uint j=0; j<num_FP_chunk; ++j)
    { 
      FP_index[i] = FP_index_begin + j;    
  
      if (chunk_neighbor_index < shared_nbr_sum[j])
      {
        nbr_index[i] = chunk_neighbor_index - nbr_sum_prev;    
        break;
      }      
  
      nbr_sum_prev  = shared_nbr_sum[j];    
    }   
  }

  //계산
  for (uint i=0; i<N; ++i)
  {
    const uint ninfo_index  = FP_index[i] * g_estimated_num_NFP + nbr_index[i];

    const Neighbor_Information Ninfo = ninfo_buffer[ninfo_index];
    
    const uint    nbr_fp_index  = Ninfo.fp_index;
    const float3  v_xij         = Ninfo.tvector;
    const float   distance      = Ninfo.distance;
    const float   distance2     = distance * distance;

    const float3 v_vi = velocity_buffer[FP_index[i]];

    const float  rhoj = density_buffer[nbr_fp_index];
    const float3 v_vj = velocity_buffer[nbr_fp_index];

    float3 v_laplacian_vel = float3(0,0,0);
    if (distance != 0.0f) 
    {
      const float3 v_grad_q  = 1.0f / (g_h * distance) * v_xij;
      const float3 v_grad_W  = dWdq(distance) * v_grad_q;

      const float3 v_vij = v_vi - v_vj;
      const float  coeff = dot(v_vij, v_xij) / (rhoj * (distance2 + g_regularization_term));

      v_laplacian_vel = coeff * v_grad_W;
    }

    shared_v_laplacian_vel[N * Gindex + i] = v_laplacian_vel;
  }

  GroupMemoryBarrierWithGroupSync(); 

  if (Gindex < num_FP_chunk)
  {    
    const uint nbr_index_begin = (Gindex == 0) ? 0 : shared_nbr_sum[Gindex-1];
    const uint nbr_index_end   = shared_nbr_sum[Gindex];
    
    float3 sum_v_laplacian_vel = 0.0;
    for (uint i = nbr_index_begin; i < nbr_index_end; ++i)
      sum_v_laplacian_vel += shared_v_laplacian_vel[i];
    
    const uint  FP_index2 = FP_index_begin + Gindex;
    
    acceleration_buffer[FP_index2] = g_viscosity_constant * sum_v_laplacian_vel + g_v_a_gravity;
  }
}