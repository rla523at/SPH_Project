#include "WCSPH.h"

#include "Debugger.h"
#include "Kernel.h"
#include "Neighborhood_Brute_Force.h"
#include "Neighborhood_Uniform_Grid.h"

#include "../../_lib/_header/msexception/Exception.h"
#include <algorithm>
#include <numbers>
#include <omp.h>

namespace ms
{
WCSPH::WCSPH(
  const Material_Property&       property,
  const Initial_Condition_Cubes& initial_condition,
  const Domain&                  solution_domain)
    : _material_proeprty(property), _domain(solution_domain)

{
  constexpr float smooth_length_param = 1.1f;

  _dt = 1.0e-03f;

  const auto& ic = initial_condition;

  _particle_radius                  = ic.particle_spacing;
  _smoothing_length                 = smooth_length_param * ic.particle_spacing;
  _fluid_particles.position_vectors = ic.cal_initial_position();
  _num_fluid_particle               = _fluid_particles.position_vectors.size();

  const auto rho0 = _material_proeprty.rest_density; 

  _fluid_particles.pressures.resize(_num_fluid_particle);
  _fluid_particles.densities.resize(_num_fluid_particle, rho0);
  _fluid_particles.velocity_vectors.resize(_num_fluid_particle);
  _fluid_particles.acceleration_vectors.resize(_num_fluid_particle);

  //this->init_boundary_position_and_normal(solution_domain, ic.particle_spacing * 0.5f);

  _uptr_kernel = std::make_unique<Cubic_Spline_Kernel>(_smoothing_length);

  const float divide_length = _uptr_kernel->supprot_radius();
  _uptr_neighborhood        = std::make_unique<Neighborhood_Uniform_Grid>(solution_domain, divide_length, _fluid_particles.position_vectors, _boundary_position_vectors);

  _mass_per_particle = this->cal_mass_per_particle_number_density_max();

  const float L        = ic.particle_spacing;
  _volume_per_particle = L * L * L;
}

WCSPH::~WCSPH(void) = default;

void WCSPH::update(void)
{
  this->time_integration();
}

const Vector3* WCSPH::fluid_particle_position_data(void) const
{
  return _fluid_particles.position_vectors.data();
}

//const Vector3* WCSPH::boundary_particle_position_data(void) const
//{
//  return _boundary_position_vectors.data();
//}

const float* WCSPH::fluid_particle_density_data(void) const
{
  return _fluid_particles.densities.data();
}

size_t WCSPH::num_fluid_particle(void) const
{
  return _num_fluid_particle;
}

//size_t WCSPH::num_boundary_particle(void) const
//{
//  return _boundary_position_vectors.size();
//}

float WCSPH::particle_radius(void) const
{
  return _particle_radius;
}

void WCSPH::update_density_and_pressure(void)
{
  const float h     = _smoothing_length;
  const float rho0  = _material_proeprty.rest_density;
  const float gamma = _material_proeprty.gamma;
  const float k     = _material_proeprty.pressure_coefficient;
  const float m0    = _mass_per_particle;

  auto& densities = _fluid_particles.densities;
  auto& pressures = _fluid_particles.pressures;

  std::vector<float> debug1(_num_fluid_particle);
  std::vector<size_t> debug2(_num_fluid_particle);

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& rho = densities[i];
    auto& p   = pressures[i];

    //update denisty
    rho = 0.0;

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_distances    = neighbor_informations.distances;
    const auto  num_neighbor          = neighbor_distances.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const float distance = neighbor_distances[j];
      rho += _uptr_kernel->W(distance);
    }

    rho *= m0;

    debug1[i] = rho;
    debug2[i] = num_neighbor;


    if (rho < 1.00f * rho0)
      rho = rho0;

    //update pressure
    p = k * (pow(rho / rho0, gamma) - 1.0f);
  }

  print_sort_and_count(debug1, 1.0e-1f);
  print_sort_and_count(debug2);
  exit(523);
}

void WCSPH::update_acceleration(void)
{
  constexpr Vector3 v_a_gravity = {0.0f, -9.8f, 0.0f};
  //constexpr Vector3 v_a_gravity = {0.0f, 0.0f, 0.0f};

  this->update_density_and_pressure();

  //common constnat
  const float m0 = _mass_per_particle;

  //viscosity constant
  const float h                   = _smoothing_length;
  const float viscousity_constant = 10.0f * m0 * _material_proeprty.viscosity;
  const float regularization_term = 0.01f * h * h;

  //surface tension constant
  const float sigma = 0.0e0f;

  //references
  auto& densities     = _fluid_particles.densities;
  auto& pressures     = _fluid_particles.pressures;
  auto& velocities    = _fluid_particles.velocity_vectors;
  auto& accelerations = _fluid_particles.acceleration_vectors;

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    Vector3 v_a_pressure(0.0f);
    Vector3 v_a_viscosity(0.0f);
    //Vector3 v_a_surface_tension(0.0f);

    const float rhoi = densities[i];
    const float pi   = pressures[i];
    const auto& v_vi = velocities[i];

    const auto& neighbor_informations      = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_indexes           = neighbor_informations.indexes;
    const auto& neighbor_translate_vectors = neighbor_informations.translate_vectors;
    const auto& neighbor_distances         = neighbor_informations.distances;
    const auto  num_neighbor               = neighbor_distances.size();

    for (size_t j = 0; j < num_neighbor; j++)
    {
      const auto neighbor_index = neighbor_indexes[j];

      if (i == neighbor_index)
        continue;

      const float rhoj = densities[neighbor_index];
      const float pj   = pressures[neighbor_index];
      const auto& v_vj = velocities[neighbor_index];

      const auto& v_xij     = neighbor_translate_vectors[j];
      const auto  distance  = neighbor_distances[j];
      const auto  distance2 = distance * distance;

      // distance가 0이면 v_xij_normal과 grad_q 계산시 0으로 나누어서 문제가 됨
      if (distance == 0.0f)
        continue;

      //const Vector3 v_xij_normal = v_xij / distance;
      //v_a_surface_tension -= sigma * m0 * W(distance) * v_xij_normal;

      // cal v_grad_kernel
      const auto v_grad_q      = 1.0f / (h * distance) * v_xij;
      const auto v_grad_kernel = _uptr_kernel->dWdq(distance) * v_grad_q;

      // cal v_grad_pressure
      const auto coeff           = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
      const auto v_grad_pressure = coeff * v_grad_kernel;

      // cal v_laplacian_velocity
      const auto v_vij  = v_vi - v_vj;
      const auto coeff2 = v_vij.Dot(v_xij) / (rhoj * (distance2 + regularization_term));

      const auto v_laplacian_velocity = coeff2 * v_grad_kernel;

      // update acceleration
      v_a_pressure -= v_grad_pressure;
      v_a_viscosity += v_laplacian_velocity;
    }

    auto& v_a = accelerations[i];

    //v_a = m0 * v_a_pressure + viscousity_constant * v_a_viscosity + v_a_gravity + v_a_surface_tension;
    
    v_a = m0 * v_a_pressure + viscousity_constant * v_a_viscosity + v_a_gravity;
  }

  //// consider boundary
  //const auto num_boundary_particle = _boundary_position_vectors.size();

  //for (int i = 0; i < num_boundary_particle; ++i)
  //{
  //  const auto& neighbor_fpids = _uptr_neighborhood->search_for_boundary(i);

  //  const auto v_xb = _boundary_position_vectors[i];
  //  const auto v_nb = _boundary_normal_vectors[i];

  //  for (auto fpid : neighbor_fpids)
  //  {
  //    const auto v_xf  = _fluid_position_vectors[fpid];
  //    const auto v_xfb = v_xf - v_xb;
  //    const auto distance  = v_xfb.Length();

  //    const auto coeff = 0.5f * B(distance);

  //    const auto v_a_boundary = coeff * v_nb;
  //    _fluid_acceleration_vectors[fpid] += v_a_boundary;
  //  }
  //}
}

void WCSPH::time_integration(void)
{
  this->semi_implicit_euler(_dt);
  // this->leap_frog_DKD(dt);
  // this->leap_frog_KDK(dt);
}

void WCSPH::semi_implicit_euler(const float dt)
{
  _uptr_neighborhood->update(_fluid_particles.position_vectors, _boundary_position_vectors);
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_a = _fluid_particles.acceleration_vectors[i];

    _fluid_particles.velocity_vectors[i] += v_a * dt;
    _fluid_particles.position_vectors[i] += _fluid_particles.velocity_vectors[i] * dt;
  }
  this->apply_boundary_condition();
}

void WCSPH::leap_frog_DKD(const float dt)
{
#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_v = _fluid_particles.velocity_vectors[i];
    auto&       v_p = _fluid_particles.position_vectors[i];

    v_p += v_v * 0.5f * dt;
  }
  // this->apply_boundary_condition();
  _uptr_neighborhood->update(_fluid_particles.position_vectors, _boundary_position_vectors);

  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    const auto& v_a = _fluid_particles.acceleration_vectors[i];
    auto&       v_v = _fluid_particles.velocity_vectors[i];
    auto&       v_p = _fluid_particles.position_vectors[i];

    v_v += dt * v_a;
    v_p += v_v * 0.5f * dt;
  }
}

void WCSPH::leap_frog_KDK(const float dt)
{
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& v_p = _fluid_particles.position_vectors[i];
    auto& v_v = _fluid_particles.velocity_vectors[i];

    const auto& v_a = _fluid_particles.acceleration_vectors[i];

    v_v += 0.5f * dt * v_a;
    v_p += dt * v_v;
  }
  // this->apply_boundary_condition();
  _uptr_neighborhood->update(_fluid_particles.position_vectors, _boundary_position_vectors);
  this->update_acceleration();

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& v_v = _fluid_particles.velocity_vectors[i];

    const auto& v_a = _fluid_particles.acceleration_vectors[i];

    v_v += 0.5f * dt * v_a;
  }
}

void WCSPH::apply_boundary_condition(void)
{
  const float cor  = 0.5f; // Coefficient Of Restitution
  const float cor2 = 0.5f; // Coefficient Of Restitution

  const float radius       = this->particle_radius();
  const float wall_x_start = _domain.x_start + radius;
  const float wall_x_end   = _domain.x_end - radius;
  const float wall_y_start = _domain.y_start + radius;
  const float wall_y_end   = _domain.y_end - radius;
  const float wall_z_start = _domain.z_start + radius;
  const float wall_z_end   = _domain.z_end - radius;

#pragma omp parallel for
  for (int i = 0; i < _num_fluid_particle; ++i)
  {
    auto& v_pos = _fluid_particles.position_vectors[i];
    auto& v_vel = _fluid_particles.velocity_vectors[i];

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

float WCSPH::B(const float dist) const
{
  // const float h = 2*_support_radius;
  const float h    = _smoothing_length;
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

float WCSPH::cal_mass_per_particle_1994_monaghan(const float total_volume) const
{
  const float volume_per_particle = total_volume / _num_fluid_particle;
  return _material_proeprty.rest_density * volume_per_particle;
}

float WCSPH::cal_number_density(const size_t fluid_particle_id) const
{
  float number_density = 0.0;

  const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(fluid_particle_id);
  const auto& neighbor_distances    = neighbor_informations.distances;
  const auto  num_neighbor          = neighbor_distances.size();

  for (size_t j = 0; j < num_neighbor; j++)
  {
    const float dist = neighbor_distances[j];
    number_density += _uptr_kernel->W(dist);
  }

  return number_density;
}

void WCSPH::init_boundary_position_and_normal(const Domain& solution_domain, const float divide_length)
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

float WCSPH::cal_mass_per_particle_number_density_max(void) const
{
  const float m0 = _material_proeprty.rest_density;

  float max_number_density = 0.0f;

  for (size_t i = 0; i < _num_fluid_particle; i++)
  {
    const float number_density = this->cal_number_density(i);

    max_number_density = (std::max)(max_number_density, number_density);
  }

  return m0 / max_number_density;
}

float WCSPH::cal_mass_per_particle_number_density_min(void) const
{
  const float m0 = _material_proeprty.rest_density;

  float min_number_density = (std::numeric_limits<float>::max)();

  for (size_t i = 0; i < _num_fluid_particle; i++)
  {
    const float number_density = this->cal_number_density(i);

    min_number_density = (std::min)(min_number_density, number_density);
  }

  return m0 / min_number_density;
}

float WCSPH::cal_mass_per_particle_number_density_mean(void) const
{
  const float m0 = _material_proeprty.rest_density;

  float avg_number_density = 0.0f;

  for (int i = 0; i < _num_fluid_particle; i++)
    avg_number_density += this->cal_number_density(i);

  avg_number_density /= _num_fluid_particle;

  return m0 / avg_number_density;
}

} // namespace ms