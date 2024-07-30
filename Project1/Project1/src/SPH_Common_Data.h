#pragma once
#include "Abbreviation.h"

#include <vector>

namespace ms
{

struct Domain
{
  float x_start = 0.0f;
  float x_end   = 0.0f;
  float y_start = 0.0f;
  float y_end   = 0.0f;
  float z_start = 0.0f;
  float z_end   = 0.0f;

  float dx(void) const
  {
    return x_end - x_start;
  }
  float dy(void) const
  {
    return y_end - y_start;
  }
  float dz(void) const
  {
    return z_end - z_start;
  }
};

struct Initial_Condition_Cubes
{
  std::vector<Domain> domains;
  float               particle_spacing = 0.0f;

  std::vector<Vector3> cal_initial_position(void) const
  {
    const auto num_cube = domains.size();

    std::vector<size_t> num_particle_in_xs(num_cube);
    std::vector<size_t> num_particle_in_ys(num_cube);
    std::vector<size_t> num_particle_in_zs(num_cube);

    size_t num_particle = 0;

    for (size_t i = 0; i < num_cube; ++i)
    {
      const auto& cube = domains[i];

      const auto num_x = static_cast<size_t>(cube.dx() / particle_spacing) + 1;
      const auto num_y = static_cast<size_t>(cube.dy() / particle_spacing) + 1;
      const auto num_z = static_cast<size_t>(cube.dz() / particle_spacing) + 1;

      num_particle_in_xs[i] = num_x;
      num_particle_in_ys[i] = num_y;
      num_particle_in_zs[i] = num_z;

      num_particle += num_x * num_y * num_z;
    }

    std::vector<Vector3> positions_vectors(num_particle);

    size_t index = 0;
    for (size_t i = 0; i < num_cube; ++i)
    {
      const auto& cube = domains[i];

      const Vector3 v_pos_init = {cube.x_start, cube.y_start, cube.z_start};
      Vector3       v_pos      = v_pos_init;

      const auto num_x = num_particle_in_xs[i];
      const auto num_y = num_particle_in_ys[i];
      const auto num_z = num_particle_in_zs[i];

      for (size_t j = 0; j < num_y; ++j)
      {
        for (size_t k = 0; k < num_z; ++k)
        {
          for (size_t l = 0; l < num_x; ++l)
          {
            positions_vectors[index] = v_pos;
            ++index;
            v_pos.x += particle_spacing;
          }

          v_pos.x = v_pos_init.x;
          v_pos.z += particle_spacing;
        }

        v_pos.x = v_pos_init.x;
        v_pos.z = v_pos_init.z;
        v_pos.y += particle_spacing;
      }
    }

    return positions_vectors;
  }
  
};

struct Fluid_Particles
{
  std::vector<Vector3> position_vectors;
  std::vector<Vector3> velocity_vectors;
  std::vector<Vector3> acceleration_vectors;
  std::vector<float>   densities;
  std::vector<float>   pressures;

public:
  size_t num_particles(void) const
  {
    return position_vectors.size();
  }
};

struct Index_Vector
{
  size_t x = 0;
  size_t y = 0;
  size_t z = 0;
};

} // namespace ms