cbuffer Uniform_Grid_Constants : register(b0)
{
    uint num_x_cell;
    uint num_y_cell;
    uint num_z_cell;
    float domain_x_start;
    float domain_y_start;
    float domain_z_start;
    float divide_length;
}

uint grid_cell_index(float3 position)
{
    uint x_index = (uint) ((position.x - domain_x_start) / divide_length);
    uint y_index = (uint) ((position.y - domain_y_start) / divide_length);
    uint z_index = (uint) ((position.z - domain_z_start) / divide_length);

    return x_index + y_index * num_x_cell + z_index * num_x_cell * num_y_cell;
}