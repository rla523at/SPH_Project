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

struct Initial_Condition_Dam
{
  Domain dam;
  float  division_length = 0.0f;
};

struct Initial_Condition_Double_Dam
{
  Domain dam1;
  Domain dam2;
  float  division_length = 0.0f;
};

class Fluid_Particles
{
public:
  Fluid_Particles(
	  const Material_Property& property, 
	  const Initial_Condition_Dam& initial_condition, 
	  const Domain& solution_domain);

  Fluid_Particles(
	  const Material_Property& property, 
	  const Initial_Condition_Double_Dam& initial_condition, 
	  const Domain& domain);
  
  ~Fluid_Particles(void);

public:
  void update(void);

public:
  const Vector3* position_data(void) const;
  size_t         num_particle(void) const;
  float          support_length(void) const;

private:
  void update_density_and_pressure(void);
  void update_acceleration(void);
  void apply_boundary_condition(void);

  void time_integration(void);
  void semi_implicit_euler(const float dt);
  void leap_frog_DKD(const float dt);
  void leap_frog_KDK(const float dt);

  float W(const float q) const; //kernel function
  float dW_dq(const float q) const;

  float cal_mass_per_particle_number_density_mean(void) const;
  float cal_mass_per_particle_number_density_max(void) const;
  float cal_mass_per_particle_number_density_min(void) const;
  float cal_mass_per_particle_1994_monaghan(const float total_volume) const;

private:
  size_t _num_particle      = 0;
  float  _support_length    = 0.0f;
  float  _mass_per_particle = 0.0f;

  std::vector<Vector3> _position_vectors;
  std::vector<Vector3> _velocity_vectors;
  std::vector<Vector3> _accelaration_vectors;
  std::vector<float>   _densities;
  std::vector<float>   _pressures;

  std::vector<Vector3> _leap_frog;

  Material_Property             _material_proeprty;
  Domain                        _domain;
  std::unique_ptr<Neighborhood> _uptr_neighborhood;
};

} // namespace ms
