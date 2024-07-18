#include "Particles.h"

#include "Neighborhood.h"

#include "../_lib/_header/msexception/Exception.h"
#include <algorithm>
#include <numbers>
#include <omp.h>

namespace ms
{
Fluid_Particles::Fluid_Particles(const Material_Property& material_property, const Initial_Condition_Cube& initial_condition, const Domain& domain)
    : _material_proeprty(material_property), _domain(domain)
{
  constexpr float  pi              = std::numbers::pi_v<float>;
  constexpr size_t desire_neighbor = 50;

  const auto& ic = initial_condition;

  const size_t num_particle_in_x = static_cast<size_t>(ic.domain.dx() / ic.division_length) + 1;
  const size_t num_particle_in_y = static_cast<size_t>(ic.domain.dy() / ic.division_length) + 1;
  const size_t num_particle_in_z = static_cast<size_t>(ic.domain.dz() / ic.division_length) + 1;

  _num_particle   = num_particle_in_x * num_particle_in_y * num_particle_in_z;
  _support_length = ic.division_length * 1.2f;

  _pressures.resize(_num_particle);
  _densities.resize(_num_particle, _material_proeprty.rest_density);
  _position_vectors.resize(_num_particle);
  _velocity_vectors.resize(_num_particle);
  _accelaration_vectors.resize(_num_particle);

  Vector3 v_pos = {ic.domain.x_start, ic.domain.y_start, ic.domain.z_start};

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

      v_pos.x = ic.domain.x_start;
      v_pos.z += ic.division_length;
    }

    v_pos.x = ic.domain.x_start;
    v_pos.z = ic.domain.z_start;
    v_pos.y += ic.division_length;
  }

  //_total_volume = ic.domain.dx() * ic.domain.dy() * ic.domain.dz();

  //const float particle_density = _num_particle / _total_volume;
  //_support_length              = 2.0f * std::pow(desire_neighbor * 3 / (particle_density * 4 * pi), 1.0f / 3.0f);
  //_mass_per_particle = this->cal_mass_per_particle_1994_monaghan(_total_volume, _num_particle);
}

void Fluid_Particles::update(const Neighborhood& neighborhood)
{
  this->update_density(neighborhood);
  this->update_pressure();
  this->update_acceleration(neighborhood);
  this->time_integration();
  this->apply_boundary_condition();
}

void Fluid_Particles::init_mass(const Neighborhood& neighborhood)
{
  const float h = _support_length / 2;

  float max_number_density = 0.0f;

  //#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    float number_density = 0.0;

    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = neighborhood.search(i);
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

  _mass_per_particle = _material_proeprty.rest_density / max_number_density;
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

const std::vector<Vector3>& Fluid_Particles::get_position_vectors(void) const
{
  return _position_vectors;
}

void Fluid_Particles::update_density(const Neighborhood& neighborhood)
{
  const float h    = _support_length / 2;
  const float m0   = _mass_per_particle;
  const float rho0 = _material_proeprty.rest_density;

//#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    auto& cur_rho = _densities[i];
    cur_rho       = 0.0;

    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = neighborhood.search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const size_t neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      cur_rho += m0 * W(q);
    }

    //cur_rho = std::clamp(cur_rho, rho0, 1.01f * rho0);
  }
}

void Fluid_Particles::update_pressure(void)
{
  constexpr float gamma = 7.0f;

  const float rho0 = _material_proeprty.rest_density;
  const float k    = _material_proeprty.pressure_coefficient;

  //Equation of State

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    const auto rho = _densities[i];

    _pressures[i] = k * (pow(rho / rho0, gamma) - 1.0f);
  }
}

void Fluid_Particles::update_acceleration(const Neighborhood& neighborhood)
{
  const float m0 = _mass_per_particle;
  const float h  = _support_length / 2;
  const float mu = _material_proeprty.viscosity;

  constexpr Vector3 v_a_gravity = {0.0f, -9.8f, 0.0f};

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    Vector3 v_a_pressure(0.0f);
    Vector3 v_a_viscosity(0.0f);

    const float    rhoi = _densities[i];
    const float    pi   = _pressures[i];
    const Vector3& v_xi = _position_vectors[i];
    const Vector3& v_vi = _velocity_vectors[i];

    const auto& neighbor_indexes = neighborhood.search(i);
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
      // 이후에 나누어질것이기 때문에 rho_i 무시
      const Vector3 v_grad_q      = 1.0f / (h * dist) * v_xij;
      const Vector3 v_grad_kernel = dW_dq(q) * v_grad_q;

      const auto coeff = m0 * (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));

      const Vector3 v_grad_pressure = coeff * v_grad_kernel;

      // cal laplacian_velocity
      //const auto    coeff2 = 10 * (m0 / rhoj) * v_xij.Dot(v_grad_kernel) / (v_xij.Dot(v_xij) + 0.01f * h * h);
      //const Vector3 v_vij  = v_vi - v_vj;

      //const Vector3 laplacian_velocity = coeff2 * v_vij;

      const auto v_vij  = v_vi - v_vj;
      const auto coeff2 = 10 * (m0 / rhoj) * v_vij.Dot(v_xij) / (v_xij.Dot(v_xij) + 0.01f * h * h);

      const Vector3 laplacian_velocity = coeff2 * v_grad_kernel;

      v_a_pressure -= v_grad_pressure;
      v_a_viscosity += laplacian_velocity;
    }

    v_a_viscosity *= mu;

    auto& v_a = _accelaration_vectors[i];
    v_a       = v_a_pressure + v_a_viscosity + v_a_gravity;
  }
}

void Fluid_Particles::time_integration(void)
{
  constexpr float dt = 5.0e-6f;
  //constexpr float dt = 1.0e-5f;

  const float m0 = _mass_per_particle;

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    const auto& v_a = _accelaration_vectors[i];

    _velocity_vectors[i] += v_a * dt;
    _position_vectors[i] += _velocity_vectors[i] * dt;
  }
}

void Fluid_Particles::apply_boundary_condition(void)
{
  const float cor = 0.5f; // Coefficient Of Restitution

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

float Fluid_Particles::cal_mass_per_particle_1994_monaghan(const float total_volume, const size_t num_particle) const
{
  const float volume_per_particle = total_volume / num_particle;
  return _material_proeprty.rest_density * volume_per_particle;
}

} // namespace ms