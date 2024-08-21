#define NUM_THREAD 64
#define NUM_MAX_GROUP 65535
#define NUM_FP_CHUNK_MAX 64
#define N 4

#include "cubic_spline_kernel.hlsli"
#include "uniform_grid_output.hlsli"

struct Local_Nbr_Sum
{
  uint nbr_sum[NUM_FP_CHUNK_MAX];
};

cbuffer CB : register(b1)
{
 float g_rho0;
 float g_m0;
 uint  g_num_fp;
 uint  g_estimated_num_NFP;
};

StructuredBuffer<float3>                pos_buffer              : register(t0);
StructuredBuffer<float>                 scailing_factor_buffer  : register(t1);
StructuredBuffer<Neighbor_Information>  ninfo_buffer            : register(t2);
StructuredBuffer<uint>                  ncount_buffer           : register(t3);
StructuredBuffer<uint>                  nbr_chunk_buffer        : register(t4);
StructuredBuffer<uint>                  nbr_chunk_count_buffer  : register(t5);

RWStructuredBuffer<float> density_buffer        : register(u0);
RWStructuredBuffer<float> pressure_buffer       : register(u1);

groupshared float shared_W[256];
groupshared uint  shared_nbr_sum[NUM_FP_CHUNK_MAX];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint chunk_index = Gid.x + Gid.y*NUM_MAX_GROUP; 
  
  if (nbr_chunk_count_buffer[0] <= chunk_index)
    return;
  
  //chunk에 속한 FP_index
  const uint FP_index_begin = (chunk_index == 0) ? 0 : nbr_chunk_buffer[chunk_index-1];
  const uint FP_index_end   = nbr_chunk_buffer[chunk_index];
  const uint num_FP_chunk   = FP_index_end - FP_index_begin;

  //chunk별로 nbr sum이 있다.
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
    const uint nbr_FP_index = ninfo_buffer[ninfo_index].fp_index;
    
    const float3  v_xi      = pos_buffer[FP_index[i]];
    const float3  v_xj      = pos_buffer[nbr_FP_index];
    const float   distance  = length(v_xi-v_xj);

    shared_W[N * Gindex + i] = W(distance);
  }
    
  GroupMemoryBarrierWithGroupSync(); 
  
  if (Gindex < num_FP_chunk)
  {    
    const uint nbr_index_begin = (Gindex == 0) ? 0 : shared_nbr_sum[Gindex-1];
    const uint nbr_index_end   = shared_nbr_sum[Gindex];
    
    float sum_W = 0.0;
    for (uint i = nbr_index_begin; i < nbr_index_end; ++i)
      sum_W += shared_W[i];
    
    const float rho      = sum_W * g_m0;
    const float rho_err  = rho - g_rho0;
    
    const uint  FP_index2 = FP_index_begin + Gindex;
    const float p         = pressure_buffer[FP_index2];
    
    density_buffer[FP_index2]  = rho;
    pressure_buffer[FP_index2] = max(0.0, p + scailing_factor_buffer[0] * rho_err);  
  }
}