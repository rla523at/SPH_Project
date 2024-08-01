#include "Uniform_Grid_Common.hlsli"

//constant buffer
cbuffer Constant_Buffer : register(b1)
{
  uint g_consume_buffer_count;
}

// UAVs
RWTexture2D<uint>                               GCFP_texture            : register(u0);
RWStructuredBuffer<uint>                        GCFP_count_buffer       : register(u1);
RWStructuredBuffer<GCFPT_ID>                    GCFPT_ID_buffer         : register(u2);
ConsumeStructuredBuffer<Changed_GCFPT_ID_Data>  changed_GCFPT_ID_buffer : register(u3);

// entry function
[numthreads(1, 1, 1)]
void main()
{
  for (uint i = 0; i < g_consume_buffer_count; ++i)
  {
    const Changed_GCFPT_ID_Data data = changed_GCFPT_ID_buffer.Consume();
              
    const GCFPT_ID  prev_id   = data.prev_id;
    const uint      fp_index  = GCFP_texture[prev_id.to_uint2()];

    //remove data    
    const uint prev_gc_index    = prev_id.gc_index;
    const uint prev_gcfp_index  = prev_id.gcfp_index;
    const uint num_prev_gcfp    = GCFP_count_buffer[prev_gc_index];
    
    for (uint j = prev_gcfp_index; j < num_prev_gcfp - 1; ++j)
    {
      const uint2 id1 = uint2(prev_gc_index, j);
      const uint2 id2 = uint2(prev_gc_index, j + 1);
      
      const uint fp_index2 = GCFP_texture[id2];
    
      //update GCFP texture
      GCFP_texture[id1] = fp_index2;

      //update GCFPT ID buffer
      GCFPT_ID_buffer[fp_index2].gcfp_index--;
    }
    
    // update GCFP count buffer
    GCFP_count_buffer[prev_gc_index]--;
    
    // add data
    GCFPT_ID cur_id;
    cur_id.gc_index   = data.cur_gc_index;
    cur_id.gcfp_index = GCFP_count_buffer[data.cur_gc_index];
    
    //update GCFP texture
    GCFP_texture[cur_id.to_uint2()] = fp_index;

    //update GCFPT ID buffer
    GCFPT_ID_buffer[fp_index] = cur_id;

    //update GCFP count buffer
    GCFP_count_buffer[cur_id.gc_index]++;
  }
}