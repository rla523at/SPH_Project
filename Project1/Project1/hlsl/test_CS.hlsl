////Version3
#define NUM_THREAD 64
#include "uniform_grid_common.hlsli"

//unordered access
RWStructuredBuffer<uint> count_buffer  : register(u0);

//entry function
[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{        
  uint GCindex = DTid.x;
  
  uint3 GCid;
  GCid.x = GCindex % g_num_x_cell;  
  GCindex /= g_num_x_cell;
  
  GCid.z = GCindex % g_num_z_cell;
  GCindex /= g_num_z_cell;
  
  GCid.y = GCindex % g_num_y_cell;
  
  uint count = 0;
  for (uint i = 0; i < g_estimated_num_ngc; ++i)
  {
    if (is_valid_GCid(find_neighbor_GCid(GCid, i)))
      count++;
  }
  
  count_buffer[DTid.x] = count;
}