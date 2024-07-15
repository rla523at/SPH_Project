#include "Camera.h"

#include "GUI_Manager.h"
#include "Window_Manager.h"

#include <DirectXMath.h>
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
  constexpr float moving_speed = 10.0f;

  float pitch_diff = 0.0f;
  float yaw_diff   = 0.0f;

  if (Window_Manager::is_Up_key_pressed())
    pitch_diff += dt;
  if (Window_Manager::is_Down_key_pressed())
    pitch_diff -= dt;
  if (Window_Manager::is_Right_key_pressed())
    yaw_diff += dt;
  if (Window_Manager::is_Left_key_pressed())
    yaw_diff -= dt;

  //v_right는 항상 Global XZ Plane에 둔다.
  //= rotation은 항상 Global Y축에 대해서 진행한다.
  //= rotation 후 v_up과 v_right는 더이상 orthognal 하지 않는다.
  const auto v_cur_right = this->right_vector();
  const auto m_rot_yaw   = Matrix::CreateRotationY(yaw_diff);
  const auto v_right     = Vector3::Transform(v_cur_right, m_rot_yaw);

  const auto v_cur_up    = this->up_vector();
  const auto m_rot_pitch = DirectX::XMMatrixRotationAxis(v_right, -pitch_diff);
  auto       v_up        = Vector3::Transform(v_cur_up, m_rot_pitch);

  auto v_view = v_right.Cross(v_up);
  v_view.Normalize();

  // v_up을 v_right와 orthogonal 하게 바꾼다.
  v_up = v_view.Cross(v_right);
  v_up.Normalize();

  if (Window_Manager::is_Wkey_pressed())
    _v_pos += moving_speed * v_view * dt;
  if (Window_Manager::is_Skey_pressed())
    _v_pos -= moving_speed * v_view * dt;
  if (Window_Manager::is_Akey_pressed())
    _v_pos -= moving_speed * v_right * dt;
  if (Window_Manager::is_Dkey_pressed())
    _v_pos += moving_speed * v_right * dt;

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
  return {_m_view(0, 1), _m_view(1, 1), _m_view(2, 1)};
}

Vector3 Camera::view_vector(void) const
{
  return {_m_view(0, 2), _m_view(1, 2), _m_view(2, 2)};
}

Vector3 Camera::right_vector(void) const
{
  return {_m_view(0, 0), _m_view(1, 0), _m_view(2, 0)};
}

} // namespace ms