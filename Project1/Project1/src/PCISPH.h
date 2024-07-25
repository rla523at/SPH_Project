#pragma once
#include "SPH_Common_Data.h"

#include <memory>

//Forward Declaration
namespace ms
{
class Neighborhood;
class Cubic_Spline_Kernel;
}

//PCISPH declration
namespace ms
{

class PCISPH
{
public:
  PCISPH(const Initial_Condition_Cubes& initial_condition,
         const Domain&                  solution_domain);

public:
  void update(void);

private:
  //it doesn't consider acceleration by pressure
  void initialize_fluid_acceleration_vectors(void);

  void  initialize_pressure_and_pressure_acceleration(void);
  void  predict_velocity_and_position(void);
  float predict_density_and_variation_and_update_pressure(void);
  void  cal_pressure_acceleration(void);
  void  apply_boundary_condition(void);
  float init_mass_and_scailing_factor(void);

private:
  float _scailing_factor   = 0.0f;
  float _mass_per_particle = 0.0f;
  float _smoothing_length  = 0.0f;
  float _viscosity         = 0.0f;
  float _rest_density      = 0.0f;
  float _dt                = 0.0f;
  float _particle_radius   = 0.0f;

  Fluid_Particles                      _fluid_particles;
  std::unique_ptr<Neighborhood>        _uptr_neighborhood;
  std::unique_ptr<Cubic_Spline_Kernel> _uptr_kernel;
  Domain                               _domain;

  std::vector<Vector3> _pressure_acceleration_vectors;
  std::vector<Vector3> _current_position_vectors;
  std::vector<Vector3> _current_velocity_vectors;

  //Temp
  std::vector<Vector3> _boundary_position_vectors;
};

} // namespace ms