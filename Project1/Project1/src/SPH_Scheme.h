#pragma once
#include "Abbreviation.h"

namespace ms
{
class SPH_Scheme
{
public:
  virtual void update(void) = 0;

public:
  virtual float          particle_radius(void) const              = 0;
  virtual size_t         num_fluid_particle(void) const           = 0;
  virtual const Vector3* fluid_particle_position_data(void) const = 0;
  virtual const float*   fluid_particle_density_data(void) const  = 0;
};

} // namespace ms