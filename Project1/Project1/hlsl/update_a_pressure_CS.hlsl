#define NUM_THREAD 128
#define NUM_MAX_GROUP 65535
#define NUM_FP_CHUNK_MAX 64
#define N 2

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
StructuredBuffer<uint>                  nbr_chunk_buffer        : register(t5);
StructuredBuffer<uint>                  nbr_chunk_count_buffer  : register(t6);

RWStructuredBuffer<float3> v_a_pressure_buffer : register(u0);

groupshared float3  shared_v_grad_pressure[256];
groupshared uint    shared_nbr_sum[NUM_FP_CHUNK_MAX];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint chunk_index = Gid.x + Gid.y*NUM_MAX_GROUP; 
  
  if (nbr_chunk_count_buffer[0] <= chunk_index)
    return;
  
  //chunk�� ���� FP_index
  const uint FP_index_begin = (chunk_index == 0) ? 0 : nbr_chunk_buffer[chunk_index-1];
  const uint FP_index_end   = nbr_chunk_buffer[chunk_index];
  const uint num_FP_chunk   = FP_index_end - FP_index_begin;
  
  //chunk���� nbr sum�� �ִ�.
  //cal nbr sum
  if (Gindex == 0)
  {
    shared_nbr_sum[0] = ncount_buffer[FP_index_begin];
  
    for (uint i=1; i<num_FP_chunk; ++i)
      shared_nbr_sum[i] = shared_nbr_sum[i-1] + ncount_buffer[FP_index_begin +i];
  }
    
  GroupMemoryBarrierWithGroupSync();

  // Thread���� ���� � FP_index�� � nbr_index�� ����ϴ��� �˾ƾ� ��
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

  //���
  for (uint i=0; i<N; ++i)
  {
    const float   rhoi  = density_buffer[FP_index[i]];
    const float   pi    = pressure_buffer[FP_index[i]];
    const float3  v_xi  = v_pos_buffer[FP_index[i]];
  
    const uint ninfo_index  = FP_index[i] * g_estimated_num_nfp + nbr_index[i];
    const uint nbr_fp_index = ninfo_buffer[ninfo_index].fp_index;  
  
    const float   rhoj  = density_buffer[nbr_fp_index];
    const float   pj    = pressure_buffer[nbr_fp_index];
    const float3  v_xj  = v_pos_buffer[nbr_fp_index];
  
    const float3  v_xij = v_xi - v_xj;
    const float   distance = length(v_xij);
    
    float3 v_grad_pressure = float3(0,0,0);
    if (distance != 0.0)
    {
      const float3 v_grad_q       = 1.0f / (g_h * distance) * v_xij;
      const float3 v_grad_kernel  = dWdq(distance) * v_grad_q;
      const float  coeff          = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
      
       v_grad_pressure = coeff * v_grad_kernel;
    } 
  
    shared_v_grad_pressure[N * Gindex + i] = v_grad_pressure;
  }  
  
  GroupMemoryBarrierWithGroupSync(); 

  if (Gindex < num_FP_chunk)
  {    
    const uint nbr_index_begin = (Gindex == 0) ? 0 : shared_nbr_sum[Gindex-1];
    const uint nbr_index_end   = shared_nbr_sum[Gindex];
    
    float3 sum_v_grad_pressure = 0.0;
    for (uint i = nbr_index_begin; i < nbr_index_end; ++i)
      sum_v_grad_pressure += shared_v_grad_pressure[i];
    
    const uint  FP_index2 = FP_index_begin + Gindex;
    
    v_a_pressure_buffer[FP_index2] = -g_m0 * sum_v_grad_pressure;
  }
}

