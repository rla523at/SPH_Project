#pragma once
#include "Mesh.h"

#include <directxtk/SimpleMath.h>
#include <vector>

namespace ms
{
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
} // namespace ms

namespace ms
{
struct Square_Vertex_Data
{
  Vector3 pos;
  Vector2 tex;
};

struct Square_VS_CBuffer_Data
{
  Matrix model;
  Matrix view;
  Matrix projection;
};

} // namespace ms

namespace ms
{

class Square : public Mesh
{
public:
  Square(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context, const wchar_t* texture_file_name);

public:
  void update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context) override;
  void render(const ComPtr<ID3D11DeviceContext> cptr_context) override;
  void register_GUI_component(GUI_Manager& GUI_manager) override{};
  void set_model_matrix(const Matrix& m_model) override;

private:
  std::vector<Square_Vertex_Data> make_vertex_data(void) const;
  std::vector<uint16_t>           make_index_data(void) const;

  void init_Vbuffer(const ComPtr<ID3D11Device> cptr_device, const std::vector<Square_Vertex_Data>& box_vertex_datas);
  void init_Ibuffer(const ComPtr<ID3D11Device> cptr_device, const std::vector<uint16_t>& indices);
  void init_VS_Cbuffer(const ComPtr<ID3D11Device> cptr_device);
  void init_VS_and_input_layout(const ComPtr<ID3D11Device> cptr_device);
  void init_square_PS(const ComPtr<ID3D11Device> cptr_device);
  void init_texture(const ComPtr<ID3D11Device> cptr_device, const wchar_t* texture_file_name);
  void init_texture_sampler_state(const ComPtr<ID3D11Device> cptr_device);

  void update_square_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context);

  void set_graphics_pipe_line(const ComPtr<ID3D11DeviceContext> cptr_context);
  void reset_graphics_pipe_line(const ComPtr<ID3D11DeviceContext> cptr_context);

protected:
  ComPtr<ID3D11Buffer>       _cptr_Vbuffer;
  ComPtr<ID3D11Buffer>       _cptr_Ibuffer;
  ComPtr<ID3D11Buffer>       _cptr_VS_Cbuffer;
  ComPtr<ID3D11InputLayout>  _cptr_input_layout;
  ComPtr<ID3D11VertexShader> _cptr_VS; // vertex shader
  ComPtr<ID3D11PixelShader>  _cptr_PS; // pixel shader

  UINT                   _num_index;
  Square_VS_CBuffer_Data _VS_Cbuffer_data;

  // Texturing
  ComPtr<ID3D11Texture2D>          _cptr_texture;
  ComPtr<ID3D11ShaderResourceView> _cptr_texture_RView;
  ComPtr<ID3D11SamplerState>       _cptr_texture_sample_state;
};

} // namespace ms