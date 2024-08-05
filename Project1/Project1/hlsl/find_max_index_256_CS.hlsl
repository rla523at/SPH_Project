#define NUM_THREAD 256

cbuffer Constant_Data : register(b0)
{
  uint g_num_data;
}

StructuredBuffer<float>   input   : register(t0);
RWStructuredBuffer<uint>  output  : register(u0);

groupshared float shared_value[NUM_THREAD];
groupshared uint  shared_index[NUM_THREAD];

static const float g_float_min = -3.4e38;

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint Gindex : SV_GroupIndex)
{
  uint data_index = DTid.x;
  
  if (data_index < g_num_data)
  {
    shared_value[Gindex] = input[data_index];
    shared_index[Gindex] = data_index;
  }
  else
    shared_value[Gindex] = g_float_min;
  
  GroupMemoryBarrierWithGroupSync();
  
  for (uint stride = NUM_THREAD / 2; 0 < stride; stride /= 2)
  {
    if (Gindex < stride)
    {
      if (shared_value[Gindex] < shared_value[Gindex + stride])
      {
        shared_value[Gindex] = shared_value[Gindex + stride];
        shared_index[Gindex] = shared_index[Gindex + stride];
      }
    }
    
    GroupMemoryBarrierWithGroupSync();
  }
  
  if (Gindex == 0)
    output[Gid.x] = shared_index[0];
}