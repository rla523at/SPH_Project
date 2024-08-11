#pragma once
#include "Abbreviation.h"
#include "Buffer_Set.h"
#include "Neighborhood.h"
#include "SPH_Common_Data.h"

#include <d3d11.h>
#include <vector>

/* Abbreviation

pid	  : particle id
gc	  : geometry cell
fp    : fluid particle
nfp   : neighbor fluid particle
ngc   : neighbor geometry cell
ninfo : neighbor informations
GCFPT : GCFP texture
GCFP  : Geometry Cell Fluid Particle(fluid particle in the geometry cell)

*/

inline constexpr UINT g_estimated_num_ngc  = 27;
inline constexpr UINT g_estimated_num_gcfp = 100;
inline constexpr UINT g_estimated_num_nfp  = 200;

// #define UNIFORM_GRID_PERFORMANCE_ANALYSIS

namespace ms
{
class Device_Manager;
}

// data structure
namespace ms
{
struct Uniform_Grid_Common_CB_Data
{
  UINT  num_x_cell         = 0;
  UINT  num_y_cell         = 0;
  UINT  num_z_cell         = 0;
  UINT  num_cell           = 0;
  UINT  num_particle       = 0;
  UINT  estimated_num_ngc  = 0;
  UINT  estimated_num_gcfp = 0;
  UINT  estimated_num_nfp  = 0;
  float domain_x_start     = 0.0f;
  float domain_y_start     = 0.0f;
  float domain_z_start     = 0.0f;
  float divide_length      = 0.0f;
};

struct Neighborhood_Information
{
  Read_Buffer_Set       ngc_index_RBS;   // geometry cell * estimated ngc���� ngc index�� ������ ISTRB�� RBS
  Read_Buffer_Set       ngc_count_RBS;   // geometry cell���� neighbor grid cell�� ������ ������ ISTRB�� RBS
  Read_Write_Buffer_Set fp_index_RWBS;   // geometry cell * estimated gcfp���� fp index�� ������ STRB�� RWBS
  Read_Write_Buffer_Set GCFP_count_RWBS; // geometry cell���� �����ִ� fluid particle�� ������ ������ STRB�� RWBS
  Read_Write_Buffer_Set GCFP_ID_RWBS;    // fluid particle���� GCFP ID�� ������ STRB�� RWBS
};

} // namespace ms

namespace ms
{
class Neighborhood_Uniform_Grid_GPU
{
public:
  static void print_performance_analysis_result(void);

public:
  Neighborhood_Uniform_Grid_GPU(
    const Domain&          domain,
    const float            divide_length,
    const Read_Buffer_Set& fluid_v_pos_RBS,
    const UINT             num_fp,
    const Device_Manager&  device_manager);

public:
  void update(const Read_Buffer_Set& fluid_v_pos_RBS);

  const Neighborhood_Information get_neighborhood_info(void) const;

private:
  Grid_Cell_ID grid_cell_id(const Vector3& v_pos) const;
  UINT         grid_cell_index(const Grid_Cell_ID& index_vector) const;
  bool         is_valid_index(const Grid_Cell_ID& index_vector) const;

  void find_changed_GCFPT_ID(const Read_Buffer_Set& fluid_v_pos_RBS);
  void update_GCFP(void);
  void rearrange_GCFP(void);

  void init_ngc_index_buffer(void);
  void init_GCFP_buffer(const Read_Buffer_Set& fluid_v_pos_RBS);

private:
  const Device_Manager* _DM_ptr;

  Uniform_Grid_Common_CB_Data _common_CB_data;
  ComPtr<ID3D11Buffer>        _cptr_common_CB;
  Append_Conssume_Buffer_Set  _changed_GCFP_ID_ACBS; // �̹� �����ӿ� �ٲ� GCFPT ID�� ������ ACBS
  Neighborhood_Information    _info;

  // find_changed_GCFPT_ID_CS
  ComPtr<ID3D11ComputeShader> _cptr_find_changed_GCFP_ID_CS;
  ComPtr<ID3D11Buffer>        _cptr_find_changed_GCFP_ID_CS_CB;

  // update_GCFP
  ComPtr<ID3D11ComputeShader> _cptr_update_GCFP_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_GCFP_CS_CB;

  // rearrange GCFP
  ComPtr<ID3D11ComputeShader> _cptr_rearrange_GCFP_CS;

  // performance analysis
  static inline float _dt_sum_update                = 0.0f;
  static inline float _dt_sum_find_changed_GCFPT_ID = 0.0f;
  static inline float _dt_sum_update_GCFP           = 0.0f;
  static inline float _dt_sum_rearrange_GCFP        = 0.0f;
  static inline float _dt_sum_update_nfp            = 0.0f;
};

} // namespace ms