#include "Billboard_Sphere.h"

#include "Camera.h"

#include "../_lib/_header/msexception/Exception.h"
#include <d3dcompiler.h>

namespace ms
{

Billboard_Sphere::Billboard_Sphere(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  Billboard_Sphere_Vertex_Data vertex_data;
  vertex_data.pos = Vector3{0.0f, 1.0f, 0.0f};

  this->init_Vbuffer(cptr_device, vertex_data);
  this->init_VS_and_input_layout(cptr_device);
  this->init_GS_Cbuffer(cptr_device);
  this->init_GS(cptr_device);
  this->init_PS(cptr_device);

  _GS_Cbuffer_data.radius = 1.0f;
}

void Billboard_Sphere::update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  _GS_Cbuffer_data.v_cam_pos = camera.position_vector();
  _GS_Cbuffer_data.v_cam_up  = camera.up_vector();
  _GS_Cbuffer_data.m_view    = camera.view_matrix();
  _GS_Cbuffer_data.m_proj    = camera.proj_matrix();
  this->update_GS_Cbuffer(cptr_context);
}

void Billboard_Sphere::render(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  this->set_graphics_pipeline(cptr_context);
  cptr_context->Draw(1, 0);

  cptr_context->GSSetShader(nullptr, nullptr, NULL);
}

void Billboard_Sphere::init_VS_and_input_layout(const ComPtr<ID3D11Device> cptr_device)
{
  D3D11_INPUT_ELEMENT_DESC pos_desc = {};
  pos_desc.SemanticName             = "POSITION";
  pos_desc.SemanticIndex            = 0;
  pos_desc.Format                   = DXGI_FORMAT_R32G32B32_FLOAT;
  pos_desc.InputSlot                = 0;
  pos_desc.AlignedByteOffset        = 0;
  pos_desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
  pos_desc.InstanceDataStepRate     = 0;

  constexpr UINT           num_input_element                      = 1;
  D3D11_INPUT_ELEMENT_DESC input_element_descs[num_input_element] = {pos_desc};

  constexpr auto* shader_name                 = L"Billboard_Sphere_VS.hlsl";
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
      shader_name,
      nullptr,
      nullptr,
      entry_point_name,
      sharder_target_profile_name,
      compile_flags,
      NULL,
      &shader_blob,
      &error_blob);

    REQUIRE(!FAILED(result), "billboard sphere vertex shader compiling should succceed");
  }

  {
    const auto result = cptr_device->CreateVertexShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_VS.GetAddressOf());

    REQUIRE(!FAILED(result), "billboard sphere vertex shader creation should succeed");
  }

  {
    const auto result = cptr_device->CreateInputLayout(
      input_element_descs,
      num_input_element,
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      _cptr_input_layout.GetAddressOf());

    REQUIRE(!FAILED(result), "billboard sphere input layout creation should succeed");
  }
}

void Billboard_Sphere::init_GS_Cbuffer(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto data_size = sizeof(Billboard_Sphere_GS_Cbuffer_Data);

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(data_size);
  buffer_desc.Usage               = D3D11_USAGE_DYNAMIC;
  buffer_desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
  buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = &_GS_Cbuffer_data;
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &initial_data, _cptr_GS_Cbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "billbaord sphere geometry shader constant buffer creation should succeed");
}

void Billboard_Sphere::init_Vbuffer(const ComPtr<ID3D11Device> cptr_device, const Billboard_Sphere_Vertex_Data& vertex_data)
{
  constexpr auto data_size = sizeof(Billboard_Sphere_Vertex_Data);

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(data_size);
  buffer_desc.Usage               = D3D11_USAGE_IMMUTABLE;
  buffer_desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags      = NULL;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = &vertex_data;
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &initial_data, _cptr_Vbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "Billboard Sphere vertex buffer creation should succeed");
}

void Billboard_Sphere::init_GS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"Billboard_Sphere_GS.hlsl";
  constexpr auto* entry_point_name            = "main";
  constexpr auto* sharder_target_profile_name = "gs_5_0";

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

    REQUIRE(!FAILED(result), "billboard sphere geometry shader compiling should succceed");
  }

  {
    const auto result = cptr_device->CreateGeometryShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_GS.GetAddressOf());

    REQUIRE(!FAILED(result), "billboard sphere geometry shader creation should succeed");
  }
}

void Billboard_Sphere::init_PS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"Billboard_Sphere_PS.hlsl";
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

    REQUIRE(!FAILED(result), "billboard sphere pixel shader compiling should succceed");
  }

  {
    const auto result = cptr_device->CreatePixelShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_PS.GetAddressOf());

    REQUIRE(!FAILED(result), "billboard sphere pixel shader creation should succeed");
  }
}

void Billboard_Sphere::set_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  UINT stride = sizeof(Billboard_Sphere_Vertex_Data);
  UINT offset = 0;

  cptr_context->IASetVertexBuffers(0, 1, _cptr_Vbuffer.GetAddressOf(), &stride, &offset);
  cptr_context->IASetInputLayout(_cptr_input_layout.Get());
  cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

  cptr_context->VSSetShader(_cptr_VS.Get(), nullptr, 0);

  cptr_context->GSSetConstantBuffers(0, 1, _cptr_GS_Cbuffer.GetAddressOf());
  cptr_context->GSSetShader(_cptr_GS.Get(), nullptr, 0);

  cptr_context->PSSetShader(_cptr_PS.Get(), nullptr, 0);
}

void Billboard_Sphere::reset_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  UINT stride = 0;
  UINT offset = 0;

  cptr_context->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);
  cptr_context->IASetInputLayout(nullptr);

  cptr_context->VSSetShader(nullptr, nullptr, 0);

  cptr_context->GSSetConstantBuffers(0, 0, nullptr);
  cptr_context->GSSetShader(nullptr, nullptr, 0);

  cptr_context->PSSetShader(nullptr, nullptr, 0);
}

void Billboard_Sphere::update_GS_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  constexpr auto data_size = sizeof(Billboard_Sphere_GS_Cbuffer_Data);

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_GS_Cbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
  memcpy(ms.pData, &_GS_Cbuffer_data, data_size);
  cptr_context->Unmap(_cptr_GS_Cbuffer.Get(), NULL);
}

} // namespace ms