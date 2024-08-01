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

// data structure
namespace ms
{
struct Uniform_Grid_Constants
{
  UINT  num_x_cell;
  UINT  num_y_cell;
  UINT  num_z_cell;
  UINT  num_cell;
  UINT  num_particle;
  float domain_x_start;
  float domain_y_start;
  float domain_z_start;
  float divide_length;
  float padding[3];
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
  UINT         grid_cell_index(const Grid_Cell_ID& index_vector) const;
  bool         is_valid_index(const Grid_Cell_ID& index_vector) const;

  void find_changed_GCFPT_ID(void);
  void update_GCFP_texture(void);
  void update_nfp(void);

  void init_ngc_texture(void);

  void init_GCFP_texture(const std::vector<Vector3>& fluid_particle_pos_vectors);
  void init_nfp(void);

  //temporary
  void copy_to_ninfos(void);
  //temporary

private:
  const Device_Manager* _device_manager_ptr;

  //////////////////////////////////////////////////////////////////////

  Uniform_Grid_Constants _constant_data;
  ComPtr<ID3D11Buffer>   _common_constnat_buffer;

  //////////////////////////////////////////////////////////////////////

  // grid cell마다 neighbor grid cell의 index들을 저장한 texture
  ComPtr<ID3D11Texture2D>          _cptr_ngc_texture;
  ComPtr<ID3D11ShaderResourceView> _cptr_ngc_texture_SRV;

  // grid cell마다 neighbor grid cell의 개수를 저장한 buffer
  ComPtr<ID3D11Buffer>             _cptr_ngc_count_buffer;
  ComPtr<ID3D11ShaderResourceView> _cptr_ngc_count_buffer_SRV;

  //////////////////////////////////////////////////////////////////////

  // 이번 프레임에 바뀐 GCFPT ID를 저장한 Buffer
  ComPtr<ID3D11Buffer>              _cptr_changed_GCFPT_ID_buffer;
  ComPtr<ID3D11UnorderedAccessView> _cptr_changed_GCFPT_ID_AC_UAV;

  //find_changed_GCFPT_ID_CS
  ComPtr<ID3D11ComputeShader> _cptr_find_changed_GCFPT_ID_CS;
  ComPtr<ID3D11Buffer>        _cptr_find_changed_GCFPT_ID_CS_constant_buffer;

  // geometry cell마다 속해있는 fluid particle의 index들을 저장한 texture
  ComPtr<ID3D11Texture2D>           _cptr_GCFP_texture;
  ComPtr<ID3D11ShaderResourceView>  _cptr_GCFP_texture_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_GCFP_texture_UAV;

  // geometry cell마다 속해있는 fluid particle의 개수를 저장한 buffer
  ComPtr<ID3D11Buffer>              _cptr_GCFP_count_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_GCFP_count_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_GCFP_count_buffer_UAV;

  // fluid particle마다 GCFPT ID를 저장한 Buffer
  ComPtr<ID3D11Buffer>              _cptr_GCFPT_ID_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_GCFPT_ID_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_GCFPT_ID_buffer_UAV;

  //update_GCFPT_CS
  ComPtr<ID3D11ComputeShader> _cptr_update_GCFPT_CS;
  ComPtr<ID3D11Buffer>        _cptr_update_GCFPT_CS_constant_buffer;

  //////////////////////////////////////////////////////////////////////

  // fluid particle마다 neighbor fluid particle의 index를 저장한 texture
  ComPtr<ID3D11Texture2D>           _cptr_nfp_texture;
  ComPtr<ID3D11ShaderResourceView>  _cptr_nfp_texture_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_nfp_texture_UAV;

  // fluid particle마다 neighbor fluid particle의 translate vector를 저장한 texture
  ComPtr<ID3D11Texture2D>           _cptr_nfp_tvec_texture;
  ComPtr<ID3D11ShaderResourceView>  _cptr_nfp_tvec_texture_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_nfp_tvec_texture_UAV;

  // fluid particle마다 neighbor fluid particle의 distance를 저장한 texture
  ComPtr<ID3D11Texture2D>           _cptr_nfp_dist_texture;
  ComPtr<ID3D11ShaderResourceView>  _cptr_nfp_dist_texture_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_nfp_dist_texture_UAV;

  // fluid particle 마다 neighbor fluid particle의 개수를 저장한 buffer
  ComPtr<ID3D11Buffer>              _cptr_nfp_count_buffer;
  ComPtr<ID3D11ShaderResourceView>  _cptr_nfp_count_buffer_SRV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_nfp_count_buffer_UAV;

  // compute shader
  ComPtr<ID3D11ComputeShader> _cptr_update_nfp_CS;

  //////////////////////////////////////////////////////////////////////

  // temporary
  ComPtr<ID3D11Buffer>             _cptr_fp_pos_buffer;
  ComPtr<ID3D11ShaderResourceView> _cptr_fp_pos_buffer_SRV;
  ComPtr<ID3D11Buffer>             _cptr_fp_pos_staging_buffer;

  ComPtr<ID3D11Texture2D> _cptr_nfp_staging_texture;
  ComPtr<ID3D11Texture2D> _cptr_nfp_tvec_staging_texture;
  ComPtr<ID3D11Texture2D> _cptr_nfp_dist_staging_texture;
  ComPtr<ID3D11Buffer>    _cptr_nfp_count_staging_buffer;

  std::vector<Neighbor_Informations> _ninfos;
  std::vector<std::vector<size_t>>   _bpid_to_neighbor_fpids;
  // temporary
};

} // namespace ms