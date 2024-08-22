#pragma once
#include "Abbreviation.h"
#include "Buffer_Set.h"

namespace ms
{
class SPH_Scheme
{
public:
  virtual void update(void) = 0;

public:
  virtual float             particle_radius(void) const              = 0;
  virtual size_t            num_fluid_particle(void) const           = 0;
  virtual const Read_Write_Buffer_Set& get_fluid_v_pos_RWBS(void) const           = 0;
  virtual const Read_Write_Buffer_Set& get_fluid_density_RWBS(void) const         = 0;
};

} // namespace ms