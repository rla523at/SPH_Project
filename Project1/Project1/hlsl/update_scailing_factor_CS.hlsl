#define NUM_THREAD 256

cbuffer Constant_Data : register(b0)
{
  uint  g_estimated_num_nfp;  
  float g_beta;
};

StructuredBuffer<uint>    max_index_buffer  : register(t0);
StructuredBuffer<uint>    nfp_count_buffer  : register(t1);
StructuredBuffer<float3>  grad_W_buffer     : register(t2);

RWStructuredBuffer<float> scailing_factor_buffer : register(u0);

struct debug_strcut
{
  float3 sum_grad_W;
  float  sum_grad_W_size;
  float   sum_dot_sum;
};

RWStructuredBuffer<debug_strcut> debug_buffer : register(u1);

groupshared float3  shared_sum_grad_W[NUM_THREAD];
groupshared float   shared_sum_grad_W_size[NUM_THREAD];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  const uint fp_index     = max_index_buffer[0];
  const uint start_index  = fp_index * g_estimated_num_nfp;
  const uint num_nfp      = nfp_count_buffer[fp_index];

  float3 v_grad_W = float3(0,0,0);
  
  if (DTid.x < num_nfp)
  {
    v_grad_W = grad_W_buffer[start_index + DTid.x];
  }

  shared_sum_grad_W[DTid.x]       = v_grad_W;
  shared_sum_grad_W_size[DTid.x]  = dot(v_grad_W, v_grad_W);

  //debug
  debug_buffer[DTid.x].sum_grad_W       = shared_sum_grad_W[DTid.x];
  debug_buffer[DTid.x].sum_grad_W_size  = shared_sum_grad_W_size[DTid.x];
  //debug

  GroupMemoryBarrierWithGroupSync();

  for (uint stride = NUM_THREAD/2; 0 < stride; stride /= 2)
  {
    if (DTid.x < stride)
    {
      const uint index2 = DTid.x + stride;
      shared_sum_grad_W[DTid.x] += shared_sum_grad_W[index2];
      shared_sum_grad_W_size[DTid.x] += shared_sum_grad_W_size[index2];
    }

    GroupMemoryBarrierWithGroupSync();
  }

  if (DTid.x == 0)
  {
    const float   sum_grad_W_size   = shared_sum_grad_W_size[0];

    const float3  v_sum_grad_W      = shared_sum_grad_W[0];
    const float   sum_dot_sum       = dot(v_sum_grad_W, v_sum_grad_W);

    scailing_factor_buffer[0] = 1.0 / (g_beta * (sum_dot_sum + sum_grad_W_size));   

    //debug
    debug_buffer[255].sum_dot_sum      = sum_dot_sum;
    debug_buffer[255].sum_grad_W_size  = sum_grad_W_size;
    //debug
  }  
}