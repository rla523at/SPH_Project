#include "Uniform_Grid_Common.hlsli"

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

StructuredBuffer<float3>    fp_pos_buffer   : register(t0);
StructuredBuffer<GCFPT_ID>  GCFPT_ID_buffer : register(t1);

AppendStructuredBuffer<Changed_GCFPT_ID_Data> changed_GCFPT_ID_buffer : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{
  uint num_data, size;
  fp_pos_buffer.GetDimensions(num_data, size);

  const uint fp_index = DTID.x;
        
  if (num_data < fp_index)
    return;

  const float3 pos = fp_pos_buffer[fp_index];

  const GCFPT_ID prev_GCFPT_id = GCFPT_ID_buffer[fp_index];
  const uint cur_gc_index = grid_cell_index(pos);

  if (prev_GCFPT_id.gc_index != cur_gc_index)
  {
    Changed_GCFPT_ID_Data data;
    data.prev_id = prev_GCFPT_id;
    data.cur_gc_index = cur_gc_index;

    changed_GCFPT_ID_buffer.Append(data);
  }
}