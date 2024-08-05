#include "uniform_grid_common.hlsli"

RWStructuredBuffer<uint>    fp_index_buffer     : register(u0);
RWStructuredBuffer<uint>    GCFP_count_buffer   : register(u1);
RWStructuredBuffer<GCFP_ID> GCFP_ID_buffer      : register(u2);

[numthreads(1024,1,1)]
void main(uint3 DTID : SV_DispatchThreadID)
{
  if (g_num_cell <= DTID.x)
    return;
  
  const uint gc_index     = DTID.x;  
  const uint num_gcfp     = GCFP_count_buffer[gc_index];
  const uint last_index   = gc_index * g_estimated_num_gcfp + num_gcfp - 1;
  
  uint change_index = last_index;
  for (uint i = 0; i < num_gcfp; ++i)
  {
    if (fp_index_buffer[last_index - i] != -1)
      continue;
    
    const uint fp_index = fp_index_buffer[change_index];
    
    if (fp_index != -1)
    {
      fp_index_buffer[last_index - i]     = fp_index;
      fp_index_buffer[change_index]       = -1;
      GCFP_ID_buffer[fp_index].gcfp_index = num_gcfp - 1 - i;
    }
    
    GCFP_count_buffer[gc_index]--;
    change_index--;
  }
}