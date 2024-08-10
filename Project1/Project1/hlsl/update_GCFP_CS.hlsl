#include "uniform_grid_common.hlsli"

#define NUM_THREAD 1024

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
[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_consume_buffer_count <= DTid.x) 
    return;
  
  const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();
    
  //update GCFP count buffer
  //prev_id가 지워진 정보는 rearrange_GCFP_CS에서 업데이트함
  uint cur_gcfp_index;
  InterlockedAdd(GCFP_count_buffer[data.cur_gc_index], 1, cur_gcfp_index);

  const GCFP_ID cur_id = make_GCFP_ID(data.cur_gc_index, cur_gcfp_index);
    
  const uint prev_id_GCFP_index = data.prev_id.GCFP_index();
  const uint cur_id_GCFP_index = cur_id.GCFP_index();
    
  const uint fp_index = fp_index_buffer[prev_id_GCFP_index];
    
  //update fp index buffer
  fp_index_buffer[prev_id_GCFP_index] = -1;
  fp_index_buffer[cur_id_GCFP_index] = fp_index;

  //update GCFPT ID buffer
  GCFP_ID_buffer[fp_index] = cur_id;
}

//// entry function
//[numthreads(1, 1, 1)]
//void main()
//{
//  for (uint i = 0; i < g_consume_buffer_count; ++i)
//  {
//    const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();
    
//    const uint cur_gcfp_index = GCFP_count_buffer[data.cur_gc_index];
//    const GCFP_ID cur_id = make_GCFP_ID(data.cur_gc_index, cur_gcfp_index);
    
//    const uint prev_id_GCFP_index = data.prev_id.GCFP_index();
//    const uint cur_id_GCFP_index = cur_id.GCFP_index();
    
//    const uint fp_index = fp_index_buffer[prev_id_GCFP_index];
    
//    //update fp index buffer
//    fp_index_buffer[prev_id_GCFP_index] = -1;
//    fp_index_buffer[cur_id_GCFP_index] = fp_index;

//    //update GCFPT ID buffer
//    GCFP_ID_buffer[fp_index] = cur_id;
    
//    //update GCFP count buffer
//    //prev_id가 지워진 정보는 rearrange_GCFP_CS에서 업데이트함
//    GCFP_count_buffer[cur_id.gc_index]++; //이 부분 때문에 병렬화가 안됨
//  }
//}