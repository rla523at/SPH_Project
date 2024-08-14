////Version3
#define NUM_THREAD 64
#define NUM_MAX_GROUP 65535

#include "uniform_grid_common.hlsli"
#include "uniform_grid_output.hlsli"

//shader resource
StructuredBuffer<float3> fp_pos_buffer : register(t0);
StructuredBuffer<uint> fp_index_buffer : register(t1);
StructuredBuffer<uint> GCFP_count_buffer : register(t2);
StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
StructuredBuffer<uint> ngc_index_buffer : register(t4);
StructuredBuffer<uint> ngc_count_buffer : register(t5);

//unordered access
RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
RWStructuredBuffer<uint> nfp_count_buffer : register(u1);

struct debug_struct
{
  uint3 cur_GCid;
  uint3 neighbor_GCids[27];
  uint valid_count;
};

RWStructuredBuffer<debug_struct> debug_buffer : register(u2);


//entry function
[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  const uint cur_fp_index = Gid.x + Gid.y * NUM_MAX_GROUP;

  if (g_num_particle <= cur_fp_index)
    return;

  if (Gindex == 0)
    nfp_count_buffer[cur_fp_index] = 0;

  const GCFP_ID cur_id = GCFP_ID_buffer[cur_fp_index];
  const uint3 cur_GCid = cur_id.GCid;
  const uint cur_gc_index = to_GCindex(cur_id.GCid);
  
  const float3 v_xi = fp_pos_buffer[cur_fp_index];
  
  for (uint i = 0; i < g_estimated_num_ngc; ++i)
  {
    const uint3 nbr_GCid = find_neighbor_GCid(cur_GCid, i);
    const uint nbr_gc_index = to_GCindex(nbr_GCid);
    const uint num_gcfp = GCFP_count_buffer[nbr_gc_index];
    
    if (is_valid_GCid(nbr_GCid) && Gindex < num_gcfp)
    {
      const uint nbr_GCindex = to_GCindex(nbr_GCid);
      const uint nbr_FPindex = fp_index_buffer[nbr_GCindex * g_estimated_num_gcfp + Gindex];
      const float3 v_xj = fp_pos_buffer[nbr_FPindex];
      const float3 v_xij = v_xi - v_xj;
      const float distance = length(v_xij);

      if (g_divide_length < distance)
        continue;
  
      Neighbor_Information info;
      info.fp_index = nbr_FPindex;
      info.tvector = v_xij;
      info.distance = distance;

      uint nbr_count;
      InterlockedAdd(nfp_count_buffer[cur_fp_index], 1, nbr_count);

      nfp_info_buffer[cur_fp_index * g_estimated_num_nfp + nbr_count] = info;
    }
  }
}




////// update nfp Version2
//#define NUM_THREAD 64
//#define NUM_MAX_GROUP 65535

//#include "uniform_grid_common.hlsli"
//#include "uniform_grid_output.hlsli"

////shader resource
//StructuredBuffer<float3> fp_pos_buffer : register(t0);
//StructuredBuffer<uint> fp_index_buffer : register(t1);
//StructuredBuffer<uint> GCFP_count_buffer : register(t2);
//StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
//StructuredBuffer<uint> ngc_index_buffer : register(t4);
//StructuredBuffer<uint> ngc_count_buffer : register(t5);

////unordered access
//RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
//RWStructuredBuffer<uint> nfp_count_buffer : register(u1);

////entry function
//[numthreads(NUM_THREAD, 1, 1)]
//void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
//{
//  const uint cur_fp_index = Gid.x + Gid.y * NUM_MAX_GROUP;

//  if (g_num_particle <= cur_fp_index)
//    return;

//  if (Gindex == 0)
//    nfp_count_buffer[cur_fp_index] = 0;

//  const GCFP_ID cur_id = GCFP_ID_buffer[cur_fp_index];
//  const uint cur_gc_index = to_GCindex(cur_id.GCid);
//  const uint num_ngc = ngc_count_buffer[cur_gc_index];

//  const float3 v_xi = fp_pos_buffer[cur_fp_index];

//  for (uint i = 0; i < num_ngc; ++i)
//  {
//    const uint ngc_index = ngc_index_buffer[cur_gc_index * g_estimated_num_ngc + i];
//    const uint num_gcfp = GCFP_count_buffer[ngc_index];

//    if (Gindex < num_gcfp)
//    {
//      const uint nfp_index = fp_index_buffer[ngc_index * g_estimated_num_gcfp + Gindex];
//      const float3 v_xj = fp_pos_buffer[nfp_index];
//      const float3 v_xij = v_xi - v_xj;
//      const float distance = length(v_xij);

//      if (g_divide_length < distance)
//        continue;
  
//      Neighbor_Information info;
//      info.fp_index = nfp_index;
//      info.tvector = v_xij;
//      info.distance = distance;

//      uint nbr_count;
//      InterlockedAdd(nfp_count_buffer[cur_fp_index], 1, nbr_count);

//      nfp_info_buffer[cur_fp_index * g_estimated_num_nfp + nbr_count] = info;
//    }
//  }
//}