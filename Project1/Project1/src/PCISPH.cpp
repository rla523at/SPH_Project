#include "PCISPH.h"

#include "Debugger.h"
#include "Device_Manager.h"
#include "Kernel.h"
#include "Neighborhood_Uniform_Grid.h"
#include "Neighborhood_Uniform_Grid_GPU.h"

#include <algorithm>
#include <cmath>
#include <omp.h>

#undef max

namespace ms
{
PCISPH::PCISPH(
  const Initial_Condition_Cubes& initial_condition,
  const Domain&                  solution_domain,
  Device_Manager&          device_manager)
    : _domain(solution_domain)
{

  constexpr float smooth_length_param = 1.2f;

  _dt                  = 1.0e-2f;
  _rest_density        = 1000.0f;
  _viscosity           = 1.0e-2f;
  _allow_density_error = _rest_density * 4.0e-2f;
  _min_iter            = 1;
  _max_iter            = 3;

  const auto& ic = initial_condition;

  _particle_radius                  = ic.particle_spacing;
  _smoothing_length                 = smooth_length_param * ic.particle_spacing;
  _fluid_particles.position_vectors = ic.cal_initial_position();

  const auto num_fluid_particle = static_cast<UINT>(_fluid_particles.position_vectors.size());

  _fluid_particles.pressures.resize(num_fluid_particle);
  _fluid_particles.densities.resize(num_fluid_particle, _rest_density);
  _fluid_particles.velocity_vectors.resize(num_fluid_particle);
  _fluid_particles.acceleration_vectors.resize(num_fluid_particle);
  _pressure_acceleration_vectors.resize(num_fluid_particle);
  _current_position_vectors.resize(num_fluid_particle);
  _current_velocity_vectors.resize(num_fluid_particle);

  _uptr_kernel = std::make_unique<Cubic_Spline_Kernel>(_smoothing_length);

  const float divide_length = _uptr_kernel->supprot_radius();
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid>(solution_domain, divide_length, _fluid_particles.position_vectors, _boundary_position_vectors);

  this->init_mass_and_scailing_factor();

  //
  _max_density_errors.resize(omp_get_max_threads());

  //for output
  _DM_ptr = &device_manager;

  _fluid_v_pos_BS   = _DM_ptr->create_STRB_RWBS<Vector3>(num_fluid_particle);
  _fluid_density_BS = _DM_ptr->create_STRB_RWBS<float>(num_fluid_particle);
}

PCISPH::~PCISPH() = default;

void PCISPH::update(void)
{
  _uptr_neighborhood->update(_fluid_particles.position_vectors, _boundary_position_vectors);

  _scailing_factor = this->cal_scailing_factor();
  this->initialize_fluid_acceleration_vectors();
  this->initialize_pressure_and_pressure_acceleration();

  float  density_error = _rest_density;
  size_t num_iter      = 0;

  _current_position_vectors = _fluid_particles.position_vectors;
  _current_velocity_vectors = _fluid_particles.velocity_vectors;

  //std::cout << _time << "\n"; //debug

  while (_allow_density_error < density_error || num_iter < _min_iter)
  {
    this->predict_velocity_and_position();
    density_error = this->predict_density_and_update_pressure_and_cal_error();
    this->cal_pressure_acceleration();

    //std::cout << num_iter << " " << density_error << "\n"; //debug

    ++num_iter;

    if (_max_iter < num_iter)
    {
      break;
    }
  }

  //���������� update�� pressure�� ���� accleration�� �ݿ�
  this->predict_velocity_and_position();
  this->apply_boundary_condition();

  _time += _dt;
}

float PCISPH::particle_radius(void) const
{
  return _particle_radius;
}

size_t PCISPH::num_fluid_particle(void) const
{
  return _fluid_particles.num_particles();
}

const Read_Write_Buffer_Set& PCISPH::get_fluid_v_pos_RWBS(void) const
{
  _DM_ptr->write(_fluid_particles.position_vectors.data(), _fluid_v_pos_BS.cptr_buffer);
 
  return _fluid_v_pos_BS;
}

const Read_Write_Buffer_Set& PCISPH::get_fluid_density_RWBS(void) const
{
  _DM_ptr->write(_fluid_particles.densities.data(), _fluid_density_BS.cptr_buffer);
  
  return _fluid_density_BS;
}

void PCISPH::initialize_fluid_acceleration_vectors(void)
{
  constexpr Vector3 v_a_gravity = {0.0f, -9.8f, 0.0f};

  //viscosity constant
  const float m0                  = _mass_per_particle;
  const float h                   = _smoothing_length;
  const float viscosity_constant  = 10.0f * m0 * _viscosity;
  const float regularization_term = 0.01f * h * h;

  //surface tension constant
  const float sigma = 0.0e0f;

  //references
  const auto& fluid_densities            = _fluid_particles.densities;
  const auto& fluid_pressures            = _fluid_particles.pressures;
  const auto& fluid_position_vectors     = _fluid_particles.position_vectors;
  const auto& fluid_velocity_vectors     = _fluid_particles.velocity_vectors;
  auto&       fluid_acceleration_vectors = _fluid_particles.acceleration_vectors;

  const size_t num_fluid_particle = _fluid_particles.num_particles();

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    auto& v_a = fluid_acceleration_vectors[i];

    Vector3 v_a_viscosity(0.0f);
    Vector3 v_a_surface_tension(0.0f);

    const float    rhoi = fluid_densities[i];
    const Vector3& v_vi = fluid_velocity_vectors[i];

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);

    const auto& neighbor_indexes           = neighbor_informations.indexes;
    const auto& neighbor_translate_vectors = neighbor_informations.translate_vectors;
    const auto& neighbor_distances         = neighbor_informations.distances;

    const auto num_neighbor = neighbor_indexes.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      if (i == neighbor_index)
        continue;

      const float    rhoj = fluid_densities[neighbor_index];
      const Vector3& v_vj = fluid_velocity_vectors[neighbor_index];

      const auto v_xij     = neighbor_translate_vectors[j];
      const auto distance  = neighbor_distances[j];
      const auto distance2 = distance * distance;

      // distance�� 0�̸� v_xij_normal�� grad_q ���� 0���� ����� ������ ��
      if (distance == 0.0f)
        continue;

      const Vector3 v_xij_normal = v_xij / distance;
      v_a_surface_tension -= sigma * m0 * _uptr_kernel->W(distance) * v_xij_normal;

      // cal grad_kernel
      const auto v_grad_q      = 1.0f / (h * distance) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distance) * v_grad_q;

      // cal v_laplacian_velocity
      const auto v_vij  = v_vi - v_vj;
      const auto coeff2 = v_vij.Dot(v_xij) / (rhoj * (distance2 + regularization_term));

      const auto v_laplacian_velocity = coeff2 * v_grad_kernel;

      // update acceleration
      v_a_viscosity += v_laplacian_velocity;
    }

    v_a = viscosity_constant * v_a_viscosity + v_a_gravity + v_a_surface_tension;
  }
}

void PCISPH::initialize_pressure_and_pressure_acceleration(void)
{
  auto& pressures     = _fluid_particles.pressures;
  auto& v_a_pressures = _pressure_acceleration_vectors;

  std::fill(pressures.begin(), pressures.end(), 0.0f);
  std::fill(v_a_pressures.begin(), v_a_pressures.end(), Vector3(0.0f, 0.0f, 0.0f));
}

void PCISPH::predict_velocity_and_position(void)
{
  const auto num_fluid_particles = _fluid_particles.num_particles();

  auto&       position_vectors     = _fluid_particles.position_vectors;
  auto&       velocity_vectors     = _fluid_particles.velocity_vectors;
  const auto& acceleration_vectors = _fluid_particles.acceleration_vectors;

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particles; i++)
  {
    const auto& v_p_cur = _current_position_vectors[i];
    const auto& v_v_cur = _current_velocity_vectors[i];

    auto&      v_p = position_vectors[i];
    auto&      v_v = velocity_vectors[i];
    const auto v_a = acceleration_vectors[i] + _pressure_acceleration_vectors[i];

    v_v = v_v_cur + v_a * _dt;
    v_p = v_p_cur + v_v * _dt;
  }
}

float PCISPH::predict_density_and_update_pressure_and_cal_error(void)
{
  const float h    = _smoothing_length;
  const float rho0 = _rest_density;
  const float m0   = _mass_per_particle;

  const auto  num_fluid_particle = _fluid_particles.num_particles();
  auto&       densities          = _fluid_particles.densities;
  auto&       pressures          = _fluid_particles.pressures;
  const auto& position_vectors   = _fluid_particles.position_vectors;

  std::fill(_max_density_errors.begin(), _max_density_errors.end(), -1.0f);

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    const auto thread_id = omp_get_thread_num();
    auto&      max_error = _max_density_errors[thread_id];

    auto&       rho  = densities[i];
    auto&       p    = pressures[i];
    const auto& v_xi = position_vectors[i];

    rho = 0.0;

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes      = neighbor_informations.indexes;

    const auto num_neighbor = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto& v_xj     = position_vectors[neighbor_indexes[j]];
      const auto  distance = (v_xi - v_xj).Length();

      rho += _uptr_kernel->W(distance);
    }

    rho *= m0;

    const float density_error = rho - rho0;

    p += _scailing_factor * density_error;
    p = std::max(p, 0.0f);

    max_error = (std::max)(max_error, density_error);
  }

  return *std::max_element(_max_density_errors.begin(), _max_density_errors.end());
}

void PCISPH::cal_pressure_acceleration(void)
{
  //viscosity constant
  const float m0 = _mass_per_particle;
  const float h  = _smoothing_length;

  //references
  const auto& densities        = _fluid_particles.densities;
  const auto& pressures        = _fluid_particles.pressures;
  const auto& position_vectors = _fluid_particles.position_vectors;

  const size_t num_fluid_particle = _fluid_particles.num_particles();

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    auto& v_a_pressure = _pressure_acceleration_vectors[i];
    v_a_pressure       = Vector3(0.0f, 0.0f, 0.0f);

    const float rhoi = densities[i];
    const float pi   = pressures[i];
    const auto& v_xi = position_vectors[i];

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes      = neighbor_informations.indexes;

    const auto num_neighbor = neighbor_indexes.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      if (i == neighbor_index)
        continue;

      const float rhoj = densities[neighbor_index];
      const float pj   = pressures[neighbor_index];
      const auto& v_xj = position_vectors[neighbor_index];

      const auto v_xij    = v_xi - v_xj;
      const auto distnace = v_xij.Length();

      // distance�� 0�̸� grad_q ���� 0���� ����� ������ ��
      if (distnace == 0.0f)
        continue;

      // cal grad_kernel
      const auto v_grad_q      = 1.0f / (h * distnace) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distnace) * v_grad_q;

      // cal v_grad_pressure
      const auto coeff           = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
      const auto v_grad_pressure = coeff * v_grad_kernel;

      // update acceleration
      v_a_pressure += v_grad_pressure;
    }

    v_a_pressure *= -m0;
  }
}

void PCISPH::apply_boundary_condition(void)
{
  constexpr float cor  = 0.5f; // Coefficient Of Restitution
  constexpr float cor2 = 0.5f; // Coefficient Of Restitution

  const auto num_fluid_particles = _fluid_particles.num_particles();

  const float radius       = _smoothing_length;
  const float wall_x_start = _domain.x_start + radius;
  const float wall_x_end   = _domain.x_end - radius;
  const float wall_y_start = _domain.y_start + radius;
  const float wall_y_end   = _domain.y_end - radius;
  const float wall_z_start = _domain.z_start + radius;
  const float wall_z_end   = _domain.z_end - radius;

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particles; ++i)
  {
    auto& v_p = _fluid_particles.position_vectors[i];
    auto& v_v = _fluid_particles.velocity_vectors[i];

    if (v_p.x < wall_x_start && v_v.x < 0.0f)
    {
      v_v.x *= -cor2;
      v_p.x = wall_x_start;
    }

    if (v_p.x > wall_x_end && v_v.x > 0.0f)
    {
      v_v.x *= -cor2;
      v_p.x = wall_x_end;
    }

    if (v_p.y < wall_y_start && v_v.y < 0.0f)
    {
      v_v.y *= -cor;
      v_p.y = wall_y_start;
    }

    if (v_p.y > wall_y_end && v_v.y > 0.0f)
    {
      v_v.y *= -cor;
      v_p.y = wall_y_end;
    }

    if (v_p.z < wall_z_start && v_v.z < 0.0f)
    {
      v_v.z *= -cor2;
      v_p.z = wall_z_start;
    }

    if (v_p.z > wall_z_end && v_v.z > 0.0f)
    {
      v_v.z *= -cor2;
      v_p.z = wall_z_end;
    }
  }
}

void PCISPH::init_mass_and_scailing_factor(void)
{
  UINT max_index = 0;

  // init mass
  {
    const auto num_fluid_particle = _fluid_particles.num_particles();
    float      max_number_density = 0.0f;

    for (UINT i = 0; i < num_fluid_particle; i++)
    {
      float number_density = 0.0;

      const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
      const auto& neighbor_distances    = neighbor_informations.distances;
      const auto  num_neighbor          = neighbor_distances.size();

      for (UINT j = 0; j < num_neighbor; j++)
      {
        const float dist = neighbor_distances[j];
        number_density += _uptr_kernel->W(dist);
      }

      if (max_number_density < number_density)
      {
        max_number_density = number_density;
        max_index          = i;
      }
    }

    _mass_per_particle = _rest_density / max_number_density;
  }

  // init scailing factor
  {
    const float rho0 = _rest_density;
    const float m0   = _mass_per_particle;
    const float beta = _dt * _dt * m0 * m0 * 2 / (rho0 * rho0);
    const float h    = _smoothing_length;

    const auto& neighbor_informations      = _uptr_neighborhood->search_for_fluid(max_index);
    const auto& neighbor_translate_vectors = neighbor_informations.translate_vectors;
    const auto& neighbor_distances         = neighbor_informations.distances;
    const auto  num_neighbor               = neighbor_translate_vectors.size();

    Vector3 v_sum_grad_kernel = {0.0f, 0.0f, 0.0f};
    float   size_sum          = 0.0f;
    for (size_t i = 0; i < num_neighbor; ++i)
    {
      const float distance = neighbor_distances[i];

      //ignore my self
      if (distance == 0)
        continue;

      const auto& v_xij = neighbor_translate_vectors[i];

      const auto v_grad_q      = 1.0f / (h * distance) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distance) * v_grad_q;

      v_sum_grad_kernel += v_grad_kernel;
      size_sum += v_grad_kernel.Dot(v_grad_kernel);
    }

    const float sum_dot_sum = v_sum_grad_kernel.Dot(v_sum_grad_kernel);

    _scailing_factor = 1.0f / (beta * (sum_dot_sum + size_sum));
  }
}

float PCISPH::cal_scailing_factor(void) const
{
  const auto num_particle = _fluid_particles.num_particles();

  //find particle which has maximum number density
  UINT max_index          = 0;
  float  max_number_density = 0.0f;

  for (UINT i = 0; i < num_particle; i++)
  {
    const float number_density = this->cal_number_density(i);

    if (max_number_density < number_density)
    {
      max_number_density = number_density;
      max_index          = i;
    }
  }

  const float rho0 = _rest_density;
  const float m0   = _mass_per_particle;
  const float beta = _dt * _dt * m0 * m0 * 2 / (rho0 * rho0);
  const float h    = _smoothing_length;

  const auto& neighbor_informations      = _uptr_neighborhood->search_for_fluid(max_index);
  const auto& neighbor_translate_vectors = neighbor_informations.translate_vectors;
  const auto& neighbor_distances         = neighbor_informations.distances;
  const auto  num_neighbor               = neighbor_translate_vectors.size();

  Vector3 v_sum_grad_kernel = {0.0f, 0.0f, 0.0f};
  float   size_sum          = 0.0f;
  for (size_t i = 0; i < num_neighbor; ++i)
  {
    const float distance = neighbor_distances[i];

    //ignore my self
    if (distance == 0)
      continue;

    const auto& v_xij = neighbor_translate_vectors[i];

    const auto v_grad_q      = 1.0f / (h * distance) * v_xij;
    const auto v_grad_kernel = _uptr_kernel->dWdq(distance) * v_grad_q;

    v_sum_grad_kernel += v_grad_kernel;
    size_sum += v_grad_kernel.Dot(v_grad_kernel);
  }

  const float sum_dot_sum = v_sum_grad_kernel.Dot(v_sum_grad_kernel);

  return 1.0f / (beta * (sum_dot_sum + size_sum));
}

float PCISPH::cal_number_density(const UINT fluid_particle_id) const
{
  float number_density = 0.0;

  const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(fluid_particle_id);
  const auto& neighbor_distances    = neighbor_informations.distances;
  const auto  num_neighbor          = neighbor_distances.size();

  for (UINT j = 0; j < num_neighbor; j++)
  {
    const float dist = neighbor_distances[j];
    number_density += _uptr_kernel->W(dist);
  }

  return number_density;
}

} // namespace ms