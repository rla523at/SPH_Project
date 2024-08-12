#define NUM_THREAD 256

cbuffer CB : register(b0)
{
  uint g_num_fp;
}

RWStructuredBuffer<uint> ncount_buffer : register(u0);

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  ncount_buffer[DTid.x]     = 0.0;
}