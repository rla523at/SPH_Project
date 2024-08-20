#define NUM_THREAD 64
#define COUNT_MAX 256

cbuffer CB: register(b0)
{
  uint g_num_FP;
}

StructuredBuffer<uint> ncount_buffer : register(t0);

RWStructuredBuffer<uint> nbr_chunk_end_index_buffer : register(u0);

groupshared uint shared_ncount[128];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 Gid : SV_GroupId, uint Gindex : SV_GroupIndex)
{
  {
    const uint FP_index = Gid.x * NUM_THREAD + 2 * Gindex;

    if (g_num_FP <= FP_index)
    {
      shared_ncount[2*Gindex] = 0;
      shared_ncount[2*Gindex+1] = 0;      
    }
    else if (g_num_FP <= FP_index + 1)
    {
      shared_ncount[2*Gindex] = 0;
    }
    else
    {
      shared_ncount[2*Gindex]     = ncount_buffer[FP_index];
      shared_ncount[2*Gindex + 1] = ncount_buffer[FP_index + 1];
    }
      
    GroupMemoryBarrierWithGroupSync();
  }

  const uint FP_index = Gid.x * NUM_THREAD + Gindex;
  
  if (g_num_FP <= FP_index)
    return;
  
  uint count =0;
  for (uint i=0; i<64; ++i)
  {
    count += shared_ncount[Gindex + i];
  
    if (COUNT_MAX < count)
    {
      nbr_chunk_end_index_buffer[FP_index] = FP_index + i;
      break;
    }
    else if (g_num_FP <= FP_index+i)
    {
      nbr_chunk_end_index_buffer[FP_index] = g_num_FP;
      break;
    }
  }
}