#pragma once
#include "SPH_Common_Data.h"

#include <directxtk/SimpleMath.h>
#include <vector>

//abbreviation
using DirectX::SimpleMath::Vector3;

//Forward Declaration
namespace ms
{
class Neighborhood;
}

namespace ms
{
struct Material_Property
{
  float rest_density         = 0.0f;
  float gamma                = 0.0f; // Tait's equation parameter
  float pressure_coefficient = 0.0f;
  float viscosity            = 0.0f;
};

struct Initial_Condition_Cube
{
  Domain domain;
  size_t num_particle_in_x = 0;
  size_t num_particle_in_y = 0;
  size_t num_particle_in_z = 0;
};

class Fluid_Particles
{
public:
  Fluid_Particles(const Material_Property& property, const Initial_Condition_Cube& initial_condition, const Domain& domain);

public:
  void update(const Neighborhood& neighborhood);

  void init_mass(const Neighborhood& neighborhood);


public:
  const Vector3* position_data(void) const;
  size_t         num_particle(void) const;
  float          support_length(void) const;

  const std::vector<Vector3>& get_position_vectors(void) const;

private:
  void update_density(const Neighborhood& neighborhood);
  void update_pressure(void);
  void update_acceleration(const Neighborhood& neighborhood);
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
  std::vector<Vector3> _accelaration_vectors;
  std::vector<float>   _densities;
  std::vector<float>   _pressures;

  Material_Property _material_proeprty;
  Domain            _domain;
};

} // namespace ms
