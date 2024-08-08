#pragma once
#include "Abbreviation.h"
#include "Buffer_Set.h"
#include "Neighborhood.h"
#include "SPH_Common_Data.h"

#include <d3d11.h>
#include <vector>

namespace ms
{
class Device_Manager;
}

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

struct GCFP_ID
{
  UINT gc_index   = 0;
  UINT gcfp_index = 0;
};

struct Changed_GCFPT_ID_Data
{
  GCFP_ID prev_id;
  UINT    cur_gc_index = 0;
};

struct Neighbor_Information
{
  UINT    fp_index = 0;
  Vector3 translate_vector;
  float   distance = 0.0f;
};

} // namespace ms

namespace ms
{
class Neighborhood_Uniform_Grid_GPU
{
public:
  Neighborhood_Uniform_Grid_GPU(
    const Domain&          domain,
    const float            divide_length,
    const Read_Buffer_Set& fluid_v_pos_RBS,
    const UINT             num_fp,
    const Device_Manager&  device_manager);

public:
  void update(const Read_Buffer_Set& fluid_v_pos_RBS);

  const Read_Write_Buffer_Set& get_ninfo_BS(void) const;
  const Read_Write_Buffer_Set& get_ncount_BS(void) const;

  ComPtr<ID3D11ShaderResourceView> nfp_info_buffer_SRV_cptr(void) const;
  ComPtr<ID3D11ShaderResourceView> nfp_count_buffer_SRV_cptr(void) const;

private:
  Grid_Cell_ID grid_cell_id(const Vector3& v_pos) const;
  UINT         grid_cell_index(const Grid_Cell_ID& index_vector) const;
  bool         is_valid_index(const Grid_Cell_ID& index_vector) const;

  void find_changed_GCFPT_ID(const Read_Buffer_Set& fluid_v_pos_RBS);
  void update_GCFP_buffer(void);
  void rearrange_GCFP(void);
  void update_nfp(const Read_Buffer_Set& fluid_v_pos_RBS);

  void init_ngc_index_buffer(void);
  void init_GCFP_buffer(const Read_Buffer_Set& fluid_v_pos_RBS);

private:
  const Device_Manager* _DM_ptr;

  Uniform_Grid_Common_CB_Data _common_CB_data;
  ComPtr<ID3D11Buffer>        _cptr_common_CB;

  //////////////////////////////////////////////////////////////////////

  // geometry cell * estimated ngc만큼 ngc index을 저장한 ISTRB의 RBS
  Read_Buffer_Set _ngc_index_RBS;

  // geometry cell마다 neighbor grid cell의 개수를 저장한 ISTRB의 RBS
  Read_Buffer_Set _ngc_count_RBS;

  // geometry cell * estimated gcfp만큼 fp index를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _fp_index_RWBS;

  // geometry cell마다 속해있는 fluid particle의 개수를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _GCFP_count_RWBS;

  // fluid particle마다 GCFP ID를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _GCFP_ID_RWBS;

  // fluid particle * estimated neighbor만큼 Neighbor_Information을 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _ninfo_RWBS;

  // fluid particle 마다 neighbor fluid particle의 개수를 저장한 STRB의 RWBS
  Read_Write_Buffer_Set _ncount_RWBS;

  // 이번 프레임에 바뀐 GCFPT ID를 저장한 Buffer
  ComPtr<ID3D11Buffer>              _cptr_changed_GCFP_ID_buffer;
  ComPtr<ID3D11UnorderedAccessView> _cptr_changed_GCFPT_ID_AC_UAV;

  //////////////////////////////////////////////////////////////////////

  // find_changed_GCFPT_ID_CS
  ComPtr<ID3D11ComputeShader> _cptr_find_changed_GCFP_ID_CS;
  ComPtr<ID3D11Buffer>        _cptr_find_changed_GCFP_ID_CS_CB;

  // update_GCFP
  ComPtr<ID3D11ComputeShader> _cptr_update_GCFP_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_GCFP_CS_CB;

  //rearrange GCFP
  ComPtr<ID3D11ComputeShader> _cptr_rearrange_GCFP_CS;

  // update nfp
  ComPtr<ID3D11ComputeShader> _cptr_update_nfp_CS;

  // performance analysis
  float _dt_update; //duration time
};

} // namespace ms