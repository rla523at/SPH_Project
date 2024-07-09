#pragma once
#include <directxtk/SimpleMath.h>
#include <vector>

namespace ms
{
using DirectX::SimpleMath::Vector3;
}

namespace ms
{
struct Material_Property
{
  float rest_density         = 0.0f;
  float pressure_coefficient = 0.0f;
  float viscosity            = 0.0f;
};

struct Initial_Condition_Cube
{
  Vector3 init_pos;
  float   edge_length          = 0.0f;
  size_t  num_particle_in_edge = 0;
};

class Fluid_Particles
{
public:
  Fluid_Particles(const Material_Property& property, const Initial_Condition_Cube& initial_condition);

public:
  void update(void);

public:
  const Vector3* position_data(void) const;
  size_t         num_particle(void) const;

private:
  void update_density_with_clamp(void);
  void update_density(void);
  void update_pressure(void);
  void update_force(void);
  void time_integration(void);
  void apply_boundary_condition(void);

  float W(const float q) const; //kernel function
  float dW_dq(const float q) const;

  float cal_mass_per_particle_1994_monaghan(const float total_volume, const size_t num_particle) const;

private:
  size_t _num_particle      = 0;
  float  _total_volume      = 0.0f;
  float  _support_length    = 0.0f;
  float  _mass_per_particle = 0.0f;

  std::vector<Vector3> _position_vectors;
  std::vector<Vector3> _velocity_vectors;
  std::vector<Vector3> _force_vectors;
  std::vector<float>   _densities;
  std::vector<float>   _pressures;

  Material_Property _material_proeprty;
};

} // namespace ms
