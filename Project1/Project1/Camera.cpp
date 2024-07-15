#include "Camera.h"

#include "GUI_Manager.h"
#include "Window_Manager.h"

#include <iostream>
#include <numbers>

namespace ms
{

Camera::Camera(const float aspect_ratio)
{
  constexpr float pi        = std::numbers::pi_v<float>;
  constexpr float fovAngleY = 70.0f * pi / 180.0f;
  constexpr float near_z    = 0.01f;
  constexpr float far_z     = 100.0f;

  if (_use_perspective_projection)
    _m_proj = DirectX::XMMatrixPerspectiveFovLH(fovAngleY, aspect_ratio, near_z, far_z);
  else
    _m_proj = DirectX::XMMatrixOrthographicOffCenterLH(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, near_z, far_z);
}

void Camera::register_GUI_component(GUI_Manager& GUI_manager)
{
  const float pi = std::numbers::pi_v<float>;

  GUI_manager.register_GUI_component("use perspective projection", &_use_perspective_projection);
}

void Camera::update(const float dt)
{
  constexpr Vector3 v_origin_view  = {0.0f, 0.0f, 1.0f};
  constexpr Vector3 v_origin_right = {1.0f, 0.0f, 0.0f};
  constexpr Vector3 v_origin_up    = {0.0f, 1.0f, 0.0f};

  constexpr float pi = std::numbers::pi_v<float>;

  //_yaw   = Window_Manager::mouse_x_pos_NDC() * pi;
  //_pitch = Window_Manager::mouse_y_pos_NDC() * pi / 2;

  const auto m_rot_yaw   = Matrix::CreateRotationY(_yaw);
  const auto m_rot_pitch = Matrix::CreateRotationX(-_pitch);

  const auto v_view  = Vector3::Transform(v_origin_view, m_rot_yaw * m_rot_pitch);
  const auto v_up    = Vector3::Transform(v_origin_up, m_rot_pitch);
  const auto v_right = Vector3::Transform(v_origin_right, m_rot_yaw);

  Vector3 v_forward = {v_view.x, 0.0f, v_view.z};
  v_forward.Normalize();

  if (Window_Manager::is_Wkey_pressed())
    _v_pos += v_forward * dt;
  if (Window_Manager::is_Skey_pressed())
    _v_pos -= v_forward * dt;
  if (Window_Manager::is_Akey_pressed())
    _v_pos -= v_right * dt;
  if (Window_Manager::is_Dkey_pressed())
    _v_pos += v_right * dt;

  _m_view = XMMatrixLookToLH(_v_pos, v_view, v_up);
}

Matrix Camera::view_matrix(void) const
{
  return _m_view;
}

Matrix Camera::proj_matrix(void) const
{
  return _m_proj;
}

Vector3 Camera::position_vector(void) const
{
  return _v_pos;
}

Vector3 Camera::up_vector(void) const
{
  return {_m_view(1, 0), _m_view(1, 1), _m_view(1, 2)};
}

} // namespace ms