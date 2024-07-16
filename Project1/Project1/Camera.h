#pragma once
#include <directxtk/SimpleMath.h>

namespace ms
{
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;
} // namespace ms

namespace ms
{
class GUI_Manager;
}

namespace ms
{

class Camera
{
public:
  Camera(const float aspect_ratio);

public:
  void register_GUI_component(GUI_Manager& GUI_manager);
  void update(const float dt);

public:
  Matrix  view_matrix(void) const;
  Matrix  proj_matrix(void) const;
  Vector3 position_vector(void) const;
  Vector3 up_vector(void) const;

private:
  Vector3 view_vector(void) const;
  Vector3 right_vector(void) const;

private:
  Vector3 _v_pos = {0.0f, 3.0f, -5.0f};
  Matrix  _m_view;
  Matrix  _m_proj;

  // GUI Component
  bool _use_perspective_projection = true;
};

} // namespace ms