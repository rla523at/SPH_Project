#include "Cubic_Spline_Kernel.hlsli"

#define NUM_THREAD 256

struct Neighbor_Information
{
  uint    fp_index;
  float3  tvector;
  float   distance;
};

cbuffer Constant_Data : register(b1)
{
  float g_beta;
  uint  g_estimated_num_nfp;  
};

StructuredBuffer<uint>                  max_index_buffer  : register(t0);
StructuredBuffer<Neighbor_Information>  nfp_info_buffer   : register(t1);
StructuredBuffer<uint>                  nfp_count_buffer  : register(t2);

RWStructuredBuffer<uint> scaling_factor_buffer : register(u0);

groupshared float  shared_sum_grad_kernel[NUM_THREAD];
groupshared float  shared_sum_grad_kernel_size[NUM_THREAD];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  const uint fp_index     = max_index_buffer[0];
  const uint start_index  = fp_index * g_estimated_num_nfp;
  const uint num_nfp      = nfp_count_buffer[fp_index];

  // assume num nfp < NUM_THREAD(256)
  float3 v_grad_kernel = float3(0,0,0);

  if (DTid.x < num_nfp)
  {
    const uint nfp_index = start_index + DTid.x;

    const Neighbor_Information ninfo = nfp_info_buffer[nfp_index];

    const float distance = ninfo.distance;

    if (distnace != 0) 
    {
      const float3 v_xij          = ninfo.tvector;    
      const float3 v_grad_q       = 1.0 / (g_h * distance) * v_xij;
      
      v_grad_kernel  = dWdq(distance) * v_grad_q;
    }
  }
  
  shared_sum_grad_kernel[DTid.x]      = v_grad_kernel;
  shared_sum_grad_kernel_size[DTid.x] = v_grad_kernel.dot(v_grad_kernel);

  GroupMemoryBarrierWithGroupSync();

  for (uint stride = NUM_THREAD/2; 0 < stride; stride /= 2)
  {
    if (DTid.x < stride)
    {
      const uint index2 = DTid.x + stride;
      shared_sum_grad_kernel[DTid.x] += shared_sum_grad_kernel[index2];
      shared_sum_grad_kernel_size[DTid.x] += shared_sum_grad_kernel_size[index2];
    }

    GroupMemoryBarrierWithGroupSync();
  }

  if (DTid.x == 0)
  {
    const float v_sum_grad_kernel_size  = shared_sum_grad_kernel_size[0];
    const float v_sum_grad_kernel       = shared_sum_grad_kernel[0];
    const float sum_dot_sum             = v_sum_grad_kernel.dot(v_sum_grad_kernel);
 
    scaling_factor_buffer[0] = 1.0 / (g_beta * (sum_dot_sum + v_sum_grad_kernel_size));   
  }  
}