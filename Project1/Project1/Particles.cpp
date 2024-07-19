#include "Particles.h"

#include "Neighborhood_Brute_Force.h"
#include "Neighborhood_Uniform_Grid.h"

#include "../_lib/_header/msexception/Exception.h"
#include <algorithm>
#include <numbers>
#include <omp.h>

namespace ms
{
Fluid_Particles::Fluid_Particles(const Material_Property& material_property, const Initial_Condition_Dam& initial_condition, const Domain& solution_domain)
    : _material_proeprty(material_property), _domain(solution_domain)
{
  const auto& ic = initial_condition;

  const size_t num_particle_in_x = static_cast<size_t>(ic.dam.dx() / ic.division_length) + 1;
  const size_t num_particle_in_y = static_cast<size_t>(ic.dam.dy() / ic.division_length) + 1;
  const size_t num_particle_in_z = static_cast<size_t>(ic.dam.dz() / ic.division_length) + 1;

  _num_particle   = num_particle_in_x * num_particle_in_y * num_particle_in_z;
  _support_length = ic.division_length * 1.2f;

  _pressures.resize(_num_particle);
  _densities.resize(_num_particle, _material_proeprty.rest_density);
  _position_vectors.resize(_num_particle);
  _velocity_vectors.resize(_num_particle);
  _accelaration_vectors.resize(_num_particle);

  //init position
  const Vector3 v_pos_init = {ic.dam.x_start, ic.dam.y_start, ic.dam.z_start};
  Vector3       v_pos      = v_pos_init;

  size_t index = 0;
  for (size_t i = 0; i < num_particle_in_y; ++i)
  {
    for (size_t j = 0; j < num_particle_in_z; ++j)
    {
      for (size_t k = 0; k < num_particle_in_x; ++k)
      {
        _position_vectors[index] = v_pos;
        ++index;
        v_pos.x += ic.division_length;
      }

      v_pos.x = v_pos_init.x;
      v_pos.z += ic.division_length;
    }

    v_pos.x = v_pos_init.x;
    v_pos.z = v_pos_init.z;
    v_pos.y += ic.division_length;
  }

  //init neighborhood
  const float divide_length = _support_length;
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid>(solution_domain, divide_length, _position_vectors);

  // cal mass
  _mass_per_particle = this->cal_mass_per_particle_number_density_mean();
  //_mass_per_particle = this->cal_mass_per_particle_number_density_max();
  //_mass_per_particle = this->cal_mass_per_particle_number_density_min();


  //const float total_volume = ic.dam.dx() * ic.dam.dy() * ic.dam.dz();
  //_mass_per_particle = this->cal_mass_per_particle_1994_monaghan(total_volume);
}

Fluid_Particles::Fluid_Particles(
  const Material_Property&            property,
  const Initial_Condition_Double_Dam& initial_condition,
  const Domain&                       solution_domain)
    : _material_proeprty(property), _domain(solution_domain)
{
  constexpr float  pi              = std::numbers::pi_v<float>;
  constexpr size_t desire_neighbor = 50;

  const auto& ic = initial_condition;

  const size_t num_particle_in_dam1_x = static_cast<size_t>(ic.dam1.dx() / ic.division_length) + 1;
  const size_t num_particle_in_dam1_y = static_cast<size_t>(ic.dam1.dy() / ic.division_length) + 1;
  const size_t num_particle_in_dam1_z = static_cast<size_t>(ic.dam1.dz() / ic.division_length) + 1;
  const size_t num_particle_in_dam1   = num_particle_in_dam1_x * num_particle_in_dam1_y * num_particle_in_dam1_z;

  const size_t num_particle_in_dam2_x = static_cast<size_t>(ic.dam2.dx() / ic.division_length) + 1;
  const size_t num_particle_in_dam2_y = static_cast<size_t>(ic.dam2.dy() / ic.division_length) + 1;
  const size_t num_particle_in_dam2_z = static_cast<size_t>(ic.dam2.dz() / ic.division_length) + 1;
  const size_t num_particle_in_dam2   = num_particle_in_dam2_x * num_particle_in_dam2_y * num_particle_in_dam2_z;

  _num_particle   = num_particle_in_dam1 + num_particle_in_dam2;
  _support_length = ic.division_length * 1.2f;

  _pressures.resize(_num_particle);
  _densities.resize(_num_particle, _material_proeprty.rest_density);
  _position_vectors.resize(_num_particle);
  _velocity_vectors.resize(_num_particle);
  _accelaration_vectors.resize(_num_particle);

  _leap_frog = _velocity_vectors;

  //init position
  const Vector3 v_pos_init = {ic.dam1.x_start, ic.dam1.y_start, ic.dam1.z_start};
  Vector3       v_pos      = v_pos_init;

  size_t index = 0;
  for (size_t i = 0; i < num_particle_in_dam1_y; ++i)
  {
    for (size_t j = 0; j < num_particle_in_dam1_z; ++j)
    {
      for (size_t k = 0; k < num_particle_in_dam1_x; ++k)
      {
        _position_vectors[index] = v_pos;
        ++index;
        v_pos.x += ic.division_length;
      }

      v_pos.x = v_pos_init.x;
      v_pos.z += ic.division_length;
    }

    v_pos.x = v_pos_init.x;
    v_pos.z = v_pos_init.z;
    v_pos.y += ic.division_length;
  }

  const Vector3 v_pos2_init = {ic.dam2.x_start, ic.dam2.y_start, ic.dam2.z_start};
  Vector3       v_pos2      = v_pos2_init;

  for (size_t i = 0; i < num_particle_in_dam2_y; ++i)
  {
    for (size_t j = 0; j < num_particle_in_dam2_z; ++j)
    {
      for (size_t k = 0; k < num_particle_in_dam2_x; ++k)
      {
        _position_vectors[index] = v_pos2;
        ++index;
        v_pos2.x += ic.division_length;
      }

      v_pos2.x = v_pos2_init.x;
      v_pos2.z += ic.division_length;
    }

    v_pos2.x = v_pos2_init.x;
    v_pos2.z = v_pos2_init.z;
    v_pos2.y += ic.division_length;
  }

  //init neighborhood
  const float divide_length = _support_length;
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid>(solution_domain, divide_length, _position_vectors);

  // cal mass
  _mass_per_particle = this->cal_mass_per_particle_number_density_mean();
}

Fluid_Particles::~Fluid_Particles(void) = default;

void Fluid_Particles::update(void)
{
  this->time_integration();
}

const Vector3* Fluid_Particles::position_data(void) const
{
  return _position_vectors.data();
}

size_t Fluid_Particles::num_particle(void) const
{
  return _num_particle;
}

float Fluid_Particles::support_length(void) const
{
  return _support_length;
}

void Fluid_Particles::update_density_and_pressure(void)
{
  const float h     = _support_length / 2;
  const float m0    = _mass_per_particle;
  const float rho0  = _material_proeprty.rest_density;
  const float gamma = _material_proeprty.gamma;
  const float k     = _material_proeprty.pressure_coefficient;

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    auto& cur_rho = _densities[i];
    cur_rho       = 0.0;

    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const size_t neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      cur_rho += m0 * W(q);
    }

    _pressures[i] = k * (pow(cur_rho / rho0, gamma) - 1.0f);
  }
}

void Fluid_Particles::update_acceleration(void)
{
  this->update_density_and_pressure();

  const float m0 = _mass_per_particle;
  const float h  = _support_length / 2;
  const float mu = _material_proeprty.viscosity;

  //constexpr Vector3 v_a_gravity = {0.0f, -9.8f, 0.0f};
  constexpr Vector3 v_a_gravity = {0.0f, 0.0f, 0.0f};

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    Vector3 v_a_pressure(0.0f);
    Vector3 v_a_viscosity(0.0f);

    const float    rhoi = _densities[i];
    const float    pi   = _pressures[i];
    const Vector3& v_xi = _position_vectors[i];
    const Vector3& v_vi = _velocity_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      if (i == neighbor_index)
        continue;

      const float    rhoj = _densities[neighbor_index];
      const float    pj   = _pressures[neighbor_index];
      const Vector3& v_xj = _position_vectors[neighbor_index];
      const Vector3& v_vj = _velocity_vectors[neighbor_index];

      const Vector3 v_xij = v_xi - v_xj;
      const float   dist  = v_xij.Length();

      const auto q = dist / h;

      // distance가 0이면 grad_q 계산시 0으로 나누어서 문제가 됨
      if (dist == 0.0f)
        continue;

      // cal v_grad_pressure
      const auto v_grad_q      = 1.0f / (h * dist) * v_xij;
      const auto v_grad_kernel = dW_dq(q) * v_grad_q;
      const auto coeff         = m0 * (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));

      const auto v_grad_pressure = coeff * v_grad_kernel;

      // cal v_laplacian_velocity
      const auto v_vij  = v_vi - v_vj;
      const auto coeff2 = 10 * (m0 / rhoj) * v_vij.Dot(v_xij) / (v_xij.Dot(v_xij) + 0.01f * h * h);

      const auto v_laplacian_velocity = coeff2 * v_grad_kernel;

      // update acceleration
      v_a_pressure -= v_grad_pressure;
      v_a_viscosity += v_laplacian_velocity;
    }

    v_a_viscosity *= mu;

    auto& v_a = _accelaration_vectors[i];
    v_a       = v_a_pressure + v_a_viscosity + v_a_gravity;
  }
}

void Fluid_Particles::time_integration(void)
{
  constexpr float dt = 1.0e-5f;

  static float time   = 0.0f;
  static float target = 0.24f;

  time += dt;
  if (time > target)
  {
    target += 0.02f;
  }

  this->semi_implicit_euler(dt);
  //this->leap_frog_DKD(dt);
  //this->leap_frog_KDK(dt);
}

void Fluid_Particles::semi_implicit_euler(const float dt)
{
  _uptr_neighborhood->update(_position_vectors);
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    const auto& v_a = _accelaration_vectors[i];

    _velocity_vectors[i] += v_a * dt;
    _position_vectors[i] += _velocity_vectors[i] * dt;
  }
  this->apply_boundary_condition();
}

void Fluid_Particles::leap_frog_DKD(const float dt)
{
#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    const auto& v_v = _velocity_vectors[i];
    auto&       v_p = _position_vectors[i];

    v_p += v_v * 0.5f * dt;
  }
  this->apply_boundary_condition();
  _uptr_neighborhood->update(_position_vectors);

  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    const auto& v_a = _accelaration_vectors[i];
    auto&       v_v = _velocity_vectors[i];
    auto&       v_p = _position_vectors[i];

    v_v += dt * v_a;
    v_p += v_v * 0.5f * dt;
  }
}

void Fluid_Particles::leap_frog_KDK(const float dt)
{
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    auto& v_p = _position_vectors[i];
    auto& v_v = _velocity_vectors[i];

    const auto& v_a = _accelaration_vectors[i];

    v_v += 0.5f * dt * v_a;
    v_p += dt * v_v;
  }
  this->apply_boundary_condition();
  _uptr_neighborhood->update(_position_vectors);
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    auto& v_v = _velocity_vectors[i];

    const auto& v_a = _accelaration_vectors[i];

    v_v += 0.5f * dt * v_a;
  }
}

void Fluid_Particles::apply_boundary_condition(void)
{
  const float cor = 0.8f; // Coefficient Of Restitution

  const float radius       = _support_length * 0.5f;
  const float wall_x_start = _domain.x_start + radius;
  const float wall_x_end   = _domain.x_end - radius;
  const float wall_y_start = _domain.y_start + radius;
  const float wall_y_end   = _domain.y_end - radius;
  const float wall_z_start = _domain.z_start + radius;
  const float wall_z_end   = _domain.z_end - radius;

#pragma omp parallel for
  for (int i = 0; i < _num_particle; ++i)
  {
    auto& v_pos = _position_vectors[i];
    auto& v_vel = _velocity_vectors[i];

    if (v_pos.x < wall_x_start && v_vel.x < 0.0f)
    {
      v_vel.x *= -cor;
      v_pos.x = wall_x_start;
    }

    if (v_pos.x > wall_x_end && v_vel.x > 0.0f)
    {
      v_vel.x *= -cor;
      v_pos.x = wall_x_end;
    }

    if (v_pos.y < wall_y_start && v_vel.y < 0.0f)
    {
      v_vel.y *= -cor;
      v_pos.y = wall_y_start;
    }

    if (v_pos.y > wall_y_end && v_vel.y > 0.0f)
    {
      v_vel.y *= -cor;
      v_pos.y = wall_y_end;
    }

    if (v_pos.z < wall_z_start && v_vel.z < 0.0f)
    {
      v_vel.z *= -cor;
      v_pos.z = wall_z_start;
    }

    if (v_pos.z > wall_z_end && v_vel.z > 0.0f)
    {
      v_vel.z *= -cor;
      v_pos.z = wall_z_end;
    }
  }
}

float Fluid_Particles::W(const float q) const
{
  REQUIRE(q >= 0.0f, "q should be positive");

  constexpr float pi    = std::numbers::pi_v<float>;
  const float     h     = _support_length / 2;
  const float     coeff = 3.0f / (2.0f * pi * h * h * h);

  if (q < 1.0f)
    return coeff * (2.0f / 3.0f - q * q + 0.5f * q * q * q);
  else if (q < 2.0f)
    return coeff * pow(2.0f - q, 3.0f) / 6.0f;
  else // q >= 2.0f
    return 0.0f;
}

float Fluid_Particles::dW_dq(const float q) const
{
  REQUIRE(q >= 0.0f, "q should be positive");

  constexpr float pi    = std::numbers::pi_v<float>;
  const float     h     = _support_length / 2;
  const float     coeff = 3.0f / (2.0f * pi * h * h * h);

  if (q < 1.0f)
    return coeff * (-2.0f * q + 1.5f * q * q);
  else if (q < 2.0f)
    return coeff * -0.5f * (2.0f - q) * (2.0f - q);
  else // q >= 2.0f
    return 0.0f;
}

float Fluid_Particles::cal_mass_per_particle_1994_monaghan(const float total_volume) const
{
  const float volume_per_particle = total_volume / _num_particle;
  return _material_proeprty.rest_density * volume_per_particle;
}

float Fluid_Particles::cal_mass_per_particle_number_density_max(void) const
{
  const float h = _support_length / 2;

  float max_number_density = 0.0f;

  for (int i = 0; i < _num_particle; i++)
  {
    float number_density = 0.0;

    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      number_density += W(q);
    }

    max_number_density = (std::max)(max_number_density, number_density);
  }

  return _material_proeprty.rest_density / max_number_density;
}

float Fluid_Particles::cal_mass_per_particle_number_density_min(void) const
{
  const float h = _support_length / 2;

  float min_number_density = (std::numeric_limits<float>::max)();

  for (int i = 0; i < _num_particle; i++)
  {
    float number_density = 0.0;

    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      number_density += W(q);
    }

    min_number_density = (std::min)(min_number_density, number_density);
  }

  return _material_proeprty.rest_density / min_number_density;
}

float Fluid_Particles::cal_mass_per_particle_number_density_mean(void) const
{
  const float h = _support_length / 2;

  float avg_number_density = 0.0f;

  for (int i = 0; i < _num_particle; i++)
  {
    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      avg_number_density += W(q);
    }
  }

  avg_number_density /= _num_particle;
  return _material_proeprty.rest_density / avg_number_density;
}

} // namespace ms