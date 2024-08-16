cbuffer Common_CB : register(b0)
{
  uint  g_num_x_cell;
  uint  g_num_y_cell;
  uint  g_num_z_cell;
  uint  g_num_cell;
  uint  g_num_particle;
  uint  g_estimated_num_ngc;
  uint  g_estimated_num_gcfp;
  uint  g_estimated_num_nfp;
  float g_domain_x_start;
  float g_domain_y_start;
  float g_domain_z_start;
  float g_divide_length;
};

uint to_GCindex(uint3 GCid)
{
  return GCid.x + GCid.y * g_num_x_cell + GCid.z * g_num_x_cell * g_num_y_cell;
}

struct GCFP_ID
{
  uint3 GCid;
  uint  gcfp_index;
  
  uint GCFP_index()
  {
    return to_GCindex(GCid) * g_estimated_num_gcfp + gcfp_index;
  }
};

uint3 to_GCid(float3 position)
{
  const uint x_index = (uint) ((position.x - g_domain_x_start) / g_divide_length);
  const uint y_index = (uint) ((position.y - g_domain_y_start) / g_divide_length);
  const uint z_index = (uint) ((position.z - g_domain_z_start) / g_divide_length);

  return uint3(x_index, y_index, z_index);
}

//uint to_GCindex(float3 position)
//{
//  const uint x_index = (uint) ((position.x - g_domain_x_start) / g_divide_length);
//  const uint y_index = (uint) ((position.y - g_domain_y_start) / g_divide_length);
//  const uint z_index = (uint) ((position.z - g_domain_z_start) / g_divide_length);
//
//  return x_index + y_index * g_num_x_cell + z_index * g_num_x_cell * g_num_y_cell;
//}

uint3 find_neighbor_GCid(const uint3 GCid, uint ngc_index)
{
  const int3 delta = int3(-1, 0, 1);
  
  uint3 index_ternary;
  index_ternary.x = ngc_index % 3;
  ngc_index /= 3;
  index_ternary.z = ngc_index % 3;
  ngc_index /= 3;  
  index_ternary.y = ngc_index % 3;  
  
  uint3 nbr_GC_id;
  
  [unroll]
  for (uint i = 0; i < 3; ++i)
    nbr_GC_id[i] = GCid[i] + delta[index_ternary[i]];
  
  return nbr_GC_id;
}

bool is_valid_GCid(const uint3 GCid)
{
//  if (g_num_x_cell <= GCid.x)
//    return false;
//
//  if (g_num_y_cell <= GCid.y)
//    return false;
//
//  if (g_num_z_cell <= GCid.z)
//    return false;
//
//  return true;
  
  bool is_valid_GCid = true;

  if (g_num_x_cell <= GCid.x)
    is_valid_GCid = false;
  else if (g_num_y_cell <= GCid.y)
    is_valid_GCid = false;
  else if (g_num_z_cell <= GCid.z)
    is_valid_GCid = false;

  return is_valid_GCid;
}