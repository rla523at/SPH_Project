cbuffer PCISPH_Constant_Data : register(b0)
{
  float g_dt;
  float g_rho0; // rest_density
  float g_viscosity;
  float g_allow_density_error;
  float g_particle_radius;
  float g_h;  // smoothing_length
  float g_m0; // mass_per_particle
  uint  g_min_iter;
  uint  g_max_iter;
};

