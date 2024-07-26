#pragma once
#include "SPH_Common_Data.h"
#include "SPH_Scheme.h"

#include <memory>

//Forward Declaration
namespace ms
{
class Neighborhood;
class Cubic_Spline_Kernel;
} // namespace ms

//PCISPH declration
namespace ms
{

class PCISPH : public SPH_Scheme
{
public:
  PCISPH(const Initial_Condition_Cubes& initial_condition,
         const Domain&                  solution_domain);
  ~PCISPH();

public:
  void update(void) override;

public:
  float          particle_radius(void) const override;
  size_t         num_fluid_particle(void) const override;
  const Vector3* fluid_particle_position_data(void) const override;
  const float*   fluid_particle_density_data(void) const override;

private:
  //it doesn't consider acceleration by pressure
  void initialize_fluid_acceleration_vectors(void);

  void  initialize_pressure_and_pressure_acceleration(void);
  void  predict_velocity_and_position(void);
  float predict_density_and_update_pressure_and_cal_error(void);
  void  cal_pressure_acceleration(void);
  void  apply_boundary_condition(void);
  void  init_mass_and_scailing_factor(void);

private:
  float cal_scailing_factor(void) const;
  float cal_number_density(const size_t fluid_particle_id) const;

private:
  float _scailing_factor   = 0.0f;
  float _mass_per_particle = 0.0f;
  float _smoothing_length  = 0.0f;
  float _viscosity         = 0.0f;
  float _rest_density      = 0.0f;
  float _dt                = 0.0f;
  float _particle_radius   = 0.0f;
  float  _allow_density_error = 0.0f;
  float  _max_density_variance = 0.0f;
  size_t _max_iter          = 0;
  size_t _min_iter          = 3;
  

  Fluid_Particles                      _fluid_particles;
  std::unique_ptr<Neighborhood>        _uptr_neighborhood;
  std::unique_ptr<Cubic_Spline_Kernel> _uptr_kernel;
  Domain                               _domain;

  std::vector<Vector3> _pressure_acceleration_vectors;
  std::vector<Vector3> _current_position_vectors;
  std::vector<Vector3> _current_velocity_vectors;

  //
  std::vector<float> _max_density_errors;


  //Temp
  std::vector<Vector3> _boundary_position_vectors;
};

} // namespace ms