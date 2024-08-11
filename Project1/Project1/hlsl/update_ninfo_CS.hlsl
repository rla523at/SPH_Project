#define NUM_THREAD 256

struct GCFP_ID
{
  uint gc_index;
  uint gcfp_index;
};

struct Neighbor_Information
{
  uint    nbr_fp_index;   // neighbor의 fluid particle index
  uint    neighbor_index; // this가 neighbor한테 몇번째 neighbor인지 나타내는 index
  float3  v_xij;          // neighbor to this vector
  float   distance;
  float   distnace2;      // distance square
};

cbuffer CB : register(b0)
{
  uint  g_estimated_num_ngc;
  uint  g_estimated_num_gcfp;
  uint  g_estimated_num_nfp;
  uint  g_num_particle;
  float g_supprot_radius;
};

StructuredBuffer<uint>      fp_index_buffer     : register(t0);
StructuredBuffer<uint>      GCFP_count_buffer   : register(t1);
StructuredBuffer<GCFP_ID>   GCFP_ID_buffer      : register(t2);
StructuredBuffer<uint>      ngc_index_buffer    : register(t3);
StructuredBuffer<uint>      ngc_count_buffer    : register(t4);
StructuredBuffer<float3>    fp_pos_buffer       : register(t5);

RWStructuredBuffer<Neighbor_Information>  ninfo_buffer   : register(u0);
RWStructuredBuffer<uint>                  ncount_buffer  : register(u1);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{        
  if (g_num_particle <= DTID.x)
    return;
 
  const uint    cur_fp_index  = DTID.x;     
  const GCFP_ID cur_id        = GCFP_ID_buffer[cur_fp_index];
  const uint    cur_gc_index  = cur_id.gc_index;  
  const uint    num_ngc       = ngc_count_buffer[cur_gc_index];
  
  const uint start_index1 = cur_gc_index * g_estimated_num_ngc;
  const uint start_index3 = cur_fp_index * g_estimated_num_nfp;

  const float3 v_xi = fp_pos_buffer[cur_fp_index];  

  for (uint i=0; i< num_ngc; ++i)
  {
    const uint ngc_index    = ngc_index_buffer[start_index1 + i];
    const uint num_gcfp     = GCFP_count_buffer[ngc_index];
    const uint start_index2 = ngc_index * g_estimated_num_gcfp;

    for (uint j = 0; j < num_gcfp; ++j)
    {
      const uint nbr_fp_index = fp_index_buffer[start_index2 + j];    
      const uint start_index4 = nbr_fp_index * g_estimated_num_nfp;
      
      if (nbr_fp_index < cur_fp_index)
        continue;
      
      const float3  v_xj      = fp_pos_buffer[nbr_fp_index];
      const float3  v_xij     = v_xi - v_xj;
      const float   distance  = length(v_xij);
      const float   distance2 = distance * distance;

      if (g_supprot_radius < distance)
        continue;
      
      uint this_ncount;
      uint neighbor_ncount;
      InterlockedAdd(ncount_buffer[cur_fp_index], 1, this_ncount);      
      InterlockedAdd(ncount_buffer[nbr_fp_index], 1, neighbor_ncount);
      
      Neighbor_Information info;      
      info.nbr_fp_index   = nbr_fp_index;
      info.neighbor_index = neighbor_ncount;
      info.v_xij          = v_xij;
      info.distance       = distance;
      info.distnace2      = distance2;      

      Neighbor_Information info2;
      info2.nbr_fp_index    = cur_fp_index;
      info2.neighbor_index  = this_ncount;
      info2.v_xij           = -v_xij;
      info2.distance        = distance;
      info2.distnace2       = distance2;      

      ninfo_buffer[start_index3 + this_ncount]      = info;
      ninfo_buffer[start_index4 + neighbor_ncount]  = info2;
    }   
  }
}