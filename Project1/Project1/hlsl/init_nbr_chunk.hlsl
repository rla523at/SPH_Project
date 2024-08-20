#define NUM_GROUP_MAX 65535

struct Local_Nbr_Sum
{
  uint nbr_sum[64];
};

cbuffer CB: register(b0)
{
  uint g_num_FP;
}

StructuredBuffer<uint> ncount_buffer : register(t0);

RWStructuredBuffer<uint>            nbr_chunk_buffer        : register(u0);
RWStructuredBuffer<uint>            nbr_chunk_count_buffer  : register(u1);
RWBuffer<uint>                      nbr_chunk_DIA_buffer    : register(u2);
RWStructuredBuffer<Local_Nbr_Sum>   local_nbr_sum_buffer    : register(u3);

[numthreads(1,1,1)]
void main(void)
{
  const uint num_nbr_max = 256;
  
  uint index    = 0;
  uint num_nbr  = 0;
  uint index2   = 0;
  for (uint i=0; i<g_num_FP; ++i)
  {
    num_nbr += ncount_buffer[i];

    if (num_nbr_max < num_nbr)
    {
      nbr_chunk_buffer[index++] = i;
      num_nbr                   = ncount_buffer[i];
      index2                    = 0;
    }
      
    local_nbr_sum_buffer[index].nbr_sum[index2++] = num_nbr;
  }

  if (nbr_chunk_buffer[index] != g_num_FP)
    nbr_chunk_buffer[index++] = g_num_FP;

  const uint num_nbr_chunk  = index;
  nbr_chunk_count_buffer[0] = num_nbr_chunk;
  nbr_chunk_DIA_buffer[0]   = num_nbr_chunk;
  nbr_chunk_DIA_buffer[1]   = 1;
  nbr_chunk_DIA_buffer[2]   = 1;

  if (NUM_GROUP_MAX < num_nbr_chunk)
  {
    nbr_chunk_DIA_buffer[0] = NUM_GROUP_MAX;
    nbr_chunk_DIA_buffer[1] = (num_nbr_chunk + NUM_GROUP_MAX -1) / NUM_GROUP_MAX ; // ceil(num_nbr_chunk, NUM_GROUP_MAX)
  }
}