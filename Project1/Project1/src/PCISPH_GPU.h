#pragma once
#include "Buffer_Set.h"
#include "SPH_Common_Data.h"
#include "SPH_Scheme.h"

#include <d3d11.h>
#include <memory>

// Forward Declaration
namespace ms
{
class Neighborhood_Uniform_Grid_GPU;
class Cubic_Spline_Kernel;
class Device_Manager;
} // namespace ms

namespace ms
{

struct PCISPH_Constant_Data
{
  float dt                  = 0.0f;
  float rest_density        = 0.0f;
  float viscosity           = 0.0f;
  float allow_density_error = 0.0f;
  float particle_radius     = 0.0f;
  float smoothing_length    = 0.0f;
  float mass_per_particle   = 0.0f;
  UINT  min_iter            = 0;
  UINT  max_iter            = 0;
};

struct PCISPH_Constant_Data2
{
  float scailing_factor = 0.0f;
};

} // namespace ms

// PCISPH_GPU declration
namespace ms
{

class PCISPH_GPU : public SPH_Scheme
{
public:
  PCISPH_GPU(const Initial_Condition_Cubes& initial_condition,
             const Domain&                  solution_domain,
             const Device_Manager&          device_manager);
  ~PCISPH_GPU();

public:
  void update(void) override;

public:
  float          particle_radius(void) const override;
  size_t         num_fluid_particle(void) const override;
  const Vector3* fluid_particle_position_data(void) const override;
  const float*   fluid_particle_density_data(void) const override;

private:
  void apply_boundary_condition(void);

  //////////////////////////////////////////////////////////////////////////

private:
  void  update_number_density(void);
  float cal_mass(void);
  void  update_scailing_factor(void);

  // it doesn't consider acceleration by pressure
  void init_fluid_acceleration(void);

  void init_pressure_and_a_pressure(void);

  void  predict_velocity_and_position(void);
  void  predict_density_error_and_update_pressure(void);
  float cal_max_density_error(void);
  void  update_a_pressure(void);

private:
  float  _time                 = 0.0f;
  float  _scailing_factor      = 0.0f;
  float  _mass_per_particle    = 0.0f;
  float  _smoothing_length     = 0.0f;
  float  _rest_density         = 0.0f;
  float  _dt                   = 0.0f;
  float  _particle_radius      = 0.0f;
  float  _allow_density_error  = 0.0f;
  float  _max_density_variance = 0.0f;
  size_t _max_iter             = 0;
  size_t _min_iter             = 3;

  Fluid_Particles                                _fluid_particles;
  std::unique_ptr<Neighborhood_Uniform_Grid_GPU> _uptr_neighborhood;
  Domain                                         _domain;

  //
  std::vector<float> _max_density_errors;

  // Temp
  std::vector<Vector3> _boundary_position_vectors;

  //////////////////////////////////////////////////////////////////////////
  UINT  _num_fluid_particle = 0;
  float _rho0               = 0.0f; // rest density
  float _m0                 = 0.0f; // mass per particle
  float _h                  = 0.0f; // smoothing length
  float _viscosity          = 0.0f;

  const Device_Manager* _DM_ptr = nullptr;

  ComPtr<ID3D11Buffer> _cptr_cubic_spline_kerenel_CB; // constant buffer

  // fluid particle만큼 position vector를 저장한 buffer set
  Buffer_Set _fluid_v_pos_BS;

  ComPtr<ID3D11Buffer>             _cptr_fluid_v_cur_pos_buffer;
  ComPtr<ID3D11ShaderResourceView> _cptr_fluid_v_cur_pos_buffer_SRV;

  // fluid particle만큼 velocity vector를 저장한 buffer set
  Buffer_Set _fluid_v_vel_BS;

  ComPtr<ID3D11Buffer>             _cptr_fluid_v_cur_vel_buffer;
  ComPtr<ID3D11ShaderResourceView> _cptr_fluid_v_cur_vel_buffer_SRV;

  // fluid particle만큼 acceleration vector를 저장한 buffer
  ComPtr<ID3D11Buffer>              _cptr_fluid_v_accel_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_fluid_v_accel_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_fluid_v_accel_buffer_UAV;

  // fluid particle만큼 acceleration by pressure vector를 저장한 buffer
  Buffer_Set _fluid_v_a_pressure_BS;

  // fluid particle만큼 density를 저장한 buffer set
  Buffer_Set _fluid_density_BS;

  // fluid particle만큼 pressure를 저장한 buffer
  Buffer_Set _fluid_pressure_BS;

  // fluid particle만큼 number density를 저장한 buffer
  ComPtr<ID3D11Buffer>              _cptr_number_density_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_number_density_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_number_density_buffer_UAV;

  // fluid particle만큼 density error를 저장한 buffer set
  Buffer_Set           _fluid_density_error_BS;
  ComPtr<ID3D11Buffer> _cptr_fluid_density_error_intermediate_buffer;

  // scailing factor를 저장한 buffer
  Buffer_Set _scailing_factor_BS;

  // update number density
  ComPtr<ID3D11ComputeShader> _cptr_update_number_density_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_number_density_CS_CB;

  // cal scailing factor
  ComPtr<ID3D11ComputeShader> _cptr_update_scailing_factor_CS;
  ComPtr<ID3D11Buffer>        _cptr_cal_scailing_factor_CS_CB;

  // init fluid acceleration
  ComPtr<ID3D11ComputeShader> _cptr_init_fluid_acceleration_CS;
  ComPtr<ID3D11Buffer>        _cptr_init_fluid_acceleration_CS_CB;

  // init pressure and a_pressure
  ComPtr<ID3D11ComputeShader> _cptr_init_pressure_and_a_pressure_CS;
  ComPtr<ID3D11Buffer>        _cptr_init_pressure_and_a_pressure_CS_CB;

  // predict vel and pos
  ComPtr<ID3D11ComputeShader> _cptr_predict_vel_and_pos_CS;
  ComPtr<ID3D11Buffer>        _cptr_predict_vel_and_pos_CS_CB;

  // predict density error and update pressure
  ComPtr<ID3D11ComputeShader> _cptr_predict_density_error_and_update_pressure_CS;
  ComPtr<ID3D11Buffer>        _cptr_predict_density_error_and_update_pressure_CS_CB;

  // update a_pressure
  ComPtr<ID3D11ComputeShader> _cptr_update_a_pressure_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_a_pressure_CS_CB;

  // Apply BC
  ComPtr<ID3D11ComputeShader> _cptr_apply_BC_CS;
  ComPtr<ID3D11Buffer>        _cptr_apply_BC_CS_CB;
};

} // namespace ms