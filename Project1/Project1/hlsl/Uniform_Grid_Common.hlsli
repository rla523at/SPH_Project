cbuffer Uniform_Grid_Constants : register(b0)
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

struct GCFP_ID
{
  uint gc_index;
  uint gcfp_index;
  
  uint GCFP_index()
  {
    return gc_index * g_estimated_num_gcfp + gcfp_index;
  }
};

struct Changed_GCFP_ID_Data
{
  GCFP_ID prev_id;
  uint cur_gc_index;
};


uint grid_cell_index(float3 position)
{
  const uint x_index = (uint) ((position.x - g_domain_x_start) / g_divide_length);
  const uint y_index = (uint) ((position.y - g_domain_y_start) / g_divide_length);
  const uint z_index = (uint) ((position.z - g_domain_z_start) / g_divide_length);

  return x_index + y_index * g_num_x_cell + z_index * g_num_x_cell * g_num_y_cell;
}