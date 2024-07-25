#pragma once
#include "Mesh.h"

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <numbers>
#include <vector>
#include <wrl.h> // ComPtr
#include <memory>

//Forward Declaration
namespace ms
{
class Camera;
class WCSPH;
} // namespace ms

// Abbreviation
using DirectX::SimpleMath::Vector3;
using Microsoft::WRL::ComPtr;

// Data Struct
namespace ms
{
struct SPH_GS_Cbuffer_Data
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

class SPH : public Mesh
{
public:
  SPH(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context);
  ~SPH(void);

public:
  void update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context) override;
  void render(const ComPtr<ID3D11DeviceContext> cptr_context) override;
  void register_GUI_component(GUI_Manager& GUI_manager) override{};
  void set_model_matrix(const Matrix& m_model) override{};

private:
  void init_boundary_Vbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_boundary_VS(const ComPtr<ID3D11Device> cptr_device);
  void init_boundary_PS(const ComPtr<ID3D11Device> cptr_device);
  void init_boundary_blender_state(const ComPtr<ID3D11Device> cptr_device);

  void init_VS_SRbuffer_pos(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_Sbuffer_pos(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_SRview_pos(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_SRbuffer_density(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_Sbuffer_density(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_SRview_density(const ComPtr<ID3D11Device> cptr_device);
  void init_VS(const ComPtr<ID3D11Device> cptr_device);
  void init_GS_Cbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_GS(const ComPtr<ID3D11Device> cptr_device);
  void init_PS(const ComPtr<ID3D11Device> cptr_device);

  void update_Vbuffer(const ComPtr<ID3D11DeviceContext> cptr_context);
  void update_VS_SRview(const ComPtr<ID3D11DeviceContext> cptr_context);
  void update_GS_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context);

  void set_fluid_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context);
  void set_boundary_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context);
  void reset_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context);

private:
  //common
  std::unique_ptr<WCSPH> _uptr_particles;
  SPH_GS_Cbuffer_Data              _GS_Cbuffer_data;
  ComPtr<ID3D11Buffer>             _cptr_GS_Cbuffer;
  ComPtr<ID3D11GeometryShader>     _cptr_GS;

  //fluid element
  ComPtr<ID3D11ShaderResourceView> _cptr_VS_SRview_pos;   
  ComPtr<ID3D11Buffer>             _cptr_VS_SRbuffer_pos; 
  ComPtr<ID3D11Buffer>             _cptr_VS_Sbuffer_pos;  

  ComPtr<ID3D11ShaderResourceView> _cptr_VS_SRview_density;
  ComPtr<ID3D11Buffer>             _cptr_VS_SRbuffer_density;
  ComPtr<ID3D11Buffer>             _cptr_VS_Sbuffer_density;  

  ComPtr<ID3D11VertexShader>       _cptr_VS;
  ComPtr<ID3D11PixelShader>        _cptr_PS;

  //boudary element
  ComPtr<ID3D11InputLayout>  _cptr_boundary_input_layout;
  ComPtr<ID3D11Buffer>       _cptr_boundary_Vbuffer;
  ComPtr<ID3D11VertexShader> _cptr_boundary_VS;
  ComPtr<ID3D11PixelShader>  _cptr_boundary_PS;
  ComPtr<ID3D11BlendState>   _cptr_boundary_blend_state;
};

} // namespace ms
