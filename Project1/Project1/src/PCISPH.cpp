#include "PCISPH.h"

#include "Kernel.h"
#include "Neighborhood_Uniform_Grid.h"

#include <algorithm>
#include <cmath>

namespace ms
{
PCISPH::PCISPH(const Initial_Condition_Cubes& initial_condition, const Domain& solution_domain)
{
  _rest_density = 1000.0f;

  const auto& ic = initial_condition;

  _particle_radius                  = ic.particle_spacing;
  _smoothing_length                 = 1.6f * ic.particle_spacing;
  _fluid_particles.position_vectors = ic.cal_initial_position();

  const auto num_fluid_particle = _fluid_particles.position_vectors.size();

  _fluid_particles.pressures.resize(num_fluid_particle);
  _fluid_particles.densities.resize(num_fluid_particle, _rest_density);
  _fluid_particles.velocity_vectors.resize(num_fluid_particle);
  _fluid_particles.acceleration_vectors.resize(num_fluid_particle);
  _pressure_acceleration_vectors.resize(num_fluid_particle);
  _current_position_vectors.resize(num_fluid_particle);
  _current_velocity_vectors.resize(num_fluid_particle);

  _uptr_kernel = std::make_unique<Cubic_Spline_Kernel>(_smoothing_length);

  const float divide_length = _uptr_kernel->supprot_length();
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid>(solution_domain, divide_length, _fluid_particles.position_vectors, _boundary_position_vectors);

  this->init_mass_and_scailing_factor();
}

void PCISPH::update(void)
{
  constexpr size_t num_minimum_iter     = 3;
  constexpr float  max_density_variance = 10;

  _uptr_neighborhood->update(_fluid_particles.position_vectors, _boundary_position_vectors);
  this->initialize_fluid_acceleration_vectors();
  this->initialize_pressure_and_pressure_acceleration();

  float  density_variation = _rest_density;
  size_t num_iter          = 0;

  _current_position_vectors = _fluid_particles.position_vectors;
  _current_velocity_vectors = _fluid_particles.velocity_vectors;

  while (max_density_variance < density_variation || num_iter < num_minimum_iter)
  {
    this->predict_velocity_and_position();
    density_variation = this->predict_density_and_variation_and_update_pressure();
    this->cal_pressure_acceleration();
  }

  //마지막으로 update된 pressure에 의한 accleration을 반영
  this->predict_velocity_and_position();
}

void PCISPH::initialize_fluid_acceleration_vectors(void)
{
  constexpr Vector3 v_a_gravity = {0.0f, -9.8f, 0.0f};

  //viscosity constant
  const float m0                  = _mass_per_particle;
  const float h                   = _smoothing_length;
  const float viscousity_constant = 10.0f * m0 * _viscosity;
  const float regularization_term = 0.01f * h * h;

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
      const auto distnace  = neighbor_distances[j];
      const auto distance2 = distnace * distnace;

      // distance가 0이면 v_xij_normal과 grad_q 계산시 0으로 나누어서 문제가 됨
      if (distnace == 0.0f)
        continue;

      // cal grad_kernel
      const auto v_grad_q      = 1.0f / (h * distnace) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distnace) * v_grad_q;

      // cal v_laplacian_velocity
      const auto v_vij  = v_vi - v_vj;
      const auto coeff2 = v_vij.Dot(v_xij) / rhoj * (distance2 + regularization_term);

      const auto v_laplacian_velocity = coeff2 * v_grad_kernel;

      // update acceleration
      v_a_viscosity += v_laplacian_velocity;
    }

    v_a = viscousity_constant * v_a_viscosity + v_a_gravity;
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

  auto&       fluid_position_vectors     = _fluid_particles.position_vectors;
  auto&       fluid_velocity_vectors     = _fluid_particles.velocity_vectors;
  const auto& fluid_acceleration_vectors = _fluid_particles.acceleration_vectors;

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particles; i++)
  {
    const auto& v_p_cur = _current_position_vectors[i];
    const auto& v_v_cur = _current_velocity_vectors[i];

    auto&      v_p = fluid_position_vectors[i];
    auto&      v_v = fluid_velocity_vectors[i];
    const auto v_a = fluid_acceleration_vectors[i] + _pressure_acceleration_vectors[i];

    v_v = v_v_cur + v_a * _dt;
    v_p = v_p_cur + v_v * _dt;
  }

  this->apply_boundary_condition();
}

float PCISPH::predict_density_and_variation_and_update_pressure(void)
{
  const float h    = _smoothing_length;
  const float rho0 = _rest_density;
  const float m0   = _mass_per_particle;

  const auto num_fluid_particle = _fluid_particles.num_particles();
  auto&      densities          = _fluid_particles.densities;
  auto&      pressures          = _fluid_particles.pressures;

  float max_variation = rho0;

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    auto&       rho  = densities[i];
    auto&       p    = pressures[i];
    const auto& v_xi = _current_position_vectors[i];

    rho = 0.0;

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes      = neighbor_informations.indexes;

    const auto num_neighbor = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto& v_xj     = _current_position_vectors[neighbor_indexes[j]];
      const auto  distance = (v_xi - v_xj).Length();

      rho += _uptr_kernel->W(distance);
    }

    rho *= m0;

    //check boundary particle
    const bool is_boundary_particle = rho < 0.9f * rho0;

    if (is_boundary_particle)
      rho = rho0;

    const float variation = std::abs(rho - rho0);
    max_variation         = (std::max)(max_variation, variation);

    p += _scailing_factor * variation;
  }

  return max_variation;
}

void PCISPH::cal_pressure_acceleration(void)
{
  //viscosity constant
  const float m0 = _mass_per_particle;
  const float h  = _smoothing_length;

  //references
  const auto& fluid_densities = _fluid_particles.densities;
  const auto& fluid_pressures = _fluid_particles.pressures;

  const size_t num_fluid_particle = _fluid_particles.num_particles();

#pragma omp parallel for
  for (int i = 0; i < num_fluid_particle; i++)
  {
    auto& v_a_pressure = _pressure_acceleration_vectors[i];
    v_a_pressure       = Vector3(0.0f, 0.0f, 0.0f);

    const float rhoi = fluid_densities[i];
    const float pi   = fluid_pressures[i];
    const auto& v_xi = _current_position_vectors[i];

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes      = neighbor_informations.indexes;

    const auto num_neighbor = neighbor_indexes.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      if (i == neighbor_index)
        continue;

      const float rhoj = fluid_densities[neighbor_index];
      const float pj   = fluid_pressures[neighbor_index];
      const auto& v_xj = _current_position_vectors[neighbor_index];

      const auto v_xij    = v_xi - v_xj;
      const auto distnace = v_xij.Length();

      // distance가 0이면 grad_q 계산시 0으로 나누어서 문제가 됨
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
  size_t max_index = 0;

  // init mass
  {
    const auto num_fluid_particle = _fluid_particles.num_particles();
    float      max_number_density = 0.0f;

    for (size_t i = 0; i < num_fluid_particle; i++)
    {
      float number_density = 0.0;

      const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
      const auto& neighbor_distances    = neighbor_informations.distances;
      const auto  num_neighbor          = neighbor_distances.size();

      for (size_t j = 0; j < num_neighbor; j++)
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
      const auto& v_xij    = neighbor_translate_vectors[i];

      const auto v_grad_q      = 1.0f / (h * distance) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distance) * v_grad_q;

      v_sum_grad_kernel += v_grad_kernel;
      size_sum += v_grad_kernel.Dot(v_grad_kernel);
    }

    const float sum_dot_sum = v_sum_grad_kernel.Dot(v_sum_grad_kernel);

    _scailing_factor = 1.0f / beta * (sum_dot_sum + size_sum);
  }
}

} // namespace ms