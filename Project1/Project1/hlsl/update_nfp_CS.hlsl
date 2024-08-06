#include "uniform_grid_common.hlsli"
#include "uniform_grid_output.hlsli"

//shader resource
StructuredBuffer<float3>    fp_pos_buffer       : register(t0);
StructuredBuffer<uint>      fp_index_buffer     : register(t1);
StructuredBuffer<uint>      GCFP_count_buffer   : register(t2);
StructuredBuffer<GCFP_ID>   GCFP_ID_buffer      : register(t3);
StructuredBuffer<uint>      ngc_index_buffer    : register(t4);
StructuredBuffer<uint>      ngc_count_buffer    : register(t5);

//unordered access
RWStructuredBuffer<Neighbor_Information>  nfp_info_buffer   : register(u0);
RWStructuredBuffer<uint>                  nfp_count_buffer  : register(u1);

//entry function
[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{        
  if (g_num_cell <= DTID.x)
    return;
 
  const uint    cur_fp_index  = DTID.x;     
  const GCFP_ID cur_id        = GCFP_ID_buffer[cur_fp_index];
  const uint    cur_gc_index  = cur_id.gc_index;  
  const uint    num_ngc       = ngc_count_buffer[cur_gc_index];
  const uint    start_index1  = cur_gc_index * g_estimated_num_ngc;
  const uint    start_index3  = cur_fp_index * g_estimated_num_nfp;

  const float3 v_xi = fp_pos_buffer[cur_fp_index];  

  uint neighbor_count = 0;

  for (uint i=0; i< num_ngc; ++i)
  {
    const uint ngc_index    = ngc_index_buffer[start_index1 + i];
    const uint num_gcfp     = GCFP_count_buffer[ngc_index];
    const uint start_index2 = ngc_index * g_estimated_num_gcfp;
    
    for (uint j = 0; j < num_gcfp; ++j)
    {
      const uint    nfp_index = fp_index_buffer[start_index2 + j];    
      const float3  v_xj      = fp_pos_buffer[nfp_index];
      const float3  v_xij     = v_xi - v_xj;
      const float   distance  = length(v_xij);
    
      if (g_divide_length < distance)
        continue;
      
      Neighbor_Information info;
      info.fp_index = nfp_index;
      info.tvector  = v_xij;
      info.distance = distance;

      nfp_info_buffer[start_index3 + neighbor_count] = info;
    
      ++neighbor_count;
    }   
  }

  nfp_count_buffer[cur_fp_index] = neighbor_count;    
}