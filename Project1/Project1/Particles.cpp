#include "Particles.h"

#include "Neighborhood_Brute_Force.h"
#include "Neighborhood_Uniform_Grid.h"
#include "Print_Manager.h"

#include "../_lib/_header/msexception/Exception.h"
#include <algorithm>
#include <numbers>
#include <omp.h>

namespace ms
{

void print(const Vector3& v)
{
  std::cout << v.x << " " << v.y << " " << v.z << "\n";
}

Fluid_Particles::Fluid_Particles(
  const Material_Property&       property,
  const Initial_Condition_Cubes& initial_condition,
  const Domain&                  solution_domain)
    : _material_proeprty(property), _domain(solution_domain)

{
  const auto& ic = initial_condition;

  const auto num_cube = ic.domains.size();

  std::vector<size_t> num_particle_in_xs(num_cube);
  std::vector<size_t> num_particle_in_ys(num_cube);
  std::vector<size_t> num_particle_in_zs(num_cube);

  for (size_t i = 0; i < num_cube; ++i)
  {
    const auto& cube = ic.domains[i];

    const auto num_x = static_cast<size_t>(cube.dx() / ic.division_length) + 1;
    const auto num_y = static_cast<size_t>(cube.dy() / ic.division_length) + 1;
    const auto num_z = static_cast<size_t>(cube.dz() / ic.division_length) + 1;

    num_particle_in_xs[i] = num_x;
    num_particle_in_ys[i] = num_y;
    num_particle_in_zs[i] = num_z;

    _num_fluid_particle += num_x * num_y * num_z;
  }

  _support_length = 1.2f * ic.division_length;

  _fluid_pressures.resize(_num_fluid_particle);
  _fluid_densities.resize(_num_fluid_particle, _material_proeprty.rest_density);
  _fluid_position_vectors.resize(_num_fluid_particle);
  _fluid_velocity_vectors.resize(_num_fluid_particle);
  _fluid_acceleration_vectors.resize(_num_fluid_particle);

  // init position
  size_t index = 0;
  for (size_t i = 0; i < num_cube; ++i)
  {
    const auto& cube = ic.domains[i];

    const Vector3 v_pos_init = {cube.x_start, cube.y_start, cube.z_start};
    Vector3       v_pos      = v_pos_init;

    const auto num_x = num_particle_in_xs[i];
    const auto num_y = num_particle_in_ys[i];
    const auto num_z = num_particle_in_zs[i];

    for (size_t j = 0; j < num_y; ++j)
    {
      for (size_t k = 0; k < num_z; ++k)
      {
        for (size_t l = 0; l < num_x; ++l)
        {
          _fluid_position_vectors[index] = v_pos;
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
  }

  //this->init_boundary_position_and_normal(solution_domain, ic.division_length * 0.5f);

  // init neighborhood
  const float divide_length = _support_length;
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid>(solution_domain, divide_length, _fluid_position_vectors, _boundary_position_vectors);


  //float total_volume = 0.0f;
  //for (size_t i = 0; i < num_cube; ++i)
  //{
  //  const auto& cube = ic.domains[i];
  //  total_volume += cube.dx() * cube.dy() * cube.dz();
  //}

  //_mass_per_particle = this->cal_mass_per_particle_1994_monaghan(total_volume);
  this->init_mass();
}

Fluid_Particles::~Fluid_Particles(void) = default;

void Fluid_Particles::update(void)
{
  this->time_integration();
}

const Vector3* Fluid_Particles::fluid_particle_position_data(void) const
{
  return _fluid_position_vectors.data();
}

const Vector3* Fluid_Particles::boundary_particle_position_data(void) const
{
  return _boundary_position_vectors.data();
}

size_t Fluid_Particles::num_fluid_particle(void) const
{
  return _num_fluid_particle;
}

size_t Fluid_Particles::num_boundary_particle(void) const
{
  return _boundary_position_vectors.size();
}

float Fluid_Particles::support_length(void) const
{
  return _support_length;
}

void Fluid_Particles::update_density_and_pressure(void)
{
  const float h = _support_length / 2;
  // const float m0    = _mass_per_particle;
  const float rho0  = _material_proeprty.rest_density;
  const float gamma = _material_proeprty.gamma;
  const float k     = _material_proeprty.pressure_coefficient;

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& cur_rho = _fluid_densities[i];
    cur_rho       = 0.0;

    const auto& v_xi = _fluid_position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search_for_fluid(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    const float m0 = _mass[i];

    for (int j = 0; j < num_neighbor; j++)
    {
      const size_t neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _fluid_position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      // const float m0 = _mass[neighbor_index];

      cur_rho += m0 * W(q);
    }

    _fluid_pressures[i] = k * (pow(cur_rho / rho0, gamma) - 1.0f);
  }
}

void Fluid_Particles::update_acceleration(void)
{
  this->update_density_and_pressure();

  // const float m0    = _mass_per_particle;
  const float h     = _support_length / 2;
  const float mu    = _material_proeprty.viscosity;
  const float sigma = 0.8e0f;

  //constexpr Vector3 v_a_gravity = {0.0f, -9.8f, 0.0f};
   constexpr Vector3 v_a_gravity = {0.0f, 0.0f, 0.0f};

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    Vector3 v_a_pressure(0.0f);
    Vector3 v_a_viscosity(0.0f);
    Vector3 v_a_surface_tension(0.0f);

    const float    rhoi = _fluid_densities[i];
    const float    pi   = _fluid_pressures[i];
    const Vector3& v_xi = _fluid_position_vectors[i];
    const Vector3& v_vi = _fluid_velocity_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search_for_fluid(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const float m0 = _mass[neighbor_index];

      if (i == neighbor_index)
        continue;

      const float    rhoj = _fluid_densities[neighbor_index];
      const float    pj   = _fluid_pressures[neighbor_index];
      const Vector3& v_xj = _fluid_position_vectors[neighbor_index];
      const Vector3& v_vj = _fluid_velocity_vectors[neighbor_index];

      const Vector3 v_xij = v_xi - v_xj;
      const float   dist  = v_xij.Length();

      const Vector3 v_xij_normal = v_xij / dist;

      const auto q = dist / h;

      // v_a_surface_tension -= sigma * W(q) * v_xij;
      v_a_surface_tension -= sigma * m0 * W(q) * v_xij_normal;

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
      const auto coeff2 = mu * 10 * (m0 / rhoj) * v_vij.Dot(v_xij) / (v_xij.Dot(v_xij) + 0.01f * h * h);

      const auto v_laplacian_velocity = coeff2 * v_grad_kernel;

      // update acceleration
      v_a_pressure -= v_grad_pressure;
      v_a_viscosity += v_laplacian_velocity;
    }

    auto& v_a = _fluid_acceleration_vectors[i];
    v_a       = v_a_pressure + v_a_viscosity + v_a_gravity + v_a_surface_tension;
  }

  // consider boundary
  const auto num_boundary_particle = _boundary_position_vectors.size();

  for (int i = 0; i < num_boundary_particle; ++i)
  {
    const auto& neighbor_fpids = _uptr_neighborhood->search_for_boundary(i);

    const auto v_xb = _boundary_position_vectors[i];
    const auto v_nb = _boundary_normal_vectors[i];

    for (auto fpid : neighbor_fpids)
    {
      const auto v_xf  = _fluid_position_vectors[fpid];
      const auto v_xfb = v_xf - v_xb;
      const auto dist  = v_xfb.Length();

      const auto coeff = 0.5f * B(dist);

      const auto v_a_boundary = coeff * v_nb;
      _fluid_acceleration_vectors[fpid] += v_a_boundary;
    }
  }

  //if (Print_Manager::can_record())
  //{
  //  Print_Manager::record() << _fluid_position_vectors[0].y << " "
  //                          << _fluid_velocity_vectors[0].y << " "
  //                          << _fluid_acceleration_vectors[0].y << "\n";

  //  Print_Manager::print();
  //}

  // std::cout << "position :" << _fluid_position_vectors[0].y << "\n";
  // std::cout << "velocity :" << _fluid_velocity_vectors[0].y << "\n";
  // std::cout << "acclerat :" << _fluid_acceleration_vectors[0].y << "\n\n";

  // 1번 Particle이 Z = -1.0 Boundary를 뚫고 나와버림.. 왜 그러는지 확인해보기

  // if (_fluid_position_vectors[0].y < 0.01f && _fluid_accelaration_vectors[0].y < 0.0f)
  // if (_fluid_velocity_vectors[0].y > 1.0f)
  //{
  //   int debug = 0;
  // }
}

void Fluid_Particles::time_integration(void)
{
  constexpr float dt = 1.0e-5f;

  //static float time   = 0.0f;
  //static float target = 0.01f;
  //time += dt;

  //if (time > target)
  //{
  //  target += 0.01f;

  //  Print_Manager::start_record();
  //  Print_Manager::record() << time << " ";
  //}

  this->semi_implicit_euler(dt);
  // this->leap_frog_DKD(dt);
  // this->leap_frog_KDK(dt);

  //if (time > 2.0f)
  //  std::exit(523);
}

void Fluid_Particles::semi_implicit_euler(const float dt)
{
  _uptr_neighborhood->update(_fluid_position_vectors, _boundary_position_vectors);
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_a = _fluid_acceleration_vectors[i];

    _fluid_velocity_vectors[i] += v_a * dt;
    _fluid_position_vectors[i] += _fluid_velocity_vectors[i] * dt;
  }
   this->apply_boundary_condition();
}

void Fluid_Particles::leap_frog_DKD(const float dt)
{
#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_v = _fluid_velocity_vectors[i];
    auto&       v_p = _fluid_position_vectors[i];

    v_p += v_v * 0.5f * dt;
  }
  // this->apply_boundary_condition();
  _uptr_neighborhood->update(_fluid_position_vectors, _boundary_position_vectors);

  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_a = _fluid_acceleration_vectors[i];
    auto&       v_v = _fluid_velocity_vectors[i];
    auto&       v_p = _fluid_position_vectors[i];

    v_v += dt * v_a;
    v_p += v_v * 0.5f * dt;
  }
}

void Fluid_Particles::leap_frog_KDK(const float dt)
{
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& v_p = _fluid_position_vectors[i];
    auto& v_v = _fluid_velocity_vectors[i];

    const auto& v_a = _fluid_acceleration_vectors[i];

    v_v += 0.5f * dt * v_a;
    v_p += dt * v_v;
  }
  // this->apply_boundary_condition();
  _uptr_neighborhood->update(_fluid_position_vectors, _boundary_position_vectors);
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& v_v = _fluid_velocity_vectors[i];

    const auto& v_a = _fluid_acceleration_vectors[i];

    v_v += 0.5f * dt * v_a;
  }
}

 void Fluid_Particles::apply_boundary_condition(void)
{
   const float cor  = 0.5f; // Coefficient Of Restitution
   const float cor2 = 0.5f; // Coefficient Of Restitution

   const float radius       = _support_length * 0.5f;
   const float wall_x_start = _domain.x_start + radius;
   const float wall_x_end   = _domain.x_end - radius;
   const float wall_y_start = _domain.y_start + radius;
   const float wall_y_end   = _domain.y_end - radius;
   const float wall_z_start = _domain.z_start + radius;
   const float wall_z_end   = _domain.z_end - radius;

 #pragma omp parallel for
   for (int i = 0; i < _num_fluid_particle; ++i)
   {
     auto& v_pos = _fluid_position_vectors[i];
     auto& v_vel = _fluid_velocity_vectors[i];

     if (v_pos.x < wall_x_start && v_vel.x < 0.0f)
     {
       v_vel.x *= -cor2;
       v_pos.x = wall_x_start;
     }

     if (v_pos.x > wall_x_end && v_vel.x > 0.0f)
     {
       v_vel.x *= -cor2;
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
       v_vel.z *= -cor2;
       v_pos.z = wall_z_start;
     }

     if (v_pos.z > wall_z_end && v_vel.z > 0.0f)
     {
       v_vel.z *= -cor2;
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

float Fluid_Particles::B(const float dist) const
{
  // const float h = 2*_support_length;
  const float h    = _support_length * 0.5f;
  const float v_c2 = _material_proeprty.sqaure_sound_speed;
  const float q    = dist / h;

  const float coeff = 0.02f * v_c2 / dist;

  float f = 0;
  if (q < 2.0f / 3.0f)
    f = 2.0f / 3.0f;
  else if (q < 1.0f)
    f = 2 * q - 1.5f * q * q;
  else if (q < 2.0f)
    f = 0.5f * (2 - q) * (2 - q);
  else
    return 0.0f;

  return coeff * f;
}

float Fluid_Particles::cal_mass_per_particle_1994_monaghan(const float total_volume) const
{
  const float volume_per_particle = total_volume / _num_fluid_particle;
  return _material_proeprty.rest_density * volume_per_particle;
}

void Fluid_Particles::init_boundary_position_and_normal(const Domain& solution_domain, const float divide_length)
{
  const auto num_x = static_cast<size_t>(std::ceil(solution_domain.dx() / divide_length)) + 1;
  const auto num_y = static_cast<size_t>(std::ceil(solution_domain.dy() / divide_length)) + 1;
  const auto num_z = static_cast<size_t>(std::ceil(solution_domain.dz() / divide_length)) + 1;

  const auto num_points = num_x * num_y * num_z - (num_x - 2) * (num_y - 2) * (num_z - 2);

  _boundary_position_vectors.resize(num_points);
  _boundary_normal_vectors.resize(num_points);

  const float x_start = solution_domain.x_start;
  const float x_end   = x_start + divide_length * (num_x - 1);
  const float y_start = solution_domain.y_start;
  const float y_end   = y_start + divide_length * (num_y - 1);
  const float z_start = solution_domain.z_start;
  const float z_end   = z_start + divide_length * (num_z - 1);

  size_t index = 0;

  { // 밑면
    Vector3 v_pos = {x_start, y_start, z_start};

    for (size_t i = 0; i < num_z; ++i)
    {
      for (size_t j = 0; j < num_x; ++j)
      {
        _boundary_position_vectors[index] = v_pos;
        _boundary_normal_vectors[index]   = Vector3{0.0f, 1.0f, 0.0f};
        v_pos.x += divide_length;
        ++index;
      }

      v_pos.x = x_start;
      v_pos.z += divide_length;
    }
  }
  { // 윗면
    Vector3 v_pos = {x_start, y_end, z_start};

    for (size_t i = 0; i < num_z; ++i)
    {
      for (size_t j = 0; j < num_x; ++j)
      {
        _boundary_position_vectors[index] = v_pos;
        _boundary_normal_vectors[index]   = Vector3{0.0f, -1.0f, 0.0f};
        v_pos.x += divide_length;
        ++index;
      }

      v_pos.x = x_start;
      v_pos.z += divide_length;
    }
  }
  { // 앞면
    Vector3 v_pos = {x_start, y_start + divide_length, z_start};

    for (size_t i = 1; i < num_y - 1; ++i)
    {
      for (size_t j = 0; j < num_x; ++j)
      {
        _boundary_position_vectors[index] = v_pos;
        _boundary_normal_vectors[index]   = Vector3{0.0f, 0.0f, -1.0f};
        v_pos.x += divide_length;
        ++index;
      }

      v_pos.x = x_start;
      v_pos.y += divide_length;
    }
  }
  { // 뒷면
    Vector3 v_pos = {x_start, y_start + divide_length, z_end};

    for (size_t i = 1; i < num_y - 1; ++i)
    {
      for (size_t j = 0; j < num_x; ++j)
      {
        _boundary_position_vectors[index] = v_pos;
        _boundary_normal_vectors[index]   = Vector3{0.0f, 0.0f, 1.0f};
        v_pos.x += divide_length;
        ++index;
      }

      v_pos.x = x_start;
      v_pos.y += divide_length;
    }
  }
  { // Left
    Vector3 v_pos = {x_start, y_start + divide_length, z_start + divide_length};

    for (size_t i = 1; i < num_y - 1; ++i)
    {
      for (size_t j = 1; j < num_z - 1; ++j)
      {
        _boundary_position_vectors[index] = v_pos;
        _boundary_normal_vectors[index]   = Vector3{1.0f, 0.0f, 0.0f};
        v_pos.z += divide_length;
        ++index;
      }

      v_pos.z = z_start + divide_length;
      v_pos.y += divide_length;
    }
  }
  { // Right
    Vector3 v_pos = {x_end, y_start + divide_length, z_start + divide_length};

    for (size_t i = 1; i < num_y - 1; ++i)
    {
      for (size_t j = 1; j < num_z - 1; ++j)
      {
        _boundary_position_vectors[index] = v_pos;
        _boundary_normal_vectors[index]   = Vector3{-1.0f, 0.0f, 0.0f};
        v_pos.z += divide_length;
        ++index;
      }

      v_pos.z = z_start + divide_length;
      v_pos.y += divide_length;
    }
  }
}

float Fluid_Particles::cal_mass_per_particle_number_density_max(void) const
{
  const float h = _support_length / 2;

  float max_number_density = 0.0f;

  for (int i = 0; i < _num_fluid_particle; i++)
  {
    float number_density = 0.0;

    const auto& v_xi = _fluid_position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search_for_fluid(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _fluid_position_vectors[neighbor_index];

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

  for (int i = 0; i < _num_fluid_particle; i++)
  {
    float number_density = 0.0;

    const auto& v_xi = _fluid_position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search_for_fluid(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _fluid_position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      number_density += W(q);
    }

    min_number_density = (std::min)(min_number_density, number_density);
  }

  return _material_proeprty.rest_density / min_number_density;
}

void Fluid_Particles::init_mass(void)
{
  _mass_per_particle = this->cal_mass_per_particle_number_density_mean();
  _mass.resize(_num_fluid_particle, _mass_per_particle);

  //_mass.resize(_num_particle);

  // const float rho0 = _material_proeprty.rest_density;
  // const float h    = _support_length / 2;

  // for (int i = 0; i < _num_particle; i++)
  //{
  //   float number_density = 0.0;

  //  const auto& v_xi = _position_vectors[i];

  //  const auto& neighbor_indexes = _uptr_neighborhood->search(i);
  //  const auto  num_neighbor     = neighbor_indexes.size();

  //  for (int j = 0; j < num_neighbor; j++)
  //  {
  //    const auto neighbor_index = neighbor_indexes[j];

  //    const auto& v_xj = _position_vectors[neighbor_index];

  //    const float dist = (v_xi - v_xj).Length();
  //    const auto  q    = dist / h;

  //    number_density += W(q);
  //  }

  //  _mass[i] = rho0 / number_density;
  //}
}

float Fluid_Particles::cal_mass_per_particle_number_density_mean(void) const
{
  const float h = _support_length / 2;

  float avg_number_density = 0.0f;

  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_xi = _fluid_position_vectors[i];

    const auto& neighbor_indexes = _uptr_neighborhood->search_for_fluid(i);
    const auto  num_neighbor     = neighbor_indexes.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _fluid_position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      avg_number_density += W(q);
    }
  }

  avg_number_density /= _num_fluid_particle;
  return _material_proeprty.rest_density / avg_number_density;
}

} // namespace ms