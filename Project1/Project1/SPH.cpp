#include "SPH.h"

#include "Particles.h"
#include "Neighborhood_Uniform_Grid.h"
#include "Neighborhood_Brute_Force.h"

#include "../_lib/_header/msexception/Exception.h"
#include <d3dcompiler.h>
#include <random>

namespace ms
{

SPH::SPH(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  Material_Property mat_prop;
  mat_prop.rest_density         = 1.0e3f;
  mat_prop.pressure_coefficient = 2.5e5f;
  mat_prop.viscosity            = 1.0e-2f;

  Initial_Condition_Cube init_cond;
  init_cond.init_pos             = {-1.0f, -1.0f, 0.0f};
  init_cond.edge_length          = 1.0f;
  init_cond.num_particle_in_edge = 10;

  _uptr_particles = std::make_unique<Fluid_Particles>(mat_prop, init_cond);

  Domain domain;
  domain.x_start = -1.0f;
  domain.x_end   = 1.0f;
  domain.y_start = -1.0f;
  domain.y_end   = 1.0f;
  domain.z_start = 0.0f;
  domain.z_end   = 1.0f;

  const float divide_length = _uptr_particles->support_length();
  const auto& pos_vecetors  = _uptr_particles->get_position_vectors();

  _uptr_neighborhood = std::make_unique<Neighborhood_Uniform_Grid>(domain, divide_length, pos_vecetors);
  
  //const auto num_particles = _uptr_particles->num_particle();
  //_uptr_neighborhood = std::make_unique<Neighborhood_Brute_Force>(num_particles);


  this->init_VS_SRbuffer(cptr_device);
  this->init_VS_SRview(cptr_device);
  this->init_VS(cptr_device);
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

    const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_SB_VS_SRB.GetAddressOf());
    REQUIRE(!FAILED(result), "staging buffer creation should succeed");
  }

  cptr_context->VSSetShader(_cptr_vertex_shader.Get(), 0, 0);
  cptr_context->GSSetShader(_cptr_geometry_shader.Get(), 0, 0);
  cptr_context->PSSetShader(_cptr_pixel_shader.Get(), 0, 0);

  cptr_context->VSSetShaderResources(0, 1, _cptr_VS_SRview.GetAddressOf());
}

SPH::~SPH(void) = default;

void SPH::update(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  const auto& pos_vectors = _uptr_particles->get_position_vectors();
  _uptr_neighborhood->update(pos_vectors);

  _uptr_particles->update(*_uptr_neighborhood);  

  this->update_VS_SRview(cptr_context);
}

void SPH::render(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

  cptr_context->Draw(num_particle, 0);
}

void SPH::init_VS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"SPHVS.hlsl";
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
      _cptr_vertex_shader.GetAddressOf());

    REQUIRE(!FAILED(result), "ID3D11VertexShader creation should succeed");
  }
}

void SPH::init_GS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"SPHGS.hlsl";
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

    REQUIRE(!FAILED(result), "geometry shader compiling should succceed");
  }

  {
    const auto result = cptr_device->CreateGeometryShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_geometry_shader.GetAddressOf());

    REQUIRE(!FAILED(result), "geometry shader creation should succeed");
  }
}

void SPH::init_PS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"SPHPS.hlsl";
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
      _cptr_pixel_shader.GetAddressOf());

    REQUIRE(!FAILED(result), "ID3D11PixelShader creation should succeed");
  }
}

void SPH::update_VS_SRview(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  const UINT     data_size    = sizeof(Vector3);
  const UINT     num_particle = static_cast<UINT>(_uptr_particles->num_particle());
  const Vector3* data_ptr     = _uptr_particles->position_data();

  //Copy CPU to Staging Buffer
  const size_t copy_size = data_size * num_particle;

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_SB_VS_SRB.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
  memcpy(ms.pData, data_ptr, copy_size);
  cptr_context->Unmap(_cptr_SB_VS_SRB.Get(), NULL);

  // Copy Staging Buffer to Vertex Shader Resource Buffer
  cptr_context->CopyResource(_cptr_VS_SRbuffer.Get(), _cptr_SB_VS_SRB.Get());
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
  REQUIRE(!FAILED(result), "vertex shader resource buffer creation should succeed");
}

void SPH::init_VS_SRview(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT num_particle = static_cast<UINT>(_uptr_particles->num_particle());

  D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc = {};
  SRV_desc.Format                          = DXGI_FORMAT_R32G32B32_FLOAT;
  SRV_desc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
  SRV_desc.BufferEx.NumElements            = num_particle;

  const auto result = cptr_device->CreateShaderResourceView(_cptr_VS_SRbuffer.Get(), &SRV_desc, _cptr_VS_SRview.GetAddressOf());
  REQUIRE(!FAILED(result), "vertex shader resource buffer creation should succeed");
}

} // namespace ms
