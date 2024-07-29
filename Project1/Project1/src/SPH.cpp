#include "SPH.h"

#include "Camera.h"
#include "Device_Manager.h"
#include "PCISPH.h"
#include "WCSPH.h"
#include "Window_Manager.h"

#include "../../_lib/_header/msexception/Exception.h"
#include <d3dcompiler.h>
#include <random>

namespace ms
{

SPH::SPH(const ComPtr<ID3D11Device> cptr_device, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  Domain solution_domain;
  solution_domain.x_start = -2.0f;
  solution_domain.x_end   = 2.0f;
  solution_domain.y_start = 0.0f;
  solution_domain.y_end   = 10.0f;
  solution_domain.z_start = -0.3f;
  solution_domain.z_end   = 0.3f;

  //solution_domain.x_start = 2.4f;
  //solution_domain.x_end   = 2.5f;
  //solution_domain.y_start = 0.1f;
  //solution_domain.y_end   = 0.2f;
  //solution_domain.z_start = 0.9f;
  //solution_domain.z_end   = 1.0f;

  std::vector<Domain> init_cond_domains;

  //IC Start

  ////zero gravity domain
  //Domain init_cond_domain;
  //init_cond_domain.x_start = -0.1f;
  //init_cond_domain.x_end   = 0.1f;
  //init_cond_domain.y_start = 1.0f;
  //init_cond_domain.y_end   = 1.2f;
  //init_cond_domain.z_start = -0.1f;
  //init_cond_domain.z_end   = 0.1f;
  //
  //init_cond_domains.push_back(init_cond_domain);

  //constexpr float square_cvel = 1000;

  //// Solo Particle
  //Domain init_cond_domain;
  //init_cond_domain.x_start = -0.2f;
  //init_cond_domain.x_end   = 0.2f;
  //init_cond_domain.y_start = 1.0f;
  //init_cond_domain.y_end   = 1.4f;
  //init_cond_domain.z_start = -0.2f;
  //init_cond_domain.z_end   = 0.2f;

  //init_cond_domains.push_back(init_cond_domain);

  //constexpr float eta         = 0.01f; // Tait's equation parameter
  //const float     square_cvel = 100.0f;

  //Dam breaking
  Domain init_cond_domain;
  init_cond_domain.x_start = -1.9f;
  init_cond_domain.x_end   = -0.9f;
  init_cond_domain.y_start = 0.2f;
  init_cond_domain.y_end   = 2.2f;
  init_cond_domain.z_start = -0.2f;
  init_cond_domain.z_end   = 0.2f;

  init_cond_domains.push_back(init_cond_domain);

  constexpr float eta         = 0.01f; // Tait's equation parameter
  const float     square_cvel = 2.0f * 9.8f * init_cond_domain.dy() / eta;

  //// Double Dam breaking
  //Domain dam1;
  //dam1.x_start = -1.4f;
  //dam1.x_end   = -0.4f;
  //dam1.y_start = 1.1f;
  //dam1.y_end   = 5.1f;
  //dam1.z_start = -1.4f;
  //dam1.z_end   = -0.4f;

  //Domain dam2;
  //dam2.x_start = 0.4f;
  //dam2.x_end   = 1.4f;
  //dam2.y_start = 1.1f;
  //dam2.y_end   = 5.1f;
  //dam2.z_start = 0.4f;
  //dam2.z_end   = 1.4f;

  //init_cond_domains.push_back(dam1);
  //init_cond_domains.push_back(dam2);

  //constexpr float eta         = 0.01f; // Tait's equation parameter
  //const float     square_cvel = 2.0f * 9.8f * (std::max)(dam1.dy(), dam2.dy()) / eta;

  // IC END

  Initial_Condition_Cubes init_cond;
  init_cond.domains          = init_cond_domains;
  init_cond.particle_spacing = 0.04f;

  constexpr float rest_density = 1.0e3f;
  constexpr float gamma        = 7.0f; // Tait's equation parameter

  Material_Property mat_prop;
  mat_prop.sqaure_sound_speed   = square_cvel;
  mat_prop.rest_density         = rest_density;
  mat_prop.gamma                = gamma;
  mat_prop.pressure_coefficient = rest_density * square_cvel / (gamma);
  mat_prop.viscosity            = 1.0e-3f;

  _uptr_SPH_Scheme = std::make_unique<WCSPH>(mat_prop, init_cond, solution_domain);
  //_uptr_SPH_Scheme = std::make_unique<PCISPH>(init_cond, solution_domain);

  _GS_Cbuffer_data.radius = _uptr_SPH_Scheme->particle_radius()*0.5f;

  this->init_VS_SRbuffer_pos(cptr_device);
  this->init_VS_SRview_pos(cptr_device);
  this->init_VS_Sbuffer_pos(cptr_device);
  this->init_VS_SRbuffer_density(cptr_device);
  this->init_VS_SRview_density(cptr_device);
  this->init_VS_Sbuffer_density(cptr_device);
  this->init_VS(cptr_device);
  this->init_GS_Cbuffer(cptr_device);
  this->init_GS(cptr_device);
  this->init_PS(cptr_device);

  this->init_boundary_Vbuffer(cptr_device);
  this->init_boundary_VS(cptr_device);
  this->init_boundary_PS(cptr_device);
  this->init_boundary_blender_state(cptr_device);

  //debug
  Device_Manager_Debug::_cptr_buffers[0] = _cptr_VS_SRbuffer_pos;
  Device_Manager_Debug::_cptr_buffers[1] = _cptr_VS_Sbuffer_pos;
  Device_Manager_Debug::_cptr_buffers[2] = _cptr_VS_SRbuffer_density;
  Device_Manager_Debug::_cptr_buffers[3] = _cptr_VS_Sbuffer_density;
  //debug
}

SPH::~SPH(void) = default;

void SPH::update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context)
{
  static bool stop_update = false;

  if (Window_Manager::is_space_key_pressed())
    stop_update = true;

  if (Window_Manager::is_x_key_pressed())
    stop_update = false;

  if (!stop_update)
    _uptr_SPH_Scheme->update();

  this->update_VS_SRview(cptr_context);
  //this->update_Vbuffer(cptr_context);

  _GS_Cbuffer_data.v_cam_pos = camera.position_vector();
  _GS_Cbuffer_data.v_cam_up  = camera.up_vector();
  _GS_Cbuffer_data.m_view    = camera.view_matrix();
  _GS_Cbuffer_data.m_proj    = camera.proj_matrix();

  this->update_GS_Cbuffer(cptr_context);
}

void SPH::render(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  this->set_fluid_graphics_pipeline(cptr_context);

  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());
  cptr_context->Draw(num_particle, 0);

  //this->set_boundary_graphics_pipeline(cptr_context);
  //const auto num_boundary_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_boundary_particle());
  //cptr_context->Draw(num_boundary_particle, 0);

  this->reset_graphics_pipeline(cptr_context);
}

void SPH::init_VS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"hlsl/SPH_VS.hlsl";
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
  constexpr auto* file_name                   = L"hlsl/SPH_GS.hlsl";
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
  constexpr auto* file_name                   = L"hlsl/SPH_PS.hlsl";
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
  {
    const auto  data_size    = sizeof(Vector3);
    const auto  num_particle = _uptr_SPH_Scheme->num_fluid_particle();
    const auto* data_ptr     = _uptr_SPH_Scheme->fluid_particle_position_data();
    const auto  copy_size    = data_size * num_particle;

    D3D11_MAPPED_SUBRESOURCE ms;
    cptr_context->Map(_cptr_boundary_Vbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(ms.pData, data_ptr, copy_size);
    cptr_context->Unmap(_cptr_boundary_Vbuffer.Get(), NULL);
  }
}

void SPH::update_VS_SRview(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  {
    const UINT  data_size    = sizeof(Vector3);
    const UINT  num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());
    const auto* data_ptr     = _uptr_SPH_Scheme->fluid_particle_position_data();

    //Copy CPU to Staging Buffer
    const size_t copy_size = data_size * num_particle;

    D3D11_MAPPED_SUBRESOURCE ms;
    cptr_context->Map(_cptr_VS_Sbuffer_pos.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
    memcpy(ms.pData, data_ptr, copy_size);
    cptr_context->Unmap(_cptr_VS_Sbuffer_pos.Get(), NULL);

    // Copy Staging Buffer to Vertex Shader Resource Buffer
    cptr_context->CopyResource(_cptr_VS_SRbuffer_pos.Get(), _cptr_VS_Sbuffer_pos.Get());
  }
  {
    const UINT  data_size    = sizeof(float);
    const UINT  num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());
    const auto* data_ptr     = _uptr_SPH_Scheme->fluid_particle_density_data();

    //CPU >> Staging Buffer
    const size_t copy_size = data_size * num_particle;

    D3D11_MAPPED_SUBRESOURCE ms;
    cptr_context->Map(_cptr_VS_Sbuffer_density.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
    memcpy(ms.pData, data_ptr, copy_size);
    cptr_context->Unmap(_cptr_VS_Sbuffer_density.Get(), NULL);

    // Staging Buffer >> Vertex Shader Resource Buffer
    cptr_context->CopyResource(_cptr_VS_SRbuffer_density.Get(), _cptr_VS_Sbuffer_density.Get());
  }
}

void SPH::update_GS_Cbuffer(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  constexpr auto data_size = sizeof(SPH_GS_Cbuffer_Data);

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_GS_Cbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
  memcpy(ms.pData, &_GS_Cbuffer_data, data_size);
  cptr_context->Unmap(_cptr_GS_Cbuffer.Get(), NULL);
}

void SPH::set_fluid_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

  constexpr size_t          num_srviews          = 2;
  ID3D11ShaderResourceView* SRviews[num_srviews] = {_cptr_VS_SRview_pos.Get(), _cptr_VS_SRview_density.Get()};
  cptr_context->VSSetShaderResources(0, num_srviews, SRviews);

  cptr_context->VSSetShader(_cptr_VS.Get(), nullptr, 0);
  cptr_context->GSSetConstantBuffers(0, 1, _cptr_GS_Cbuffer.GetAddressOf());
  cptr_context->GSSetShader(_cptr_GS.Get(), nullptr, 0);
  cptr_context->PSSetShader(_cptr_PS.Get(), nullptr, 0);
}

void SPH::set_boundary_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  UINT stride = sizeof(Vector3);
  UINT offset = 0;
  cptr_context->IASetVertexBuffers(0, 1, _cptr_boundary_Vbuffer.GetAddressOf(), &stride, &offset);
  cptr_context->IASetInputLayout(_cptr_boundary_input_layout.Get());
  cptr_context->VSSetShader(_cptr_boundary_VS.Get(), nullptr, 0);
  cptr_context->PSSetShader(_cptr_boundary_PS.Get(), nullptr, 0);
  cptr_context->OMSetBlendState(_cptr_boundary_blend_state.Get(), nullptr, 0XFFFFFFFF);
}

void SPH::reset_graphics_pipeline(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  UINT stride = 0;
  UINT offset = 0;
  cptr_context->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);
  cptr_context->IASetInputLayout(nullptr);
  cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);
  cptr_context->VSSetShaderResources(0, 0, nullptr);
  cptr_context->VSSetShader(nullptr, nullptr, 0);
  cptr_context->GSSetConstantBuffers(0, 0, nullptr);
  cptr_context->GSSetShader(nullptr, nullptr, 0);
  cptr_context->PSSetShader(nullptr, nullptr, 0);

  cptr_context->OMSetBlendState(nullptr, nullptr, 0XFFFFFFFF); //SAMPLE MASK에 NULL 넣으면 안된다.
}

void SPH::init_VS_SRbuffer_pos(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT data_size    = sizeof(Vector3);
  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  buffer_desc.Usage               = D3D11_USAGE_DEFAULT;
  buffer_desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  buffer_desc.CPUAccessFlags      = NULL;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = _uptr_SPH_Scheme->fluid_particle_position_data();
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &initial_data, _cptr_VS_SRbuffer_pos.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader resource buffer creation should succeed");
}

void SPH::init_VS_Sbuffer_pos(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT data_size    = sizeof(Vector3);
  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());

  D3D11_BUFFER_DESC buffer_desc   = {};
  buffer_desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  buffer_desc.Usage               = D3D11_USAGE_STAGING;
  buffer_desc.BindFlags           = NULL;
  buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  buffer_desc.MiscFlags           = NULL;
  buffer_desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA buffer_data = {};
  buffer_data.pSysMem                = _uptr_SPH_Scheme->fluid_particle_position_data();
  buffer_data.SysMemPitch            = 0;
  buffer_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_VS_Sbuffer_pos.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader staging buffer creation should succeed");
}

void SPH::init_VS_SRview_pos(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());

  D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc = {};
  SRV_desc.Format                          = DXGI_FORMAT_R32G32B32_FLOAT;
  SRV_desc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
  SRV_desc.BufferEx.NumElements            = num_particle;

  const auto result = cptr_device->CreateShaderResourceView(_cptr_VS_SRbuffer_pos.Get(), &SRV_desc, _cptr_VS_SRview_pos.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader resource buffer creation should succeed");
}

void SPH::init_VS_SRbuffer_density(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT data_size    = sizeof(float);
  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  desc.Usage               = D3D11_USAGE_DEFAULT;
  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags      = NULL;
  desc.MiscFlags           = NULL;
  desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = _uptr_SPH_Scheme->fluid_particle_density_data();
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&desc, &initial_data, _cptr_VS_SRbuffer_density.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader resource buffer density creation should succeed");
}

void SPH::init_VS_Sbuffer_density(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT data_size    = sizeof(float);
  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  desc.Usage               = D3D11_USAGE_STAGING;
  desc.BindFlags           = NULL;
  desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags           = NULL;
  desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA initial_data = {};
  initial_data.pSysMem                = _uptr_SPH_Scheme->fluid_particle_density_data();
  initial_data.SysMemPitch            = 0;
  initial_data.SysMemSlicePitch       = 0;

  const auto result = cptr_device->CreateBuffer(&desc, &initial_data, _cptr_VS_Sbuffer_density.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader staging buffer density creation should succeed");
}

void SPH::init_VS_SRview_density(const ComPtr<ID3D11Device> cptr_device)
{
  const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_fluid_particle());

  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format                          = DXGI_FORMAT_R32_FLOAT;
  desc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
  desc.BufferEx.NumElements            = num_particle;

  const auto result = cptr_device->CreateShaderResourceView(_cptr_VS_SRbuffer_density.Get(), &desc, _cptr_VS_SRview_density.GetAddressOf());
  REQUIRE(!FAILED(result), "SPH vertex shader resource buffer density creation should succeed");
}

void SPH::init_boundary_Vbuffer(const ComPtr<ID3D11Device> cptr_device)
{
  //const UINT data_size    = sizeof(Vector3);
  //const UINT num_particle = static_cast<UINT>(_uptr_SPH_Scheme->num_boundary_particle());

  //D3D11_BUFFER_DESC desc   = {};
  //desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
  //desc.Usage               = D3D11_USAGE_IMMUTABLE;
  //desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
  //desc.CPUAccessFlags      = NULL;
  //desc.MiscFlags           = NULL;
  //desc.StructureByteStride = NULL;

  //D3D11_SUBRESOURCE_DATA initial_data = {};
  //initial_data.pSysMem                = _uptr_SPH_Scheme->boundary_particle_position_data();
  //initial_data.SysMemPitch            = 0;
  //initial_data.SysMemSlicePitch       = 0;

  //const auto result = cptr_device->CreateBuffer(&desc, &initial_data, _cptr_boundary_Vbuffer.GetAddressOf());
  //REQUIRE(!FAILED(result), "SPH vertex buffer creation should succeed");
}

void SPH::init_boundary_VS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                   = L"hlsl/SPH_boundary_VS.hlsl";
  constexpr auto* entry_point_name            = "main";
  constexpr auto* sharder_target_profile_name = "vs_5_0";

  D3D11_INPUT_ELEMENT_DESC pos_desc = {};
  pos_desc.SemanticName             = "POSITION";
  pos_desc.SemanticIndex            = 0;
  pos_desc.Format                   = DXGI_FORMAT_R32G32B32_FLOAT;
  pos_desc.InputSlot                = 0;
  pos_desc.AlignedByteOffset        = 0;
  pos_desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
  pos_desc.InstanceDataStepRate     = 0;

  constexpr UINT           num_input                      = 1;
  D3D11_INPUT_ELEMENT_DESC input_element_descs[num_input] = {pos_desc};

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

    REQUIRE(!FAILED(result), "SPH boundary vertex shader compiling should succeed");
  }

  {
    const auto result = cptr_device->CreateInputLayout(
      input_element_descs,
      num_input,
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      _cptr_boundary_input_layout.GetAddressOf());

    REQUIRE(!FAILED(result), "SPH boundary input layout creation should succeed");
  }

  {
    const auto result = cptr_device->CreateVertexShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_boundary_VS.GetAddressOf());

    REQUIRE(!FAILED(result), "SPH boundary vertex shader creation should succeed");
  }
}

void SPH::init_boundary_PS(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr auto* file_name                  = L"hlsl/SPH_boundary_PS.hlsl";
  constexpr auto* entry_point_name           = "main";
  constexpr auto* shader_target_profile_name = "ps_5_0";

#if defined(_DEBUG)
  constexpr UINT compile_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  constexpr UINT compile_flag  = NULL;
#endif

  ComPtr<ID3DBlob> shader_blob;
  ComPtr<ID3DBlob> error_blob;

  {
    const auto result = D3DCompileFromFile(
      file_name,
      nullptr,
      nullptr,
      entry_point_name,
      shader_target_profile_name,
      compile_flag,
      NULL,
      &shader_blob,
      &error_blob);

    REQUIRE(!FAILED(result), "SPH boundary pixel shader compiling should succeed");
  }
  {
    const auto result = cptr_device->CreatePixelShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      NULL,
      _cptr_boundary_PS.GetAddressOf());

    REQUIRE(!FAILED(result), "SPH boundary pixel shader creation should succeed");
  }
}

void SPH::init_boundary_blender_state(const ComPtr<ID3D11Device> cptr_device)
{
  D3D11_BLEND_DESC desc;
  desc.AlphaToCoverageEnable                 = FALSE;
  desc.IndependentBlendEnable                = FALSE;
  desc.RenderTarget[0].BlendEnable           = true;
  desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
  desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
  desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
  desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
  desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
  desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
  desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  cptr_device->CreateBlendState(&desc, _cptr_boundary_blend_state.GetAddressOf());
}

} // namespace ms
