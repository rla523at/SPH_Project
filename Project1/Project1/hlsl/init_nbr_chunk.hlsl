#define NUM_GROUP_MAX 65535

cbuffer CB: register(b0)
{
  uint g_num_FP;
}

StructuredBuffer<uint> nbr_chunk_end_index_buffer : register(t0);

RWStructuredBuffer<uint>            nbr_chunk_buffer        : register(u0);
RWStructuredBuffer<uint>            nbr_chunk_count_buffer  : register(u1);
RWBuffer<uint>                      nbr_chunk_DIA_buffer    : register(u2);

RWStructuredBuffer<uint>            debug_uint_buffer  : register(u3);//debug


[numthreads(1,1,1)]
void main(void)
{
  uint index = 0;
  for (uint i=0; i<g_num_FP;)
  {
    const uint end_index      = nbr_chunk_end_index_buffer[i];
    nbr_chunk_buffer[index++] = end_index;
    
    i = end_index;
  }
  
  const uint num_nbr_chunk  = index;
  nbr_chunk_count_buffer[0] = num_nbr_chunk;
  
  if (num_nbr_chunk < NUM_GROUP_MAX)
  {
    nbr_chunk_DIA_buffer[0]   = num_nbr_chunk;
    nbr_chunk_DIA_buffer[1]   = 1;
    nbr_chunk_DIA_buffer[2]   = 1;
  }
  else
  {
    nbr_chunk_DIA_buffer[0] = NUM_GROUP_MAX;
    nbr_chunk_DIA_buffer[1] = (num_nbr_chunk + NUM_GROUP_MAX -1) / NUM_GROUP_MAX ; // ceil(num_nbr_chunk, NUM_GROUP_MAX)
    nbr_chunk_DIA_buffer[2] = 1;
  }
}