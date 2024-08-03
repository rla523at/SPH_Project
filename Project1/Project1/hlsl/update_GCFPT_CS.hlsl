#include "uniform_grid_common.hlsli"

struct Debug_Strcut
{
  GCFP_ID prev_id;
  uint prev_id_GCFP_index;
  uint GCFP_count;
  uint prev_id_fp_index;
};

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
RWStructuredBuffer<Debug_Strcut>              debug_buffer            : register(u4);

// entry function
[numthreads(1, 1, 1)]
void main()
{
  Debug_Strcut db_end;
  db_end.prev_id.gcfp_index = -1;
  db_end.prev_id.gc_index = -1;
  db_end.prev_id_GCFP_index = -1;
  db_end.GCFP_count = -1;  
  db_end.prev_id_fp_index = -1;
  debug_buffer[g_consume_buffer_count] = db_end;
  
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
    
    //// update GCFP count buffer
    //GCFP_count_buffer[prev_gc_index]--;
  
    
    
    ////maintain data continuous
    //const uint prev_gc_index    = prev_id.gc_index;
    //const uint prev_gcfp_index  = prev_id.gcfp_index;
    //const uint num_prev_gcfp    = GCFP_count_buffer[prev_gc_index];
    
    //for (uint j = prev_gcfp_index; j < num_prev_gcfp - 1; ++j)
    //{
    //  GCFP_ID id1;
    //  id1.gc_index    = prev_gc_index;
    //  id1.gcfp_index  = j;

    //  GCFP_ID id2;
    //  id2.gc_index    = prev_gc_index;
    //  id2.gcfp_index  = j+1;
      
    //  const uint GCFP_index1 = id1.GCFP_index();
    //  const uint GCFP_index2 = id2.GCFP_index();

    //  const uint fp_index2 = fp_index_buffer[GCFP_index2];
    
    //  //update fp_index_buffer
    //  fp_index_buffer[GCFP_index1] = fp_index2;
    //  fp_index_buffer[GCFP_index2] = -1;

    //  //update GCFPT ID buffer
    //  GCFP_ID_buffer[fp_index2].gcfp_index--;
    //} 

    
    
    ////deubg 
    //Debug_Strcut db;
    //db.prev_id = prev_id;
    //db.prev_id_GCFP_index = prev_id.GCFP_index();
    //db.GCFP_count = GCFP_count_buffer[prev_id.gc_index];
    //db.prev_id_fp_index = fp_index;
    //debug_buffer[i] = db;


  }
}





// #include "Uniform_Grid_Common.hlsli"

// //constant buffer
// cbuffer Constant_Buffer : register(b1)
// {
//   uint g_consume_buffer_count;
// }

// // UAVs
// RWStructuredBuffer<uint>                      fp_index_buffer         : register(u0);
// RWStructuredBuffer<uint>                      GCFP_count_buffer       : register(u1);
// RWStructuredBuffer<GCFP_ID>                   GCFP_ID_buffer          : register(u2);
// ConsumeStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer  : register(u3);

// // entry function
// [numthreads(1, 1, 1)]
// void main()
// {
//   for (uint i = 0; i < g_consume_buffer_count; ++i)
//   {
//     const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();
              
//     const GCFP_ID  prev_id   = data.prev_id;
//     const uint     fp_index  = fp_index_buffer[prev_id.GCFP_index()];

//     //remove data    
//     const uint prev_gc_index    = prev_id.gc_index;
//     const uint prev_gcfp_index  = prev_id.gcfp_index;
//     const uint num_prev_gcfp    = GCFP_count_buffer[prev_gc_index];
    
//     for (uint j = prev_gcfp_index; j < num_prev_gcfp - 1; ++j)
//     {
//       GCFP_ID id1;
//       id1.gc_index    = prev_gc_index;
//       id1.gcfp_index  = j;

//       GCFP_ID id2;
//       id2.gc_index    = prev_gc_index;
//       id2.gcfp_index  = j+1;
      
//       const uint GCFP_index1 = id1.GCFP_index();
//       const uint GCFP_index2 = id2.GCFP_index();

//       const uint fp_index2 = fp_index_buffer[GCFP_index2];
    
//       //update fp_index_buffer
//       fp_index_buffer[GCFP_index1] = fp_index2;

//       //update GCFPT ID buffer
//       GCFP_ID_buffer[fp_index2].gcfp_index--;
//     }
    
//     // update GCFP count buffer
//     GCFP_count_buffer[prev_gc_index]--;
    
//     // add data
//     GCFP_ID cur_id;
//     cur_id.gc_index   = data.cur_gc_index;
//     cur_id.gcfp_index = GCFP_count_buffer[data.cur_gc_index];
    
//     //update GCFP texture
//     fp_index_buffer[cur_id.GCFP_index()] = fp_index;

//     //update GCFPT ID buffer
//     GCFP_ID_buffer[fp_index] = cur_id;

//     //update GCFP count buffer
//     GCFP_count_buffer[cur_id.gc_index]++;
//   }
// }