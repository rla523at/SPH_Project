#include "SPH.h"

#include "Camera.h"
#include "Neighborhood_Brute_Force.h"
#include "Neighborhood_Uniform_Grid.h"
#include "Particles.h"

#include "../_lib/_header/msexception/Exception.h"
#include <d3dcompiler.h>
#include <random>

namespace ms
{

SPH::SPH(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  Material_Property mat_prop;
  mat_prop.rest_density         = 1.0e3f;
  mat_prop.pressure_coefficient = 5.0e5f;
  mat_prop.viscosity            = 5.0e-3f;

  Initial_Condition_Cube init_cond;
  init_cond.init_pos             = {-0.9f, 0.0f, 0.0f};
  init_cond.edge_length          = 1.0f;
  init_cond.num_particle_in_edge = 30;

  Domain domain;
  domain.x_start = -1.0f;
  domain.x_end   = 3.0f;
  domain.y_start = 0.0f;
  domain.y_end   = 3.0f;
  domain.z_start = 0.0f;
  domain.z_end   = 1.1f;

  _uptr_particles = std::make_unique<Fluid_Particles>(mat_prop, init_cond, domain);

  const float divide_length = _uptr_particles->support_length();
  const auto& pos_vecetors  = _uptr_particles->get_position_vectors();

  _uptr_neighborhood = std::make_unique<Neighborhood_Uniform_Grid>(domain, divide_length, pos_vecetors);

  //const auto num_particles = _uptr_particles->num_particle();
  //_uptr_neighborhood = std::make_unique<Neighborhood_Brute_Force>(num_particles);

  _GS_Cbuffer_data.radius = _uptr_particles->support_length() / 2;

  //cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

  //this->init_Vbuffer(cptr_device);

  this->init_VS_SRbuffer(cptr_device);
  this->init_VS_SRview(cptr_device);
  this->init_VS(cptr_device);
  this->init_GS_Cbuffer(cptr_device);
  this->init_GS(cptr_device);
  this->init_PS(cptr_device);

  //init Vertex Shader Staging Buffer
  {
    const UINT data_size    = sizeof(Vector3);
    const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

    D3D11_BUFFER_DESC buffer_desc   = {};
    buffer_desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
    buffer_desc.Usage               = D3D11_USAGE_STAGING;
    buffer_desc.BindFlags           = NULL;
    buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    buffer_desc.MiscFlags           = NULL;
    buffer_desc.StructureByteStride = NULL;

    D3D11_SUBRESOURCE_DATA buffer_data = {};
    buffer_data.pSysMem                = _uptr_particles->position_data();
    buffer_data.SysMemPitch            = 0;
    buffer_data.SysMemSlicePitch       = 0;

    const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_VS_Sbuffer.GetAddressOf());
    REQUIRE(!FAILED(result), "staging buffer creation should succeed");
  }
}

SPH::~SPH(void) = default;

void SPH::update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  //const auto& pos_vectors = _uptr_particles->get_position_vectors();
  //_uptr_neighborhood->update(pos_vectors);
  //_uptr_particles->update(*_uptr_neighborhood);
  //this->update_Vbuffer(cptr_context);

  //_GS_Cbuffer_data.v_cam_pos = camera.position_vector();
  //_GS_Cbuffer_data.v_cam_up  = camera.up_vector();
  //_GS_Cbuffer_data.m_view    = camera.view_matrix();
  //_GS_Cbuffer_data.m_proj    = camera.proj_matrix();
  //this->update_GS_Cbuffer(cptr_context);

  const auto& pos_vectors = _uptr_particles->get_position_vectors();
  _uptr_neighborhood->update(pos_vectors);
  _uptr_particles->update(*_uptr_neighborhood);
  this->update_VS_SRview(cptr_context);

  _GS_Cbuffer_data.v_cam_pos = camera.position_vector();
  _GS_Cbuffer_data.v_cam_up  = camera.up_vector();
  _GS_Cbuffer_data.m_view    = camera.view_matrix();
  _GS_Cbuffer_data.m_proj    = camera.proj_matrix();
  this->update_GS_Cbuffer(cptr_context);
}

void SPH::render(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  this->set_graphics_pipeline(cptr_context);

  const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

  cptr_context->Draw(num_particle, 0);

  this->reset_graphics_pipeline(cptr_context);
}

void SPH::init_VS(const ComPtr<ID3D11Device> cptr_device)
{
//  constexpr auto* file_name                   = L"SPH_VS.hlsl";
//  constexpr auto* entry_point_name            = "main";
//  constexpr auto* sharder_target_profile_name = "vs_5_0";
//
//  D3D11_INPUT_ELEMENT_DESC pos_desc = {};
//  pos_desc.SemanticName             = "POSITION";
//  pos_desc.SemanticIndex            = 0;
//  pos_desc.Format                   = DXGI_FORMAT_R32G32B32_FLOAT;
//  pos_desc.InputSlot                = 0;
//  pos_desc.AlignedByteOffset        = 0;
//  pos_desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
//  pos_desc.InstanceDataStepRate     = 0;
//
//  constexpr UINT           num_input                      = 1;
//  D3D11_INPUT_ELEMENT_DESC input_element_descs[num_input] = {pos_desc};
//
//#if defined(_DEBUG)
//  constexpr UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
//#else
//  constexpr UINT compile_flags = NULL;
//#endif
//
//  ComPtr<ID3DBlob> shader_blob;
//  ComPtr<ID3DBlob> error_blob;
//
//  {
//    const auto result = D3DCompileFromFile(
//      file_name,
//      nullptr,
//      nullptr,
//      entry_point_name,
//      sharder_target_profile_name,
//      compile_flags,
//      NULL,
//      &shader_blob,
//      &error_blob);
//
//    REQUIRE(!FAILED(result), "SPH vertex shader compiling should succeed");
//  }
//
//  {
//    const auto result = cptr_device->CreateInputLayout(
//      input_element_descs,
//      num_input,
//      shader_blob->GetBufferPointer(),
//      shader_blob->GetBufferSize(),
//      _cptr_input_layout.GetAddressOf());
//
//    REQUIRE(!FAILED(result), "SPH input layout creation should succeed");
//  }
//
//  {
//    const auto result = cptr_device->CreateVertexShader(
//      shader_blob->GetBufferPointer(),
//      shader_blob->GetBufferSize(),
//      NULL,
//      _cptr_VS.GetAddressOf());
//
//    REQUIRE(!FAILED(result), "SPH vertex shader creation should succeed");
//  }

    constexpr auto* file_name                   = L"SPH_VS.hlsl";
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
  
      REQUIRE(!FAILED(result), "vertex shader creation should succeed");
    }
  
    {
      const auto result = cptr_device->CreateVertexShader(
        shader_blob->GetBufferPointer(),
        shader_blob->GetBufferSize(),
        NULL,
        _cptr_VS.GetAddressOf());
  
      REQUIRE(!FAILED(result), "ID3D11VertexShader creation should succeed");
    }
}

void SPH::init_GS_Cbuffer(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto data_size = sizeof(SPH_GS_Cbuffer_Data);

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(data_size);
  desc.Usage               = D3D11_USAGE_DYNAMIC;
  desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags           = NULL;
  desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = &_GS_Cbuffer_data;
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&desc, &initial_data, _cptr_GS_Cbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH geometry shader constant buffer creation should succeed");
}

void SPH::init_GS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"SPH_GS.hlsl";
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

    REQUIRE(!FAILED(result), "SPH geometry shader compiling should succceed");
  }

  {
    const auto result = cptr_device->CreateGeometryShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_GS.GetAddressOf());

    REQUIRE(!FAILED(result), "SPH geometry shader creation should succeed");
  }
}

void SPH::init_PS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"SPH_PS.hlsl";
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

    REQUIRE(!FAILED(result), "SPH pixel shader compiling should succceed");
  }

  {
    const auto result = cptr_device->CreatePixelShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_PS.GetAddressOf());

    REQUIRE(!FAILED(result), "SPH pixel shader creation should succeed");
  }
}

void SPH::update_Vbuffer(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  const auto  data_size    = sizeof(Vector3);
  const auto  num_particle = _uptr_particles->num_particle();
  const auto* data_ptr     = _uptr_particles->position_data();
  const auto  copy_size    = data_size * num_particle;

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_Vbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
  memcpy(ms.pData, data_ptr, copy_size);
  cptr_context->Unmap(_cptr_Vbuffer.Get(), NULL);
}

void SPH::update_VS_SRview(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  const UINT     data_size    = sizeof(Vector3);
  const UINT     num_particle = static_cast<UINT>(_uptr_particles->num_particle());
  const Vector3* data_ptr     = _uptr_particles->position_data();

  //Copy CPU to Staging Buffer
  const size_t copy_size = data_size * num_particle;

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_VS_Sbuffer.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
  memcpy(ms.pData, data_ptr, copy_size);
  cptr_context->Unmap(_cptr_VS_Sbuffer.Get(), NULL);

  // Copy Staging Buffer to Vertex Shader Resource Buffer
  cptr_context->CopyResource(_cptr_VS_SRbuffer.Get(), _cptr_VS_Sbuffer.Get());
}

void SPH::update_GS_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  constexpr auto data_size = sizeof(SPH_GS_Cbuffer_Data);

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_GS_Cbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
  memcpy(ms.pData, &_GS_Cbuffer_data, data_size);
  cptr_context->Unmap(_cptr_GS_Cbuffer.Get(), NULL);
}

void SPH::set_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  //UINT stride = sizeof(Vector3);
  //UINT offset = 0;
  //cptr_context->IASetVertexBuffers(0, 1, _cptr_Vbuffer.GetAddressOf(), &stride, &offset);
  //cptr_context->IASetInputLayout(_cptr_input_layout.Get());
  //cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

  //cptr_context->VSSetShader(_cptr_VS.Get(), nullptr, 0);

  //cptr_context->GSSetConstantBuffers(0, 1, _cptr_GS_Cbuffer.GetAddressOf());
  //cptr_context->GSSetShader(_cptr_GS.Get(), nullptr, 0);

  //cptr_context->PSSetShader(_cptr_PS.Get(), nullptr, 0);

  cptr_context->VSSetShaderResources(0, 1, _cptr_VS_SRview.GetAddressOf());
  cptr_context->VSSetShader(_cptr_VS.Get(), nullptr, 0);

  cptr_context->GSSetConstantBuffers(0, 1, _cptr_GS_Cbuffer.GetAddressOf());
  cptr_context->GSSetShader(_cptr_GS.Get(), nullptr, 0);

  cptr_context->PSSetShader(_cptr_PS.Get(), nullptr, 0);
}

void SPH::reset_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  //UINT stride = 0;
  //UINT offset = 0;
  //cptr_context->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);
  //cptr_context->IASetInputLayout(nullptr);
  //cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);

  //cptr_context->VSSetShaderResources(0, 0, nullptr);
  //cptr_context->VSSetShader(nullptr, nullptr, 0);

  //cptr_context->GSSetConstantBuffers(0, 0, nullptr);
  //cptr_context->GSSetShader(nullptr, nullptr, 0);

  //cptr_context->PSSetShader(nullptr, nullptr, 0);

  cptr_context->VSSetShaderResources(0, 0, nullptr);
  cptr_context->VSSetShader(nullptr, nullptr, 0);

  cptr_context->GSSetConstantBuffers(0, 0, nullptr);
  cptr_context->GSSetShader(nullptr, nullptr, 0);

  cptr_context->PSSetShader(nullptr, nullptr, 0);
}

void SPH::init_VS_SRbuffer(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT data_size    = sizeof(Vector3);
  const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  buffer_desc.Usage               = D3D11_USAGE_DEFAULT;
  buffer_desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  buffer_desc.CPUAccessFlags      = NULL;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA buffer_data = {};
  buffer_data.pSysMem                = _uptr_particles->position_data();
  buffer_data.SysMemPitch            = 0;
  buffer_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_VS_SRbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader resource buffer creation should succeed");
}

void SPH::init_VS_SRview(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

  D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc = {};
  SRV_desc.Format                          = DXGI_FORMAT_R32G32B32_FLOAT;
  SRV_desc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
  SRV_desc.BufferEx.NumElements            = num_particle;

  const auto result = cptr_device->CreateShaderResourceView(_cptr_VS_SRbuffer.Get(), &SRV_desc, _cptr_VS_SRview.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader resource buffer creation should succeed");
}

void SPH::init_Vbuffer(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT data_size    = sizeof(Vector3);
  const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  desc.Usage               = D3D11_USAGE_DYNAMIC;
  desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
  desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags           = NULL;
  desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = _uptr_particles->position_data();
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&desc, &initial_data, _cptr_Vbuffer.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex buffer creation should succeed");
}

} // namespace ms
