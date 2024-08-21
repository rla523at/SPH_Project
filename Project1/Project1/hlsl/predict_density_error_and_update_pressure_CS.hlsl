#define NUM_THREAD 32

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
  const float p = pressure_buffer[fp_index];

  density_buffer[fp_index]  = rho;
  pressure_buffer[fp_index] = max(0.0, p + scailing_factor_buffer[0] * denisty_error);  
}


//#define NUM_THREAD 128
//#define NUM_MAX_GROUP 65535
//#define N 2
//
//#include "cubic_spline_kernel.hlsli"
//#include "uniform_grid_output.hlsli"
//
//cbuffer CB : register(b1)
//{
//  float g_rho0;
//  float g_m0;
//  uint  g_num_fp;
//  uint  g_estimated_num_NFP;
//};
//
//StructuredBuffer<float3>                pos_buffer              : register(t0);
//StructuredBuffer<float>                 scailing_factor_buffer  : register(t1);
//StructuredBuffer<Neighbor_Information>  ninfo_buffer            : register(t2);
//StructuredBuffer<uint>                  ncount_buffer           : register(t3);
//
//RWStructuredBuffer<float> density_buffer        : register(u0);
//RWStructuredBuffer<float> pressure_buffer       : register(u1);
//
//groupshared float shared_sum_W[NUM_THREAD];
//
//[numthreads(NUM_THREAD,1,1)]
//void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
//{
//  const uint fp_index = Gid.x + Gid.y*NUM_MAX_GROUP; 
//
//  if (g_num_fp <= fp_index)
//    return;
//
//  const uint num_nfp = ncount_buffer[fp_index];
//
//  const float3 v_xi = pos_buffer[fp_index];
//
//  float sum_W = 0.0;
//
//  for (uint i=0; i<N; ++i)
//  {
//    const uint index = Gindex * N + i;
//
//    if (num_nfp <= index)
//        break;
//
//    const uint ninfo_index  = fp_index * g_estimated_num_NFP + index;
//    const uint nbr_fp_index = ninfo_buffer[ninfo_index].fp_index;
//
//    const float3  v_xj      = pos_buffer[nbr_fp_index];
//    const float   distance  = length(v_xi-v_xj);
//
//    sum_W += W(distance);
//  }
//
//  shared_sum_W[Gindex] = sum_W;
//
//  GroupMemoryBarrierWithGroupSync(); 
//
//  if (Gindex == 0)
//  {
//    float rho = 0.0;
//    
//    const uint n = min(NUM_THREAD, num_nfp);
//  
//    for (uint i=0; i <n; ++i)
//      rho += shared_sum_W[i];
//
//    rho *= g_m0;
//
//    const float denisty_error = rho - g_rho0;
//    const float p             = pressure_buffer[fp_index];
//
//    density_buffer[fp_index]  = rho;
//    pressure_buffer[fp_index] = max(0.0, p + scailing_factor_buffer[0] * denisty_error);  
//  }
//}