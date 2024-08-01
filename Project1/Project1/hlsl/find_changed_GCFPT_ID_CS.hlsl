#include "Uniform_Grid_Common.hlsli"

//shader resource
StructuredBuffer<float3>    fp_pos_buffer   : register(t0);
StructuredBuffer<GCFPT_ID>  GCFPT_ID_buffer : register(t1);

//unordered acess
AppendStructuredBuffer<Changed_GCFPT_ID_Data> changed_GCFPT_ID_buffer : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{        
  if (g_num_particle <= DTID.x)
    return;

  const uint      fp_index      = DTID.x;
  const GCFPT_ID  prev_GCFPT_id = GCFPT_ID_buffer[fp_index];
  const float3    pos           = fp_pos_buffer[fp_index];
  const uint      cur_gc_index  = grid_cell_index(pos);

  if (prev_GCFPT_id.gc_index != cur_gc_index)
  {
    Changed_GCFPT_ID_Data data;
    data.prev_id = prev_GCFPT_id;
    data.cur_gc_index = cur_gc_index;

    changed_GCFPT_ID_buffer.Append(data);
  }
}