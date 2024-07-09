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
  void init_VS_SRbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_SRview(const ComPtr<ID3D11Device> cptr_device);

  void create_particle(void);

  void mass_2011_cornell(void);
  void mass_1994_monaghan(void);

  void update_density_with_clamp(void);
  void update_density(void);
  void update_pressure(void);
  void update_force(void);

private:
  float fk(const float q) const;
  float dfk_dq(const float q) const;

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
  static constexpr float _total_volume = 1.0f;
  static constexpr float _k            = 4.0e6f; //pressure coefficient
  static constexpr float _dt           = 1.0e-3f;
  static constexpr float _rest_density = 1000;
  static constexpr float _viscosity    = 8.0e-2f;

  size_t _num_particle   = 0;
  float  _mass           = 0.0f;
  float  _h              = 0.0f; //smoothing length
  float  _support_length = 0.0f;
  float  _kernel_coeff   = 0.0f;
};

} // namespace ms
