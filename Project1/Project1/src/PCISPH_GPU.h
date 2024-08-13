#pragma once
#include "Buffer_Set.h"
#include "SPH_Common_Data.h"
#include "SPH_Scheme.h"

#include <d3d11.h>
#include <memory>

/*  Abbreviation
    BC  : Boundary Condition
    FP  : Fluid Particle
*/

#define PCISPH_GPU_PERFORMANCE_ANALYSIS

// Forward Declaration
namespace ms
{
class Neighborhood_Uniform_Grid_GPU;
class Device_Manager;
} // namespace ms

// PCISPH_GPU declration
namespace ms
{

class PCISPH_GPU : public SPH_Scheme
{
public:
  static void print_performance_analysis_result(void);
  static void print_performance_analysis_result_avg(const UINT num_frame);

public:
  PCISPH_GPU(const Initial_Condition_Cubes& initial_condition,
             const Domain&                  solution_domain,
             Device_Manager&                device_manager);
  ~PCISPH_GPU();

public:
  void update(void) override;

public:
  float                        particle_radius(void) const override;
  size_t                       num_fluid_particle(void) const override;
  const Read_Write_Buffer_Set& get_fluid_v_pos_BS(void) const override;
  const Read_Write_Buffer_Set& get_fluid_density_BS(void) const override;

private:
  void  apply_BC(void);
  float cal_mass(const float rho0);
  float cal_max_density_error(void);
  void  copy_cur_pos_and_vel(void);
  void  init_fluid_acceleration(void); // it doesn't consider acceleration by pressure
  void  init_pressure_and_a_pressure(void);
  void  init_ncount(void);
  void  predict_velocity_and_position(void);
  void  predict_density_error_and_update_pressure(void);
  void  update_number_density(void);
  void  update_scailing_factor(void);
  void  update_a_pressure(void);
  void  update_neighborhood(void);
  void  update_ninfo(void);
  void  update_W(void);
  void  update_grad_W(void);
  void  update_laplacian_vel_coeff(void);
  void  update_a_pressure_coeff(void);

private:
  float  _dt                  = 0.0f;
  float  _allow_density_error = 0.0f;
  float  _particle_radius     = 0.0f;
  size_t _min_iter            = 0;
  size_t _max_iter            = 0;
  UINT   _num_FP              = 0;

  Device_Manager* _DM_ptr = nullptr;

  float _time = 0.0f;

  // fluid particle만큼 data를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _fluid_v_pos_RWBS;
  Read_Write_Buffer_Set _fluid_v_cur_pos_RWBS;
  Read_Write_Buffer_Set _fluid_v_vel_RWBS;
  Read_Write_Buffer_Set _fluid_v_cur_vel_RWBS;
  Read_Write_Buffer_Set _fluid_v_accel_RWBS;
  Read_Write_Buffer_Set _fluid_v_a_pressure_RWBS; // acceleration vector by pressure
  Read_Write_Buffer_Set _fluid_density_RWBS;
  Read_Write_Buffer_Set _fluid_pressure_RWBS;
  Read_Write_Buffer_Set _fluid_number_density_RWBS;
  Read_Write_Buffer_Set _fluid_density_error_RWBS;

  Read_Write_Buffer_Set _ninfo_RWBS;               // fluid particle * estimated_num_nfp만큼 neighbor information을 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _ncount_RWBS;              // fluid particle만큼 neighbor의 수를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _W_RWBS;                   // fluid particle * estimated_num_nfp만큼 Kernel 함수의 값을 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _v_grad_W_RWBS;              // fluid particle * estimated_num_nfp만큼 gradient Kernel 함수의 값을 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _laplacian_vel_coeff_RWBS; // fluid particle * estimated_num_nfp만큼 laplacian velocity coeff 값을 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _a_pressure_coeff_RWBS;    // fluid particle * estimated_num_nfp만큼 a_pressure coeff 값을 저장한 STRB의 RWBS

  // scailing factor를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _scailing_factor_RWBS;

  // for optimization
  ComPtr<ID3D11Buffer> _cptr_fluid_density_error_intermediate_buffer;
  ComPtr<ID3D11Buffer> _cptr_max_density_error_STGB;

  // cubic spline kernel
  ComPtr<ID3D11Buffer> _cptr_cubic_spline_kerenel_ICONB;

  // neighborhood search
  std::unique_ptr<Neighborhood_Uniform_Grid_GPU> _uptr_neighborhood;

  // update number density
  ComPtr<ID3D11ComputeShader> _cptr_update_number_density_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_number_density_CS_CONB;

  // cal scailing factor
  ComPtr<ID3D11ComputeShader> _cptr_update_scailing_factor_CS;
  ComPtr<ID3D11Buffer>        _cptr_cal_scailing_factor_CS_ICONB;

  // init fluid acceleration
  ComPtr<ID3D11ComputeShader> _cptr_init_fluid_acceleration_CS;
  ComPtr<ID3D11Buffer>        _cptr_init_fluid_acceleration_CS_CONB;

  // init pressure and a_pressure
  ComPtr<ID3D11ComputeShader> _cptr_init_pressure_and_a_pressure_CS;
  ComPtr<ID3D11Buffer>        _cptr_init_pressure_and_a_pressure_CS_CONB;

  // predict vel and pos
  ComPtr<ID3D11ComputeShader> _cptr_predict_vel_and_pos_CS;
  ComPtr<ID3D11Buffer>        _cptr_predict_vel_and_pos_CS_CONB;

  // predict density error and update pressure
  ComPtr<ID3D11ComputeShader> _cptr_predict_density_error_and_update_pressure_CS;
  ComPtr<ID3D11Buffer>        _cptr_predict_density_error_and_update_pressure_CS_CONB;

  // update a_pressure
  ComPtr<ID3D11ComputeShader> _cptr_update_a_pressure_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_a_pressure_CS_ICONB;

  // update ninfo
  ComPtr<ID3D11ComputeShader> _cptr_update_ninfo_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_ninfo_CS_ICONB;

  // update W
  ComPtr<ID3D11ComputeShader> _cptr_update_W_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_W_CS_ICONB;

  // update grad_W
  ComPtr<ID3D11ComputeShader> _cptr_update_grad_W_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_grad_W_CS_ICONB;

  // update laplacian_vel_coeff
  ComPtr<ID3D11ComputeShader> _cptr_update_laplacian_vel_coeff_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_laplacian_vel_coeff_CS_ICONB;

  // update a_pressure coeff
  ComPtr<ID3D11ComputeShader> _cptr_update_a_pressure_coeff_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_a_pressure_coeff_CS_ICONB;

  // Apply BC
  ComPtr<ID3D11ComputeShader> _cptr_apply_BC_CS;
  ComPtr<ID3D11Buffer>        _cptr_apply_BC_CS_CONB;

  // init ncount
  ComPtr<ID3D11ComputeShader> _cptr_init_ncount_CS;
  ComPtr<ID3D11Buffer>        _cptr_init_ncount_CS_CONB;

  // performance analysis

  static inline float _dt_sum_update                                    = 0.0f;
  static inline float _dt_sum_update_neighborhood                       = 0.0f;
  static inline float _dt_sum_update_scailing_factor                    = 0.0f;
  static inline float _dt_sum_update_ninfo                              = 0.0f;
  static inline float _dt_sum_update_W                                  = 0.0f;
  static inline float _dt_sum_update_grad_W                             = 0.0f;
  static inline float _dt_sum_update_laplacian_vel_coeff                = 0.0f;
  static inline float _dt_sum_update_a_pressure_coeff                   = 0.0f;
  static inline float _dt_sum_init_fluid_acceleration                   = 0.0f;
  static inline float _dt_sum_init_pressure_and_a_pressure              = 0.0f;
  static inline float _dt_sum_copy_cur_pos_and_vel                      = 0.0f;
  static inline float _dt_sum_predict_velocity_and_position             = 0.0f;
  static inline float _dt_sum_predict_density_error_and_update_pressure = 0.0f;
  static inline float _dt_sum_cal_max_density_error                     = 0.0f;
  static inline float _dt_sum_update_a_pressure                         = 0.0f;
  static inline float _dt_sum_apply_BC                                  = 0.0f;
};

} // namespace ms