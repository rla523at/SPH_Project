#pragma once
#include "SPH_Common_Data.h"

#include <directxtk/SimpleMath.h>
#include <vector>
#include <memory>

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
  float sqaure_sound_speed   = 0.0f;
  float rest_density         = 0.0f;
  float gamma                = 0.0f; // Tait's equation parameter
  float pressure_coefficient = 0.0f;
  float viscosity            = 0.0f;
};

struct Initial_Condition_Cubes
{
  std::vector<Domain> domains;
  float               division_length = 0.0f;
};

class Fluid_Particles
{
public:
  Fluid_Particles(
    const Material_Property&       property,
    const Initial_Condition_Cubes& initial_condition,
    const Domain&                  solution_domain);

  ~Fluid_Particles(void);

public:
  void update(void);

public:
  const Vector3* fluid_particle_position_data(void) const;
  const Vector3* boundary_particle_position_data(void) const;
  size_t         num_fluid_particle(void) const;
  size_t         num_boundary_particle(void) const;
  float          support_length(void) const;

private:
  void update_density_and_pressure(void);
  void update_acceleration(void);
  //void apply_boundary_condition(void);

  void time_integration(void);
  void semi_implicit_euler(const float dt);
  void leap_frog_DKD(const float dt);
  void leap_frog_KDK(const float dt);

  float W(const float q) const; //kernel function
  float dW_dq(const float q) const;
  float B(const float dist) const; //boundary function

  void  init_mass(void);
  float cal_mass_per_particle_number_density_mean(void) const;
  float cal_mass_per_particle_number_density_max(void) const;
  float cal_mass_per_particle_number_density_min(void) const;
  float cal_mass_per_particle_1994_monaghan(const float total_volume) const;

  void init_boundary_position_and_normal(const Domain& solution_domain, const float divide_length);

private:
  size_t _num_fluid_particle = 0;
  float  _support_length     = 0.0f;
  float  _mass_per_particle  = 0.0f;

  std::vector<float> _mass;

  std::vector<Vector3> _fluid_position_vectors;
  std::vector<Vector3> _fluid_velocity_vectors;
  std::vector<Vector3> _fluid_acceleration_vectors;
  std::vector<float>   _fluid_densities;
  std::vector<float>   _fluid_pressures;

  std::vector<Vector3> _boundary_position_vectors;
  std::vector<Vector3> _boundary_normal_vectors;

  Material_Property             _material_proeprty;
  Domain                        _domain;
  std::unique_ptr<Neighborhood> _uptr_neighborhood;
};

} // namespace ms
