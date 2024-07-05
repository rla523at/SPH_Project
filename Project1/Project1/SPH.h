#pragma once
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <numbers>
#include <vector>
#include <wrl.h> // ComPtr

namespace ms
{

using DirectX::SimpleMath::Vector3;
using Microsoft::WRL::ComPtr;

struct Particle
{
  Vector3 position = Vector3(0.0f);
  Vector3 velocity = Vector3(0.0f);
  Vector3 force    = Vector3(0.0f);
  float   density  = 0.0f;
  float   pressure = 0.0f;
  BOOL    is_alive = FALSE;
};

class SPH
{
public:
  SPH(const ComPtr<ID3D11Device> cptr_device);

public:
  void update(const ComPtr<ID3D11DeviceContext> cptr_context);
  void render(const ComPtr<ID3D11DeviceContext> cptr_context);

private:
  //init Vertex Shader Resource Buffer
  void init_VS_SRbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_SRview(const ComPtr<ID3D11Device> cptr_device);

  void update_density_pressure(void);
  void update_force(void);

  float fk(const float q) const;
  float dfk_dq(const float q) const;

  void create_particle(void);

private:
  std::vector<Particle> _particles;

  ComPtr<ID3D11InputLayout>    _cptr_input_layout;
  ComPtr<ID3D11VertexShader>   _cptr_vertex_shader;
  ComPtr<ID3D11PixelShader>    _cptr_pixel_shader;
  ComPtr<ID3D11GeometryShader> _cptr_geometry_shader;

  ComPtr<ID3D11ShaderResourceView> _cptr_VS_SRview;   //shader resource view using at vertex shader
  ComPtr<ID3D11Buffer>             _cptr_VS_SRbuffer; //shader resource buffer using at vertex shader
  ComPtr<ID3D11Buffer>             _cptr_SB_VS_SRB;   //staging buffer for VS_SRB

  //temporary
  static constexpr size_t _num_particle           = 100;
  static constexpr size_t _num_creation_per_frame = 2;
  static constexpr float  _k                      = 1.0e6f; //pressure coefficient
  static constexpr float  _dt                     = 1.0e-3f;
  static constexpr float  _r                      = 1.0f / 100.0f;
  static constexpr float  _rest_density           = 1000;
  static constexpr float  _h                      = 1.0f * _r; //smoothing length
  static constexpr float  _support_length         = 2 * _h;
  static inline float     _mass                   = 0;
  static constexpr float  _mass_virtual           = _rest_density * std::numbers::pi_v<float> * _h * _h;
  static constexpr float  _viscosity              = 1.0e-3f;
  static constexpr float  _kernel_coeff           = 1.0f / (_h * _h);
};

} // namespace ms
