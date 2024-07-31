struct GCFPT_ID
{
  uint gc_index;
  uint gcfp_index;
};

struct Changed_GCFPT_ID_Data
{
  GCFPT_ID prev_id;
  uint cur_gc_index;
};

cbuffer Constant_Buffer : register(b0)
{
  uint consume_buffer_count;
}

RWTexture2D<uint> GCFP_texture : register(u0);
RWStructuredBuffer<uint> GCFP_counter_buffer : register(u1);
RWStructuredBuffer<GCFPT_ID> GCFPT_ID_buffer : register(u2);
ConsumeStructuredBuffer<Changed_GCFPT_ID_Data> changed_GCFPT_ID_buffer : register(u3);

// user define fuction
uint2 to_uint2(const GCFPT_ID id)
{
  return uint2(id.gc_index, id.gcfp_index);
}

// entry function
[numthreads(1, 1, 1)]
void main()
{
  for (int i = 0; i < consume_buffer_count; ++i)
  {
    const Changed_GCFPT_ID_Data changed_data = changed_GCFPT_ID_buffer.Consume();
              
    const GCFPT_ID prev_id = changed_data.prev_id;
    const uint fp_index = GCFP_texture[to_uint2(prev_id)];

    //remove previous data    
    const uint prev_gc_index = prev_id.gc_index;
    const uint prev_gcfp_index = prev_id.gcfp_index;
    const uint num_prev_gcfp = GCFP_counter_buffer[prev_gc_index];
    
    for (uint j = prev_gcfp_index; j < num_prev_gcfp - 1; ++j)
    {
      const uint2 id1 = uint2(prev_gc_index, j);
      const uint2 id2 = uint2(prev_gc_index, j + 1);
      
      const uint fp_index2 = GCFP_texture[id2];
      
      //over write
      GCFP_texture[id1] = fp_index2;
      
      // update GCFPT ID buffer
      GCFPT_ID_buffer[fp_index2].gcfp_index--;
    }
    
    // update GCFP counter buffer
    GCFP_counter_buffer[prev_gc_index]--;
    
    //add new data
    const uint cur_gc_index = changed_data.cur_gc_index;
    const uint cur_gcfp_index = GCFP_counter_buffer[cur_gcfp_index];
    const uint2 cur_id = uint2(cur_gc_index, cur_gcfp_index);
    
    GCFP_texture[cur_id] = fp_index;

    // update GCFPT ID buffer
    GCFPT_ID_buffer[fp_index] =
    
    // update GCFP counter buffer
    GCFP_counter_buffer[cur_gc_index]++;
  }
}