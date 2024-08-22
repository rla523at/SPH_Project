#pragma once
#include "SPH_Common_Data.h"
#include "SPH_Scheme.h"

#include <memory>
#include <vector>

//abbreviation

//Forward Declaration
namespace ms
{
class Neighborhood;
class Cubic_Spline_Kernel;
class Device_Manager;
} // namespace ms

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

class WCSPH : public SPH_Scheme
{
public:
  WCSPH(
    const Initial_Condition_Cubes& initial_condition,
    const Domain&                  solution_domain,
    Device_Manager&          device_manager);

  ~WCSPH(void);

public:
  void update(void) override;

public:
  float                        particle_radius(void) const override;
  size_t                       num_fluid_particle(void) const override;
  const Read_Write_Buffer_Set& get_fluid_v_pos_RWBS(void) const override;
  const Read_Write_Buffer_Set& get_fluid_density_RWBS(void) const override;

  //const Vector3* boundary_particle_position_data(void) const override;
  //size_t         num_boundary_particle(void) const override;

private:
  void update_density_and_pressure(void);
  void update_acceleration(void);
  void apply_boundary_condition(void);

  void time_integration(void);
  void semi_implicit_euler(const float dt);
  void leap_frog_DKD(const float dt);
  void leap_frog_KDK(const float dt);

  float B(const float dist) const; //boundary function

  float cal_mass_per_particle_number_density_mean(void) const;
  float cal_mass_per_particle_number_density_max(void) const;
  float cal_mass_per_particle_number_density_min(void) const;
  float cal_mass_per_particle_1994_monaghan(const float total_volume) const;
  float cal_number_density(const size_t fluid_particle_id) const;

  void init_boundary_position_and_normal(const Domain& solution_domain, const float divide_length);

private:
  size_t _num_fluid_particle  = 0;
  float  _smoothing_length    = 0.0f;
  float  _particle_radius     = 0.0f;
  float  _volume_per_particle = 0.0f;
  float  _mass_per_particle   = 0.0f;
  float  _dt                  = 0.0f;
  float  _free_surface_param  = 0.0f;

  Fluid_Particles                      _fluid_particles;
  std::unique_ptr<Neighborhood>        _uptr_neighborhood;
  std::unique_ptr<Cubic_Spline_Kernel> _uptr_kernel;
  Domain                               _domain;
  Material_Property                    _material_proeprty;

  std::vector<Vector3> _boundary_position_vectors;
  std::vector<Vector3> _boundary_normal_vectors;

  // for ouput
  Device_Manager* _DM_ptr;

  Read_Write_Buffer_Set _fluid_v_pos_RWBS;
  Read_Write_Buffer_Set _fluid_density_RWBS;
};

} // namespace ms
