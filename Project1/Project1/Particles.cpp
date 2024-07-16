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

  _num_particle = static_cast<size_t>(std::pow(ic.num_particle_in_edge, 3));
  _total_volume = std::pow(ic.edge_length, 3.0f);

  _pressures.resize(_num_particle);
  _densities.resize(_num_particle, _material_proeprty.rest_density);
  _position_vectors.resize(_num_particle);
  _velocity_vectors.resize(_num_particle);
  _force_vectors.resize(_num_particle);

  const float particle_density = _num_particle / _total_volume;

  _support_length = 1.0f * std::pow(desire_neighbor * 3 / (particle_density * 4 * pi), 1.0f / 3.0f);

  const float delta  = ic.edge_length / (ic.num_particle_in_edge - 1);
  const float radius = _support_length * 0.5f;

  Vector3 pos     = ic.init_pos + Vector3(radius, radius, radius);
  Vector3 end_pos = pos + Vector3(ic.edge_length/2, ic.edge_length, ic.edge_length);

  for (int i = 0; i < _num_particle; ++i)
  {
    _position_vectors[i] = pos;

    pos.x += delta;
    if (pos.x > end_pos.x)
    {
      pos.x = ic.init_pos.x + radius;
      pos.z += delta;
      if (pos.z > end_pos.z)
      {
        pos.z = ic.init_pos.z + radius;
        pos.y += delta;
      }
    }
  }

  _mass_per_particle = this->cal_mass_per_particle_1994_monaghan(_total_volume, _num_particle);
}

void Fluid_Particles::update(const Neighborhood& neighborhood)
{
  this->update_density_with_clamp(neighborhood);
  this->update_pressure();
  this->update_force(neighborhood);
  this->time_integration();
  this->apply_boundary_condition();
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

void Fluid_Particles::update_density_with_clamp(const Neighborhood& neighborhood)
{
  const float h    = _support_length / 2;
  const float m0   = _mass_per_particle;
  const float rho0 = _material_proeprty.rest_density;

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    auto& cur_rho = _densities[i];
    cur_rho       = 0.0;

    const auto& v_xi = _position_vectors[i];

    const auto& neighbor_indexes = neighborhood.search(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    size_t real_neighbor = 0;

    for (int j = 0; j < num_neighbor; j++)
    {
      const size_t neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      ++real_neighbor;

      cur_rho += m0 * W(q);
    }

    cur_rho = std::clamp(cur_rho, rho0, 1.01f * rho0);
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

void Fluid_Particles::update_force(const Neighborhood& neighborhood)
{
  const float m0 = _mass_per_particle;
  const float h  = _support_length / 2;
  const float mu = _material_proeprty.viscosity;

  //const Vector3 v_gravity_force = m0 * Vector3(0.0f, -9.8f, 0.0f);
  const Vector3 v_gravity_force = Vector3(0.0f, -9.8f, 0.0f);

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    Vector3 v_pressure_force(0.0f);
    Vector3 v_viscosity_force(0.0f);

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

      // distance가 0이면 df_dq = 0이되고 0으로나눠서 오류가남
      // 이거 없어도 문제 없을거 같은데..? 문제 상황이 뭐였더라...
      if (dist < 0.03f)
        continue;

      // cal v_grad_pressure
      // 이후에 나누어질것이기 때문에 rho_i 무시
      const Vector3 v_grad_q      = 1.0f / (h * dist) * v_xij;
      const Vector3 v_grad_kernel = dW_dq(q) * v_grad_q;

      const auto coeff = m0 * (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));

      const Vector3 v_grad_pressure = coeff * v_grad_kernel;

      // cal laplacian_velocity
      const auto    coeff2 = 2 * (m0 / rhoj) * v_xij.Dot(v_grad_kernel) / (v_xij.Dot(v_xij) + 0.01f * h * h);
      const Vector3 v_vij  = v_vi - v_vj;

      const Vector3 laplacian_velocity = coeff2 * v_vij;

      v_pressure_force -= v_grad_pressure;
      v_viscosity_force += laplacian_velocity;
    }

    v_viscosity_force *= mu;

    _force_vectors[i] = v_pressure_force + v_viscosity_force + v_gravity_force;
  }
}

void Fluid_Particles::time_integration(void)
{
  constexpr float dt = 1.0e-3f;

  const float m0 = _mass_per_particle;

#pragma omp parallel for
  for (int i = 0; i < _num_particle; i++)
  {
    const auto& v_a = _force_vectors[i];

    //const auto& v_f = _force_vectors[i];
    //const auto  v_a = v_f / m0;

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