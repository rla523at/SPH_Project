#pragma once
#include "Neighborhood.h"
#include "SPH_Common_Data.h"

//abbreviation
//pid	: particle index
//gcid	: geometry cell index

namespace ms
{
class Neighborhood_Uniform_Grid : public Neighborhood
{
public:
  Neighborhood_Uniform_Grid(
    const Domain&               domain,
    const float                 divide_length,
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors);

public:
  void update(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors) override;

  const Neighbor_Informations& search_for_fluid(const size_t fpid) const override;
  const std::vector<size_t>& search_for_boundary(const size_t bpid) const override;

private:
  Index_Vector grid_cell_index_vector(const Vector3& v_pos) const;
  size_t       grid_cell_index(const Vector3& v_pos) const;
  size_t       grid_cell_index(const Index_Vector& index_vector) const;
  bool         is_valid_index(const Index_Vector& index_vector) const;
  bool         is_valid_index(const size_t gcid) const;

  //fill neighbor particle ids into pids and return number of neighbor particles
  size_t search(const Vector3& pos, size_t* pids) const;

  void update_fpid_to_neighbor_fpids(const std::vector<Vector3>& pos_vectors);
  void update_bpid_to_neighbor_fpids(
    const std::vector<Vector3>& fluid_particle_pos_vectors,
    const std::vector<Vector3>& boundary_particle_pos_vectors);

  void init_gcid_to_neighbor_gcids(void);

private:
  std::vector<std::vector<size_t>>   _gcid_to_neighbor_gcids;
  std::vector<std::vector<size_t>>   _gcid_to_fpids;
  std::vector<size_t>                _fpid_to_gcid;
  std::vector<Neighbor_Informations> _fpid_to_neighbor_informations;
  std::vector<size_t>                _bpid_to_gcid;
  std::vector<std::vector<size_t>>   _bpid_to_neighbor_fpids;

  Domain _domain;
  float  _divide_length = 0.0f;

  size_t _num_x_cell = 0;
  size_t _num_y_cell = 0;
  size_t _num_z_cell = 0;
};

} // namespace ms