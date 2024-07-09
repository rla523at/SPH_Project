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
}

namespace ms
{
class Fluid_Particles;
}

namespace ms
{

class SPH
{
public:
  SPH(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context);
  ~SPH(void);

public:
  void update(const ComPtr<ID3D11DeviceContext> cptr_context);
  void render(const ComPtr<ID3D11DeviceContext> cptr_context);

private:
  void init_VS_SRbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_SRview(const ComPtr<ID3D11Device> cptr_device);
  void init_VS(const ComPtr<ID3D11Device> cptr_device);
  void init_GS(const ComPtr<ID3D11Device> cptr_device);
  void init_PS(const ComPtr<ID3D11Device> cptr_device);
  void update_VS_SRview(const ComPtr<ID3D11DeviceContext> cptr_context);

private:
  std::unique_ptr<Fluid_Particles> _uptr_particles;

  ComPtr<ID3D11InputLayout>    _cptr_input_layout;
  ComPtr<ID3D11VertexShader>   _cptr_vertex_shader;
  ComPtr<ID3D11PixelShader>    _cptr_pixel_shader;
  ComPtr<ID3D11GeometryShader> _cptr_geometry_shader;

  ComPtr<ID3D11ShaderResourceView> _cptr_VS_SRview;   //shader resource view using at vertex shader
  ComPtr<ID3D11Buffer>             _cptr_VS_SRbuffer; //shader resource buffer using at vertex shader
  ComPtr<ID3D11Buffer>             _cptr_SB_VS_SRB;   //staging buffer for VS_SRB
};

} // namespace ms
