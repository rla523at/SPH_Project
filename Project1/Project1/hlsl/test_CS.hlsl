////Version3
#define NUM_THREAD 64
#include "uniform_grid_common.hlsli"

//unordered access
RWStructuredBuffer<uint> count_buffer  : register(u0);

//entry function
[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{        
  

  for (uint i = 0; i < g_num_x_cell; ++i)
  {
    for (uint j = 0; j < g_num_y_cell; ++j)
    {
      for (uint k = 0; k < g_num_z_cell; ++k)
      {
        const auto gcid_vector = Grid_Cell_ID{i, j, k};
        const auto gc_index    = this->grid_cell_index(gcid_vector);

        UINT ngc_count = 0;

        UINT start_index = gc_index * estiamted_num_ngc;

        for (UINT p = 0; p < 3; ++p)
        {
          for (UINT q = 0; q < 3; ++q)
          {
            for (UINT r = 0; r < 3; ++r)
            {
              Grid_Cell_ID neighbor_gcid_vector;
              neighbor_gcid_vector.x = gcid_vector.x + delta[p];
              neighbor_gcid_vector.y = gcid_vector.y + delta[q];
              neighbor_gcid_vector.z = gcid_vector.z + delta[r];

              if (!this->is_valid_index(neighbor_gcid_vector))
                continue;

              ngc_indexes[start_index + ngc_count] = this->grid_cell_index(neighbor_gcid_vector);
              ++ngc_count;
            }
          }
        }

        ngc_counts[gc_index] = ngc_count;
      }
    }
  }
}