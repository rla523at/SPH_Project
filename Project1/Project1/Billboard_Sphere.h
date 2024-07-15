#pragma once

#include "Mesh.h"

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <vector>
#include <wrl.h> // ComPtr

//Forward Declaration
namespace ms
{
class Camera;
}

// Abbreviation
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;
using Microsoft::WRL::ComPtr;

// Data Struct
namespace ms
{
struct Billboard_Sphere_Vertex_Data
{
  Vector3 pos;
};

struct Billboard_Sphere_GS_Cbuffer_Data
{
  float   radius;
  Vector3 v_cam_pos;
  Vector3 v_cam_up;
  float   padding;
  Matrix  m_view;
  Matrix  m_proj;
};
} // namespace ms

namespace ms
{

class Billboard_Sphere : public Mesh
{
public:
  Billboard_Sphere(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context);

public:
  void update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context) override;
  void render(const ComPtr<ID3D11DeviceContext> cptr_context) override;
  void register_GUI_component(GUI_Manager& GUI_manager) override{};
  void set_model_matrix(const Matrix& m_model) override{};

private:
  void init_Vbuffer(const ComPtr<ID3D11Device> cptr_device, const Billboard_Sphere_Vertex_Data& vertex_data);
  void init_VS_and_input_layout(const ComPtr<ID3D11Device> cptr_device);
  void init_GS_Cbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_GS(const ComPtr<ID3D11Device> cptr_device);
  void init_PS(const ComPtr<ID3D11Device> cptr_device);

  void set_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context);
  void reset_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context);

  void update_GS_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context);

private:
  ComPtr<ID3D11Buffer>         _cptr_Vbuffer;
  ComPtr<ID3D11InputLayout>    _cptr_input_layout;
  ComPtr<ID3D11VertexShader>   _cptr_VS; // vertex shader
  ComPtr<ID3D11Buffer>         _cptr_GS_Cbuffer;
  ComPtr<ID3D11GeometryShader> _cptr_GS; // Geometry shader
  ComPtr<ID3D11PixelShader>    _cptr_PS; // pixel shader

  Billboard_Sphere_GS_Cbuffer_Data _GS_Cbuffer_data;
};

} // namespace ms
