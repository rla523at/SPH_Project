#pragma once
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
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
  void update_density(void);
  void update_force(void);

  float kernel(const float q) const;
  float dkernel_dq(const float q) const;

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
  static constexpr float _radius    = 1.0f / 32.0f;
  static constexpr float _viscosity = 0.1f;
};

} // namespace ms
