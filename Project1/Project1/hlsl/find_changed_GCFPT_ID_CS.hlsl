#include "Uniform_Grid_Common.hlsli"

  struct Debug_Data
  {
    uint cur_gc_index;
    uint prev_gc_index;
  };

//shader resource
StructuredBuffer<float3>  fp_pos_buffer   : register(t0);
StructuredBuffer<GCFP_ID> GCFP_ID_buffer  : register(t1);

//unordered acess
AppendStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer : register(u0);
RWStructuredBuffer<Debug_Data> debug_buffer : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{        
  if (g_num_particle <= DTID.x)
    return;

  const uint    fp_index      = DTID.x;
  const GCFP_ID prev_GCFP_id  = GCFP_ID_buffer[fp_index];
  const uint    prev_gc_index = prev_GCFP_id.gc_index;

  const float3  pos           = fp_pos_buffer[fp_index];
  const uint    cur_gc_index  = grid_cell_index(pos);

  Debug_Data debug_data;
  debug_data.cur_gc_index = -1;
  debug_data.prev_gc_index = -1;

  debug_buffer[fp_index] = debug_data;

  if (prev_gc_index != cur_gc_index)
  {
    debug_data.cur_gc_index = cur_gc_index;
    debug_data.prev_gc_index = prev_gc_index;
  
    debug_buffer[fp_index] = debug_data;

    Changed_GCFP_ID_Data data;
    data.prev_id      = prev_GCFP_id;
    data.cur_gc_index = cur_gc_index;

    changed_GCFP_ID_buffer.Append(data);
  }
}

// #include "Uniform_Grid_Common.hlsli"

// //shader resource
// StructuredBuffer<float3>  fp_pos_buffer   : register(t0);
// StructuredBuffer<GCFP_ID> GCFP_ID_buffer  : register(t1);

// //unordered acess
// AppendStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer : register(u0);

// [numthreads(1024, 1, 1)]
// void main(uint3 DTID : SV_DispatchThreadID)
// {        
//   if (g_num_particle <= DTID.x)
//     return;

//   const uint    fp_index      = DTID.x;
//   const GCFP_ID prev_GCFP_id  = GCFP_ID_buffer[fp_index];
//   const uint    prev_gc_index = prev_GCFP_id.gc_index;

//   const float3  pos           = fp_pos_buffer[fp_index];
//   const uint    cur_gc_index  = grid_cell_index(pos);

//   if (prev_gc_index != cur_gc_index)
//   {
//     Changed_GCFP_ID_Data data;
//     data.prev_id      = prev_GCFP_id;
//     data.cur_gc_index = cur_gc_index;

//     changed_GCFP_ID_buffer.Append(data);
//   }
// }