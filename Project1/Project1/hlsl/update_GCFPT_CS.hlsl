#include "uniform_grid_common.hlsli"

//constant buffer
cbuffer Constant_Buffer : register(b1)
{
  uint g_consume_buffer_count;
}

// UAVs
RWStructuredBuffer<uint>                      fp_index_buffer         : register(u0);
RWStructuredBuffer<uint>                      GCFP_count_buffer       : register(u1);
RWStructuredBuffer<GCFP_ID>                   GCFP_ID_buffer          : register(u2);
ConsumeStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer  : register(u3);

// entry function
[numthreads(1, 1, 1)]
void main()
{
  for (uint i = 0; i < g_consume_buffer_count; ++i)
  {
    const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();
    
    const GCFP_ID   prev_id             = data.prev_id;
    const uint      prev_id_GCFP_index  = prev_id.GCFP_index();
    
    const uint      fp_index            = fp_index_buffer[prev_id_GCFP_index];
    
    GCFP_ID cur_id;
    cur_id.gc_index   = data.cur_gc_index;
    cur_id.gcfp_index = GCFP_count_buffer[data.cur_gc_index];
   
    const uint cur_id_GCFP_index = cur_id.GCFP_index();
    
    //update fp index buffer
    fp_index_buffer[prev_id_GCFP_index] = -1;      
    fp_index_buffer[cur_id_GCFP_index]  = fp_index;

    //update GCFPT ID buffer
    GCFP_ID_buffer[fp_index] = cur_id;
    
    //update GCFP count buffer
    GCFP_count_buffer[cur_id.gc_index]++;
  }
}