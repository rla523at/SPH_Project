#pragma once
#include "Abbreviation.h"
#include "Neighborhood.h"
#include "SPH_Common_Data.h"

#include <d3d11.h>
#include <vector>

namespace ms
{
class Device_Manager;
}

/* abbreviation

pid	  : particle id
gc	  : geometry cell
fp    : fluid particle
ngc   : neighbor geometry cell
ninfo : neighbor informations
GCFPT : GCFP texture
GCFP  : Geometry Cell Fluid Particle(fluid particle in the geometry cell)

*/

inline constexpr size_t estimated_neighbor = 200;

// data structure
namespace ms
{
struct Neighbor_Informations_GPU
{
  UINT    indexes[estimated_neighbor];
  Vector3 translate_vectors[estimated_neighbor];
  float   distances[estimated_neighbor];
  UINT    num_neighbor = 0;
};

struct GCFPT_ID
{
  UINT gc_index   = 0;
  UINT gcfp_index = 0;
};

struct Changed_GCFPT_ID_Data
{
  GCFPT_ID prev_id;
  UINT     cur_gc_index;
};

struct Neighbor_Information
{
  size_t  index;
  Vector3 translate_vector;
  float   distance;
};

} // namespace ms

namespace ms
{
class Neighborhood_Uniform_Grid_GPU : public Neighborhood
{
public:
  Neighborhood_Uniform_Grid_GPU(
    const Domain&               domain,
    const float                 divide_length,
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors,
    const Device_Manager&       device_manager);

public:
  void update(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors) override;

  const Neighbor_Informations& search_for_fluid(const size_t fpid) const override;
  const std::vector<size_t>&   search_for_boundary(const size_t bpid) const override;

private:
  Grid_Cell_ID grid_cell_id(const Vector3& v_pos) const;
  size_t       grid_cell_index(const Grid_Cell_ID& index_vector) const;
  bool         is_valid_index(const Grid_Cell_ID& index_vector) const;

  void find_changed_GCFPT_ID(void);
  void update_GCFP_texture(void);

  std::vector<int> make_gcid_to_ngcids_initial_data(void);
  void             init_ngc_texture(const Device_Manager& device_manager);

  void init_GCFP_texture(const std::vector<Vector3>& fluid_particle_pos_vectors);
  void init_fpid_to_ninfo(const Device_Manager& device_manager);

private:
  // geometry cell마다 속해있는 fluid particle의 index들을 저장한 texture
  ComPtr<ID3D11Texture2D>           _cptr_GCFP_texture;
  ComPtr<ID3D11ShaderResourceView>  _cptr_GCFP_texture_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_GCFP_texture_UAV;

  // geometry cell마다 속해있는 fluid particle의 개수를 저장한 buffer
  ComPtr<ID3D11Buffer>              _cptr_GCFP_counter_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_GCFP_counter_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_GCFP_counter_buffer_UAV;

  // fluid particle마다 GCFPT ID를 저장한 Buffer
  ComPtr<ID3D11Buffer>              _cptr_GCFPT_ID_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_GCFPT_ID_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_GCFPT_ID_buffer_UAV;

  // 이번 프레임에 바뀐 GCFPT ID를 저장한 Buffer
  ComPtr<ID3D11Buffer>              _cptr_changed_GCFPT_ID_buffer;
  ComPtr<ID3D11UnorderedAccessView> _cptr_changed_GCFPT_ID_AC_UAV;

  ComPtr<ID3D11Buffer> _cptr_count_staging_buffer;

  ComPtr<ID3D11ComputeShader> _cptr_find_changed_GCFPT_ID_CS;

  ComPtr<ID3D11ComputeShader> _cptr_update_GCFPT_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_GCFPT_CS_constant_buffer;

  //////////////////////////////////////////////////////////////////////

  // fluid particle마다 neighbor informations를 저장한 buffer
  ComPtr<ID3D11Buffer>              _cptr_ninfo_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_ninfo_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_ninfo_buffer_UAV;

  // grid cell마다 neighbor grid cell의 index들을 저장한 texture
  ComPtr<ID3D11Texture2D>          _cptr_ngc_texture;
  ComPtr<ID3D11ShaderResourceView> _cptr_ngc_texture_SRV;

  ComPtr<ID3D11Buffer> _cptr_fp_index_to_ninfo_staging_buffer;


  //Constant Buffer로 묶어버리자
  Domain _domain;
  float  _divide_length = 0.0f;

  size_t _num_x_cell   = 0;
  size_t _num_y_cell   = 0;
  size_t _num_z_cell   = 0;
  size_t _num_particle = 0;

  const Device_Manager* _device_manager_ptr;

  // temporary
  ComPtr<ID3D11Buffer>             _cptr_fp_pos_buffer;
  ComPtr<ID3D11ShaderResourceView> _cptr_fp_pos_buffer_SRV;
  ComPtr<ID3D11Buffer>             _cptr_fp_pos_staging_buffer;
};

} // namespace ms