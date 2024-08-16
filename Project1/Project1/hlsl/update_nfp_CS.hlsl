#define NUM_THREAD 64
#define NUM_MAX_GROUP 65535

#include "uniform_grid_common.hlsli"
#include "uniform_grid_output.hlsli"

StructuredBuffer<float3> fp_pos_buffer : register(t0);
StructuredBuffer<uint> fp_index_buffer : register(t1);
StructuredBuffer<uint> GCFP_count_buffer : register(t2);
StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
StructuredBuffer<uint> ngc_index_buffer : register(t4);
StructuredBuffer<uint> ngc_count_buffer : register(t5);

RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
RWStructuredBuffer<uint> nfp_count_buffer : register(u1);

groupshared uint shared_ngc_index[27];
groupshared uint shared_num_gcfp[27];
groupshared uint shared_index[NUM_THREAD+1];

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
 const uint cur_fp_index = Gid.x + Gid.y * NUM_MAX_GROUP; 

 if (g_num_particle <= cur_fp_index)
   return;

 if (Gindex == 0)
   nfp_count_buffer[cur_fp_index] = 0;

 const GCFP_ID cur_id       = GCFP_ID_buffer[cur_fp_index];
 const uint cur_gc_index    = to_GCindex(cur_id.GCid);
 const uint num_ngc         = ngc_count_buffer[cur_gc_index];

 if (Gindex < num_ngc)
 {
   const uint ngc_index = ngc_index_buffer[cur_gc_index * g_estimated_num_ngc + Gindex];
   const uint num_gcfp  = GCFP_count_buffer[ngc_index];

   shared_ngc_index[Gindex] = ngc_index;
   shared_num_gcfp[Gindex]  = num_gcfp;
 }

 GroupMemoryBarrierWithGroupSync();

 const float3 v_xi = fp_pos_buffer[cur_fp_index];

 Neighbor_Information ninfos[27];
 uint nbr_count = 0;

 for (uint i = 0; i < num_ngc; ++i)
 {
   const uint ngc_index = shared_ngc_index[i];
   const uint num_gcfp  = shared_num_gcfp[i];

   if (num_gcfp <= Gindex)
    continue;

  const uint nfp_index = fp_index_buffer[ngc_index * g_estimated_num_gcfp + Gindex];
  const float3 v_xj = fp_pos_buffer[nfp_index];
  const float3 v_xij = v_xi - v_xj;
  const float distance = length(v_xij);

  if (g_divide_length < distance)
    continue;

  Neighbor_Information info;
  info.fp_index = nfp_index;
  info.tvector = v_xij;
  info.distance = distance;

  ninfos[nbr_count++] = info;
 }

 shared_index[Gindex+1] = nbr_count;

 GroupMemoryBarrierWithGroupSync();

 if (Gindex==0)
 {
    shared_index[0] = 0;

    for (uint j=0; j<NUM_THREAD; ++j)
      shared_index[j+1] += shared_index[j];

    nfp_count_buffer[cur_fp_index] = shared_index[NUM_THREAD];
 }

 GroupMemoryBarrierWithGroupSync();
 
 const uint start_index = cur_fp_index * g_estimated_num_nfp + shared_index[Gindex];
 for (uint j=0; j<nbr_count; ++j)
    nfp_info_buffer[start_index + j] = ninfos[j];
}

// //Version 12
// #define NUM_THREAD 64
// #define NUM_MAX_GROUP 65535

// #include "uniform_grid_common.hlsli"
// #include "uniform_grid_output.hlsli"

// StructuredBuffer<float3> fp_pos_buffer : register(t0);
// StructuredBuffer<uint> fp_index_buffer : register(t1);
// StructuredBuffer<uint> GCFP_count_buffer : register(t2);
// StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
// StructuredBuffer<uint> ngc_index_buffer : register(t4);
// StructuredBuffer<uint> ngc_count_buffer : register(t5);

// RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
// RWStructuredBuffer<uint> nfp_count_buffer : register(u1);

// groupshared uint shared_fp_index[2700];
// groupshared uint shared_ngc_index[27];
// groupshared uint shared_num_gcfp[27];

// [numthreads(NUM_THREAD, 1, 1)]
// void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
// {
//  const uint cur_fp_index = Gid.x + Gid.y * NUM_MAX_GROUP; 

//  if (g_num_particle <= cur_fp_index)
//    return;

//  if (Gindex == 0)
//    nfp_count_buffer[cur_fp_index] = 0;

//  const GCFP_ID cur_id       = GCFP_ID_buffer[cur_fp_index];
//  const uint cur_gc_index    = to_GCindex(cur_id.GCid);
//  const uint num_ngc         = ngc_count_buffer[cur_gc_index];

//  if (Gindex < num_ngc)
//  {
//    const uint ngc_index = ngc_index_buffer[cur_gc_index * g_estimated_num_ngc + Gindex];
//    const uint num_gcfp  = GCFP_count_buffer[ngc_index];

//    shared_ngc_index[Gindex] = ngc_index;
//    shared_num_gcfp[Gindex]  = num_gcfp;
//  }

//  GroupMemoryBarrierWithGroupSync();

//  for (uint i = 0; i < num_ngc; ++i)
//  {
//    const uint ngc_index = shared_ngc_index[i];
//    const uint num_gcfp  = shared_num_gcfp[i];

//    if (Gindex < num_gcfp)
//      shared_fp_index[i * g_estimated_num_gcfp + Gindex] = fp_index_buffer[ngc_index * g_estimated_num_gcfp + Gindex]; 
//  } 

//  GroupMemoryBarrierWithGroupSync();

//  const float3 v_xi = fp_pos_buffer[cur_fp_index];

//  for (uint i = 0; i < num_ngc; ++i)
//  {
//    if (Gindex < shared_num_gcfp[i])
//    {
//      const uint nfp_index   = shared_fp_index[i * g_estimated_num_gcfp + Gindex];
//      const float3 v_xj      = fp_pos_buffer[nfp_index];
//      const float3 v_xij     = v_xi - v_xj;
//      const float distance   = length(v_xij);

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
// }

////Version 11
//#define NUM_THREAD 64
//#define NUM_MAX_GROUP 65535
//
//#include "uniform_grid_common.hlsli"
//#include "uniform_grid_output.hlsli"
//
//StructuredBuffer<float3> fp_pos_buffer : register(t0);
//StructuredBuffer<uint> fp_index_buffer : register(t1);
//StructuredBuffer<uint> GCFP_count_buffer : register(t2);
//StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
//StructuredBuffer<uint> ngc_index_buffer : register(t4);
//StructuredBuffer<uint> ngc_count_buffer : register(t5);
//
//RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
//RWStructuredBuffer<uint> nfp_count_buffer : register(u1);
//
//groupshared uint shared_ngc_index[27];
//groupshared uint shared_num_gcfp[27];
//
//[numthreads(NUM_THREAD, 1, 1)]
//void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
//{
// const uint cur_fp_index = Gid.x + Gid.y * NUM_MAX_GROUP; 
//
// if (g_num_particle <= cur_fp_index)
//   return;
//
// if (Gindex == 0)
//   nfp_count_buffer[cur_fp_index] = 0;
//
// const GCFP_ID cur_id       = GCFP_ID_buffer[cur_fp_index];
// const uint cur_gc_index    = to_GCindex(cur_id.GCid);
// const uint num_ngc         = ngc_count_buffer[cur_gc_index];
//
// if (Gindex < num_ngc)
// {
//   const uint ngc_index = ngc_index_buffer[cur_gc_index * g_estimated_num_ngc + Gindex];
//   const uint num_gcfp  = GCFP_count_buffer[ngc_index];
//
//   shared_ngc_index[Gindex] = ngc_index;
//   shared_num_gcfp[Gindex]  = num_gcfp;
// }
//
// GroupMemoryBarrierWithGroupSync();
//
// const float3 v_xi = fp_pos_buffer[cur_fp_index];
//
// for (uint i = 0; i < num_ngc; ++i)
// {
//   const uint ngc_index = shared_ngc_index[i];
//   const uint num_gcfp  = shared_num_gcfp[i];
//
//   if (num_gcfp <= Gindex)
//    continue;
//
//  const uint nfp_index = fp_index_buffer[ngc_index * g_estimated_num_gcfp + Gindex];
//  const float3 v_xj = fp_pos_buffer[nfp_index];
//  const float3 v_xij = v_xi - v_xj;
//  const float distance = length(v_xij);
//
//  if (g_divide_length < distance)
//    continue;
//
//  Neighbor_Information info;
//  info.fp_index = nfp_index;
//  info.tvector = v_xij;
//  info.distance = distance;
//
//  uint nbr_count;
//  InterlockedAdd(nfp_count_buffer[cur_fp_index], 1, nbr_count);
//
//  nfp_info_buffer[cur_fp_index * g_estimated_num_nfp + nbr_count] = info;
// }
//}


// // Original
// #define NUM_THREAD 64
// #define NUM_MAX_GROUP 65535

// #include "uniform_grid_common.hlsli"
// #include "uniform_grid_output.hlsli"

// StructuredBuffer<float3> fp_pos_buffer : register(t0);
// StructuredBuffer<uint> fp_index_buffer : register(t1);
// StructuredBuffer<uint> GCFP_count_buffer : register(t2);
// StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
// StructuredBuffer<uint> ngc_index_buffer : register(t4);
// StructuredBuffer<uint> ngc_count_buffer : register(t5);

// RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
// RWStructuredBuffer<uint> nfp_count_buffer : register(u1);

// [numthreads(NUM_THREAD, 1, 1)]
// void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
// {
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
// }


////Version1
// #define NUM_THREAD 64
// #define NUM_MAX_GROUP 65535

// #include "uniform_grid_common.hlsli"
// #include "uniform_grid_output.hlsli"

// //shader resource
// StructuredBuffer<float3> fp_pos_buffer : register(t0);
// StructuredBuffer<uint> fp_index_buffer : register(t1);
// StructuredBuffer<uint> GCFP_count_buffer : register(t2);
// StructuredBuffer<GCFP_ID> GCFP_ID_buffer : register(t3);
// StructuredBuffer<uint> ngc_index_buffer : register(t4);
// StructuredBuffer<uint> ngc_count_buffer : register(t5);

// //unordered access
// RWStructuredBuffer<Neighbor_Information> nfp_info_buffer : register(u0);
// RWStructuredBuffer<uint> nfp_count_buffer : register(u1);

// struct debug_struct
// {
//   uint3 cur_GCid;
//   uint3 neighbor_GCids[27];
//   uint valid_count;
// };

// RWStructuredBuffer<debug_struct> debug_buffer : register(u2);


// //entry function
// [numthreads(NUM_THREAD, 1, 1)]
// void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
// {
//   const uint cur_fp_index = Gid.x + Gid.y * NUM_MAX_GROUP;

//   if (g_num_particle <= cur_fp_index)
//     return;

//   if (Gindex == 0)
//     nfp_count_buffer[cur_fp_index] = 0;

//   const GCFP_ID cur_id = GCFP_ID_buffer[cur_fp_index];
//   const uint3 cur_GCid = cur_id.GCid;
//   const uint cur_gc_index = to_GCindex(cur_id.GCid);
  
//   const float3 v_xi = fp_pos_buffer[cur_fp_index];
  
//   for (uint i = 0; i < g_estimated_num_ngc; ++i)
//   {
//     const uint3 nbr_GCid = find_neighbor_GCid(cur_GCid, i);

//     if (!is_valid_GCid(nbr_GCid))
//       continue;
    
//     const uint nbr_gc_index = to_GCindex(nbr_GCid);
//     const uint num_gcfp     = GCFP_count_buffer[nbr_gc_index];

//     if (num_gcfp <= Gindex)
//       continue;

//     const uint nbr_GCindex = to_GCindex(nbr_GCid);
//     const uint nbr_FPindex = fp_index_buffer[nbr_GCindex * g_estimated_num_gcfp + Gindex];
    
//     const float3 v_xj     = fp_pos_buffer[nbr_FPindex];
//     const float3 v_xij    = v_xi - v_xj;
//     const float distance  = length(v_xij);

//     if (g_divide_length < distance)
//       continue;
  
//     Neighbor_Information info;
//     info.fp_index = nbr_FPindex;
//     info.tvector = v_xij;
//     info.distance = distance;

//     uint nbr_count;
//     InterlockedAdd(nfp_count_buffer[cur_fp_index], 1, nbr_count);

//     nfp_info_buffer[cur_fp_index * g_estimated_num_nfp + nbr_count] = info;
//   }
// }