#include "Square.h"

#include "Camera.h"
#include "GUI_Manager.h"
#include "Texture_Manager.h"

#include "../_lib/_header/msexception/Exception.h"
#include <DirectXTex.h>
#include <d3dcompiler.h>
#include <tuple>
#include <vector>

namespace ms
{

Square::Square(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context, const wchar_t* texture_file_name)
{
  const auto vertex_datas = this->make_vertex_data();
  const auto index_datas  = this->make_index_data();

  this->init_Vbuffer(cptr_device, vertex_datas);
  this->init_Ibuffer(cptr_device, index_datas);
  this->init_VS_Cbuffer(cptr_device);
  this->init_VS_and_input_layout(cptr_device);

  this->init_texture(cptr_device, texture_file_name);
  this->init_texture_sampler_state(cptr_device);

  this->init_square_PS(cptr_device);
}

void Square::update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  _VS_Cbuffer_data.view       = camera.view_matrix();
  _VS_Cbuffer_data.projection = camera.proj_matrix();
  this->update_square_Cbuffer(cptr_context);
}

void Square::render(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  this->set_graphics_pipe_line(cptr_context);
  cptr_context->DrawIndexed(_num_index, 0, 0);
}

void Square::set_model_matrix(const Matrix& m_model)
{
  _VS_Cbuffer_data.model = m_model;
}

std::vector<Square_Vertex_Data> Square::make_vertex_data() const
{
  constexpr size_t num_vetex = 4;

  std::vector<Square_Vertex_Data> vertex_datas(num_vetex);
  vertex_datas[0].pos = {-1.0f, -1.0f, -0.0f};
  vertex_datas[1].pos = {-1.0f, 1.0f, -0.0f};
  vertex_datas[2].pos = {1.0f, 1.0f, -0.0f};
  vertex_datas[3].pos = {1.0f, -1.0f, -0.0f};

  vertex_datas[0].tex = {0.0f, 1.0f};
  vertex_datas[1].tex = {0.0f, 0.0f};
  vertex_datas[2].tex = {1.0f, 0.0f};
  vertex_datas[3].tex = {1.0f, 1.0f};

  return vertex_datas;
}

std::vector<uint16_t> Square::make_index_data(void) const
{
  // see Square::make_vertex_data
  // 1¦¡2
  // ¦¢ ¦¢
  // 0¦¡3

  return {0, 1, 2, 0, 2, 3};
}

void Square::init_Vbuffer(const ComPtr<ID3D11Device> cptr_device, const std::vector<Square_Vertex_Data>& vertices)
{
  constexpr int data_size = sizeof(Square_Vertex_Data);

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(data_size * vertices.size());
  buffer_desc.Usage               = D3D11_USAGE_IMMUTABLE;
  buffer_desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags      = NULL;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA buffer_data = {};
  buffer_data.pSysMem                = vertices.data();
  buffer_data.SysMemPitch            = 0;
  buffer_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_Vbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "box vertex buffer creation should succeed");
}

void Square::init_Ibuffer(const ComPtr<ID3D11Device> cptr_device, const std::vector<uint16_t>& indices)
{
  _num_index = static_cast<UINT>(indices.size());

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(sizeof(uint16_t) * indices.size());
  buffer_desc.Usage               = D3D11_USAGE_IMMUTABLE;
  buffer_desc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
  buffer_desc.CPUAccessFlags      = NULL;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA buffer_data = {};
  buffer_data.pSysMem                = indices.data();
  buffer_data.SysMemPitch            = 0;
  buffer_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_Ibuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "box index buffer creation should succeed");
}

void Square::init_VS_Cbuffer(const ComPtr<ID3D11Device> cptr_device)
{
  D3D11_BUFFER_DESC cbDesc   = {};
  cbDesc.ByteWidth           = sizeof(Square_VS_CBuffer_Data);
  cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
  cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
  cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  cbDesc.MiscFlags           = NULL;
  cbDesc.StructureByteStride = 0;

  D3D11_SUBRESOURCE_DATA buffer_data = {};
  buffer_data.pSysMem                = &_VS_Cbuffer_data;
  buffer_data.SysMemPitch            = 0;
  buffer_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&cbDesc, &buffer_data, _cptr_VS_Cbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "box constant buffer creation should succeed");
}

void Square::init_VS_and_input_layout(const ComPtr<ID3D11Device> cptr_device)
{
  D3D11_INPUT_ELEMENT_DESC pos_desc = {};
  pos_desc.SemanticName             = "POSITION";
  pos_desc.SemanticIndex            = 0;
  pos_desc.Format                   = DXGI_FORMAT_R32G32B32_FLOAT;
  pos_desc.InputSlot                = 0;
  pos_desc.AlignedByteOffset        = 0;
  pos_desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
  pos_desc.InstanceDataStepRate     = 0;
  // pos_desc.AlignedByteOffset        = 0;

  D3D11_INPUT_ELEMENT_DESC tex_desc = {};
  tex_desc.SemanticName             = "TEXCOORD";
  tex_desc.SemanticIndex            = 0;
  tex_desc.Format                   = DXGI_FORMAT_R32G32_FLOAT;
  tex_desc.InputSlot                = 0;
  tex_desc.AlignedByteOffset        = D3D11_APPEND_ALIGNED_ELEMENT;
  tex_desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
  tex_desc.InstanceDataStepRate     = 0;

  std::vector<D3D11_INPUT_ELEMENT_DESC> input_element_descs = {pos_desc, tex_desc};

  constexpr auto* file_name                   = L"Square_VS.hlsl";
  constexpr auto* entry_point_name            = "main";
  constexpr auto* sharder_target_profile_name = "vs_5_0";

#if defined(_DEBUG)
  constexpr UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  constexpr UINT compile_flags = NULL;
#endif

  ComPtr<ID3DBlob> shader_blob;
  ComPtr<ID3DBlob> error_blob;

  {
    const auto result = D3DCompileFromFile(
      file_name,
      nullptr,
      nullptr,
      entry_point_name,
      sharder_target_profile_name,
      compile_flags,
      NULL,
      &shader_blob,
      &error_blob);

    REQUIRE(!FAILED(result), "box pixel shader creation should succceed");
  }

  {
    const auto result = cptr_device->CreateInputLayout(
      input_element_descs.data(),
      UINT(input_element_descs.size()),
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      _cptr_input_layout.GetAddressOf());

    REQUIRE(!FAILED(result), "box input layout creation should succeed");
  }

  {
    const auto result = cptr_device->CreateVertexShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_VS.GetAddressOf());

    REQUIRE(!FAILED(result), "box vertex shader creation should succeed");
  }
}

void Square::init_square_PS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"Square_PS.hlsl";
  constexpr auto* entry_point_name            = "main";
  constexpr auto* sharder_target_profile_name = "ps_5_0";

#if defined(_DEBUG)
  constexpr UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  constexpr UINT compile_flags = NULL;
#endif

  ComPtr<ID3DBlob> shader_blob;
  ComPtr<ID3DBlob> error_blob;

  {
    const auto result = D3DCompileFromFile(
      file_name,
      nullptr,
      nullptr,
      entry_point_name,
      sharder_target_profile_name,
      compile_flags,
      NULL,
      &shader_blob,
      &error_blob);

    REQUIRE(!FAILED(result), "pixel shader creation should succceed");
  }

  {
    const auto result = cptr_device->CreatePixelShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_PS.GetAddressOf());

    REQUIRE(!FAILED(result), "ID3D11PixelShader creation should succeed");
  }
}

void Square::init_texture(const ComPtr<ID3D11Device> cptr_device, const wchar_t* texture_file_name)
{
  Texture_Manager::make_immutable_texture2D(cptr_device, _cptr_texture.GetAddressOf(), texture_file_name);

  cptr_device->CreateShaderResourceView(_cptr_texture.Get(), nullptr, _cptr_texture_RView.GetAddressOf());
}

void Square::init_texture_sampler_state(const ComPtr<ID3D11Device> cptr_device)
{
  D3D11_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampler_desc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler_desc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler_desc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler_desc.MipLODBias         = 0.0f;
  sampler_desc.MaxAnisotropy      = 1;
  sampler_desc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
  sampler_desc.BorderColor[0]     = 0.0f;
  sampler_desc.BorderColor[1]     = 0.0f;
  sampler_desc.BorderColor[2]     = 0.0f;
  sampler_desc.BorderColor[3]     = 0.0f;
  sampler_desc.MinLOD             = 0;
  sampler_desc.MaxLOD             = D3D11_FLOAT32_MAX;

  // Create the Sample State
  cptr_device->CreateSamplerState(&sampler_desc, _cptr_texture_sample_state.GetAddressOf());
}

void Square::update_square_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_VS_Cbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
  memcpy(ms.pData, &_VS_Cbuffer_data, sizeof(_VS_Cbuffer_data));
  cptr_context->Unmap(_cptr_VS_Cbuffer.Get(), NULL);
}

void Square::set_graphics_pipe_line(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  UINT stride = sizeof(Square_Vertex_Data);
  UINT offset = 0;
  cptr_context->IASetVertexBuffers(0, 1, _cptr_Vbuffer.GetAddressOf(), &stride, &offset);
  cptr_context->IASetIndexBuffer(_cptr_Ibuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
  cptr_context->IASetInputLayout(_cptr_input_layout.Get());
  cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  cptr_context->VSSetConstantBuffers(0, 1, _cptr_VS_Cbuffer.GetAddressOf());
  cptr_context->VSSetShader(_cptr_VS.Get(), 0, 0);

  cptr_context->PSSetShaderResources(0, 1, _cptr_texture_RView.GetAddressOf());
  cptr_context->PSSetSamplers(0, 1, _cptr_texture_sample_state.GetAddressOf());
  cptr_context->PSSetShader(_cptr_PS.Get(), 0, 0);
}

} // namespace ms
