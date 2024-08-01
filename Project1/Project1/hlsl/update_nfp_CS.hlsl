#include "Uniform_Grid_Common.hlsli"

//shader resource
StructuredBuffer<float3>    fp_pos_buffer       : register(t0);
Texture2D<uint>             GCFP_texture        : register(t1);
StructuredBuffer<uint>      GCFP_count_buffer   : register(t2);
StructuredBuffer<GCFPT_ID>  GCFP_ID_buffer      : register(t3);
Texture2D<uint>             ngc_texture         : register(t4);
StructuredBuffer<uint>      ngc_count_buffer    : register(t5);

//unordered access
RWTexture2D<uint>         nfp_texture         : register(u0);
RWTexture2D<float4>       nfp_tvec_texture    : register(u1);
RWTexture2D<float>        nfp_dist_texture    : register(u2);
RWStructuredBuffer<uint>  nfp_count_buffer    : register(u3);

//entry function
[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{        
  if (g_num_cell <= DTID.x)
    return;
 
  const uint        cur_fp_index  = DTID.x;     
  const GCFPT_ID    cur_id        = GCFP_ID_buffer[cur_fp_index];
  const uint        cur_gc_index  = cur_id.gc_index;
  
  const float3 v_xi = fp_pos_buffer[cur_fp_index];
    
  const uint num_ngc = ngc_count_buffer[cur_gc_index];
  for (uint i=0; i< num_ngc; ++i)
  {
    const uint ngc_index = ngc_texture[uint2(cur_gc_index, i)];

    uint neighbor_count = 0;

    const uint num_gcfp = GCFP_count_buffer[ngc_index];
    for (uint j = 0; j < num_gcfp; ++j)
    {
      const uint nfp_index = GCFP_texture[uint2(ngc_index, j)];
    
      const float3  v_xj      = fp_pos_buffer[nfp_index];
      const float3  v_xij     = v_xi - v_xj;
      const float   distance  = length(v_xij);
    
      if (g_divide_length < distance)
        continue;
      
      uint2 id = uint2(cur_fp_index, neighbor_count);
    
      nfp_texture[id]      = nfp_index;
      nfp_tvec_texture[id] = float4(v_xij, 0);
      nfp_dist_texture[id] = distance;
    
      ++neighbor_count;
    }
  
    nfp_count_buffer[cur_fp_index] = neighbor_count;
  }
}