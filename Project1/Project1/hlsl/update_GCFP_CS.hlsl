#define NUM_THREAD 32

#include "uniform_grid_common.hlsli"

StructuredBuffer<float3> fp_pos_buffer   : register(t0);

RWStructuredBuffer<uint>    fp_index_buffer   : register(u0);
RWStructuredBuffer<uint>    GCFP_count_buffer : register(u1);
RWStructuredBuffer<GCFP_ID> GCFP_ID_buffer    : register(u2);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{        
  if (g_num_particle <= DTID.x)
    return;

  const uint    fp_index      = DTID.x;
  const GCFP_ID prev_GCFP_id  = GCFP_ID_buffer[fp_index];
  const uint3   prev_GCid     = prev_GCFP_id.GCid;

  const float3  pos         = fp_pos_buffer[fp_index];
  const uint3   cur_GCid    = to_GCid(pos);
  const uint    cur_GCindex = to_GCindex(cur_GCid);

  if (any(prev_GCid != cur_GCid))
  {
    GCFP_ID cur_GCFP_id;
    cur_GCFP_id.GCid = cur_GCid;
    InterlockedAdd(GCFP_count_buffer[cur_GCindex], 1, cur_GCFP_id.gcfp_index);
    
    const uint prev_GCFP_index = prev_GCFP_id.GCFP_index();
    const uint cur_GCFP_index  = cur_GCFP_id.GCFP_index();
      
    const uint fp_index = fp_index_buffer[prev_GCFP_index];
      
    //update fp index buffer
    fp_index_buffer[prev_GCFP_index] = -1;
    fp_index_buffer[cur_GCFP_index] = fp_index;

    //update GCFPT ID buffer
    GCFP_ID_buffer[fp_index] = cur_GCFP_id;
  }
}