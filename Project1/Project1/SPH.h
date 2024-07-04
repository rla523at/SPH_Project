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
};

class SPH
{
public:
  SPH(const ComPtr<ID3D11Device> cptr_device);

public:
  void update(const ComPtr<ID3D11DeviceContext> cptr_context);
  void render(const ComPtr<ID3D11DeviceContext> cptr_context);

private:
  void update_density_pressure(void);
  void update_force(void);

  float fk(const float q) const;
  float dfk_dq(const float q) const;

private:
  std::vector<Particle> _particles;

  ComPtr<ID3D11InputLayout>    _cptr_input_layout;
  ComPtr<ID3D11VertexShader>   _cptr_vertex_shader;
  ComPtr<ID3D11PixelShader>    _cptr_pixel_shader;
  ComPtr<ID3D11GeometryShader> _cptr_geometry_shader;

  ComPtr<ID3D11ShaderResourceView> _cptr_VS_SRV;    //shader resource view using at vertex shader
  ComPtr<ID3D11Buffer>             _cptr_VS_SRB;    //shader resource buffer using at vertex shader
  ComPtr<ID3D11Buffer>             _cptr_SB_VS_SRB; //staging buffer for VS_SRB

  //temporary
  static constexpr size_t _num_particle = 10;
  static constexpr float  _k            = 1.0e6f; //pressure coefficient
  static constexpr float  _dt           = 1.0e-4f;
  static constexpr float  _r            = 1.0f / 50.0f;
  static constexpr float  _volume       = std::numbers::pi_v<float> * _r * _r;
  static constexpr float  _density      = 1000;
  static constexpr float  _mass         = _volume * _density;
  static constexpr float  _h            = 2.0f*_r; //smoothing length
  static constexpr float  _viscosity    = 1.0e-6f;
  static constexpr float  _kernel_coeff = 1.0f / (_r * _r);
};

} // namespace ms
