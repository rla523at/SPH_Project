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

//abbreviation
//pid	  : particle id
//gcid	: geometry cell id
//ngcid : neighbor geometry cell id

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
    const Device_Manager& device_manager);

public:
  void update(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors) override;

  const Neighbor_Informations& search_for_fluid(const size_t fpid) const override;
  const std::vector<size_t>&   search_for_boundary(const size_t bpid) const override;
  //
  //private:
  //  Index_Vector grid_cell_index_vector(const Vector3& v_pos) const;
  //  size_t       grid_cell_index(const Vector3& v_pos) const;
  size_t grid_cell_index(const Index_Vector& index_vector) const;
  bool   is_valid_index(const Index_Vector& index_vector) const;
  //  bool         is_valid_index(const size_t gcid) const;
  //
  //  //fill neighbor particle ids into pids and return number of neighbor particles
  //  size_t search(const Vector3& pos, size_t* pids) const;
  //
  //  void update_fpid_to_neighbor_fpids(const std::vector<Vector3>& pos_vectors);
  //  void update_bpid_to_neighbor_fpids(
  //    const std::vector<Vector3>& fluid_particle_pos_vectors,
  //    const std::vector<Vector3>& boundary_particle_pos_vectors);
  //
  std::vector<int> make_gcid_to_ngcids_initial_data(void);
  void             init_gcid_to_ngcids(const Device_Manager& device_manager);

private:
  ComPtr<ID3D11Texture2D>          _cptr_gcid_to_ngcids_texture;
  ComPtr<ID3D11ShaderResourceView> _cptr_gcid_to_ngcids_SRV;

  std::vector<std::vector<size_t>>   _gcid_to_fpids;
  std::vector<size_t>                _fpid_to_gcid;
  std::vector<size_t>                _bpid_to_gcid;
  std::vector<Neighbor_Informations> _fpid_to_neighbor_informations;
  std::vector<std::vector<size_t>>   _bpid_to_neighbor_fpids;

  Domain _domain;
  float  _divide_length = 0.0f;

  size_t _num_x_cell = 0;
  size_t _num_y_cell = 0;
  size_t _num_z_cell = 0;
};

} // namespace ms