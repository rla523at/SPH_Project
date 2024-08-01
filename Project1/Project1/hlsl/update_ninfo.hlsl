#define ESTIMATED_NUM_NEIGHBOR 200

// struct declration
struct Neighbor_Informations
{
  uint indexes[ESTIMATED_NUM_NEIGHBOR];
  float3 translate_vectors[ESTIMATED_NUM_NEIGHBOR];
  float distances[ESTIMATED_NUM_NEIGHBOR];
  uint num_neighbor = 0;
};

struct GCFPT_ID
{
  uint gc_index;
  uint gcfp_index;
};

//constant buffer
cbuffer Constant_Buffer : register(b0)
{
  float supprot_length;
}

//shader resource
StructuredBuffer<float3>    fp_pos_buffer       : register(t0);
Texture2D<uint>             GCFP_texture        : register(t1);
StructuredBuffer<uint>      GCFP_counter_buffer : register(t2);
StructuredBuffer<GCFPT_ID>  GCFP_ID_buffer      : register(t3);
Texture2D<uint>             ngc_texture         : register(t4);

//unordered access
RWStructuredBuffer<Neighbor_Informations> ninfos_buffer : register(u0);

// user define fuction
uint2 to_uint2(const GCFPT_ID id)
{
  return uint2(id.gc_index, id.gcfp_index);
}

//entry function
[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{
  uint num_data, size;
  fp_pos_buffer.GetDimensions(num_data, size);

  const uint cur_fp_index = DTID.x;
        
  if (num_data < cur_fp_index)
    return;
    
  const GCFPT_ID    cur_id          = GCFP_ID_buffer[cur_fp_index];
  const uint        cur_gc_index    = cur_id.gc_index;
  
  const float3 v_xi = fp_pos_buffer[cur_fp_index];
    
  

  uint neighbor_count = 0;
  const uint num_neighbor = GCFP_counter_buffer[cur_gc_index];
  for (uint i = 0; i < num_neighbor; ++i)
  {
    const uint neighbor_fp_index = GCFP_texture[uint2(cur_gc_index, i)];
    
    const float3 v_xj = fp_pos_buffer[neighbor_fp_index];
    const float3 v_xij = v_xi - v_xj;
    const float distance = length(v_xij);
    
    if (supprot_length < distance)
      continue;
    
    ninfos_buffer[cur_fp_index].indexes[neighbor_count] = neighbor_fp_index;
    ninfos_buffer[cur_fp_index].translate_vectors[neighbor_count] = v_xij;
    ninfos_buffer[cur_fp_index].distances[neighbor_count] = distance;
    
    ++neighbor_count;
  }
  
  ninfos_buffer[cur_fp_index].num_neighbor = neighbor_count;
}
