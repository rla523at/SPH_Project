#include "uniform_grid_common.hlsli"

#define NUM_THREAD 256

cbuffer CB : register(b1)
{
  uint g_consume_buffer_count;
}

RWStructuredBuffer<uint>                      fp_index_buffer         : register(u0);
RWStructuredBuffer<uint>                      GCFP_count_buffer       : register(u1);
RWStructuredBuffer<GCFP_ID>                   GCFP_ID_buffer          : register(u2);
ConsumeStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer  : register(u3);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_consume_buffer_count <= DTid.x) 
    return;
  
  const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();
    
  GCFP_ID cur_id;
  cur_id.gc_index = data.cur_gc_index;
  InterlockedAdd(GCFP_count_buffer[data.cur_gc_index], 1, cur_id.gcfp_index);

  const uint prev_GCFP_index  = data.prev_id.GCFP_index();
  const uint cur_GCFP_index   = cur_id.GCFP_index();    
  const uint fp_index         = fp_index_buffer[prev_GCFP_index];
    
  fp_index_buffer[prev_GCFP_index]  = -1;
  fp_index_buffer[cur_GCFP_index]   = fp_index;
  GCFP_ID_buffer[fp_index]          = cur_id;
  
  //prev_id가 지워진 정보는 rearrange_GCFP_CS에서 GCFP count buffer에 반영됨
}