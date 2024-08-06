#define NUM_THREAD 256

cbuffer CB : register(b0)
{
  uint g_num_fluid_particle;
}

RWStructuredBuffer<float>   pressure_buffer     : register(u0);
RWStructuredBuffer<float3>  a_presssure_buffer  : register(u1);

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fluid_particle <= DTid.x)
    return;

  pressure_buffer[DTid.x]     = 0.0;
  a_presssure_buffer[DTid.x]  = float3(0.0, 0.0, 0.0);
}