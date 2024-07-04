#include "SPH.h"

#include "../_lib/_header/msexception/Exception.h"
#include <d3dcompiler.h>
#include <random>

namespace ms
{

SPH::SPH(const ComPtr<ID3D11Device> cptr_device)
{
  constexpr size_t num_particle = 10;
  _particles.resize(num_particle);

  // set initial pos

  //std::random_device                    rd;
  //std::mt19937                          gen(rd());
  //std::uniform_real_distribution<float> random_theta(-3.141592f, 3.141592f);
  //std::uniform_real_distribution<float> random_radius(0.0f, 1.0f);

  //const auto v_source_center = Vector3(-0.5f, 0.5f, 0.0f);

  //for (auto& particle : _particles)
  //{
  //  const float   theta  = random_theta(gen);
  //  const float   radius = random_radius(gen);
  //  const Vector3 v_dir  = {std::cos(theta), std::sin(theta), 0.0f};

  //  particle.position = v_source_center + radius * v_dir;
  //}

  constexpr float x_limit = -0.9f;

  Vector3 init_pos = {-0.9f, -0.8f, 0.0f};
  for (int i=0; i<num_particle; ++i)
  {
    _particles[i].position = init_pos;

    init_pos.x += _radius;
    if (init_pos.x > x_limit)
    {
      init_pos.x = -0.9f;
      init_pos.y += 2 * _radius;
    }
  }

  //init Vertex Shader Resource Buffer and Resource View
  {
    const UINT data_size = sizeof(Particle);

    D3D11_BUFFER_DESC buffer_desc   = {};
    buffer_desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
    buffer_desc.Usage               = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags      = NULL;
    buffer_desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = data_size;

    D3D11_SUBRESOURCE_DATA buffer_data = {};
    buffer_data.pSysMem                = _particles.data();
    buffer_data.SysMemPitch            = 0;
    buffer_data.SysMemSlicePitch       = 0;

    const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_VS_SRB.GetAddressOf());
    REQUIRE(!FAILED(result), "vertex shader resource buffer creation should succeed");
  }

  //init Shader Resource View for Vertex Shader
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc = {};
    SRV_desc.Format                          = DXGI_FORMAT_UNKNOWN;
    SRV_desc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
    SRV_desc.BufferEx.NumElements            = num_particle;

    const auto result = cptr_device->CreateShaderResourceView(_cptr_VS_SRB.Get(), &SRV_desc, _cptr_VS_SRV.GetAddressOf());
    REQUIRE(!FAILED(result), "vertex shader resource buffer creation should succeed");
  }

  //init Vertex Shader Staging Buffer
  {
    const UINT data_size = sizeof(Particle);

    D3D11_BUFFER_DESC buffer_desc   = {};
    buffer_desc.ByteWidth           = static_cast<UINT>(data_size * num_particle);
    buffer_desc.Usage               = D3D11_USAGE_STAGING;
    buffer_desc.BindFlags           = NULL;
    buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    buffer_desc.MiscFlags           = NULL;
    buffer_desc.StructureByteStride = NULL;

    D3D11_SUBRESOURCE_DATA buffer_data = {};
    buffer_data.pSysMem                = _particles.data();
    buffer_data.SysMemPitch            = 0;
    buffer_data.SysMemSlicePitch       = 0;

    const auto result = cptr_device->CreateBuffer(&buffer_desc, &buffer_data, _cptr_SB_VS_SRB.GetAddressOf());
    REQUIRE(!FAILED(result), "staging buffer creation should succeed");
  }

  //init_vertex_shader_and_input_layout
  {
    D3D11_INPUT_ELEMENT_DESC dummy_desc = {};
    dummy_desc.SemanticName             = "POSITION";
    dummy_desc.SemanticIndex            = 0;
    dummy_desc.Format                   = DXGI_FORMAT_R32G32B32_FLOAT;
    dummy_desc.InputSlot                = 0;
    dummy_desc.AlignedByteOffset        = 0;
    dummy_desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
    dummy_desc.InstanceDataStepRate     = 0;

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_element_descs = {dummy_desc};

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
      const auto result = cptr_device->CreateInputLayout(
        input_element_descs.data(),
        UINT(input_element_descs.size()),
        shader_blob->GetBufferPointer(),
        shader_blob->GetBufferSize(),
        _cptr_input_layout.GetAddressOf());

      REQUIRE(!FAILED(result), "ID3D11InputLayout creation should succeed");
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

  //init_pixel_shader
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

  //init_geometry_shader
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

      REQUIRE(!FAILED(result), "ID3D11GeometryShader creation should succeed");
    }
  }
}

void SPH::update(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  const float dt = 1.0f / 100.0f; // 고정

  update_density();
  update_force();

  for (int i = 0; i < _particles.size(); i++)
  {
    _particles[i].velocity += _particles[i].force * dt;
    _particles[i].position += _particles[i].velocity * dt;
  }

  const Vector3 gravity       = Vector3(0.0f, -9.8f, 0.0f); // force로 옮기기
  const float   cor           = 0.5f;                       // Coefficient Of Restitution
  const float   ground_height = -0.8f;

  for (auto& p : _particles)
  {
    p.velocity += gravity * dt;

    if (p.position.y < ground_height && p.velocity.y < 0.0f)
    {
      p.velocity.y *= -cor;
      p.position.y = ground_height;
    }

    if (p.position.x < -0.9f && p.velocity.x < 0.0f)
    {
      p.velocity.x *= -cor;
      p.position.x = -0.9f;
    }

    if (p.position.x > 0.9f && p.velocity.x > 0.0f)
    {
      p.velocity.x *= -cor;
      p.position.x = 0.9f;
    }
  }

  //Copy CPU to Staging Buffer
  const size_t copy_size = sizeof(Particle) * _particles.size();

  D3D11_MAPPED_SUBRESOURCE ms;
  cptr_context->Map(_cptr_SB_VS_SRB.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
  memcpy(ms.pData, _particles.data(), copy_size);
  cptr_context->Unmap(_cptr_SB_VS_SRB.Get(), NULL);

  // Copy Staging Buffer to Vertex Shader Resource Buffer
  cptr_context->CopyResource(_cptr_VS_SRB.Get(), _cptr_SB_VS_SRB.Get());
}

void SPH::render(const ComPtr<ID3D11DeviceContext> cptr_context)
{
  cptr_context->VSSetShader(_cptr_vertex_shader.Get(), 0, 0);
  cptr_context->GSSetShader(_cptr_geometry_shader.Get(), 0, 0);
  cptr_context->PSSetShader(_cptr_pixel_shader.Get(), 0, 0);

  cptr_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
  cptr_context->VSSetShaderResources(0, 1, _cptr_VS_SRV.GetAddressOf());
  cptr_context->Draw(UINT(_particles.size()), 0);
}

void SPH::update_density(void)
{
#pragma omp parallel for
  for (size_t i = 0; i < _particles.size(); i++)
  {
    auto& current = _particles[i];

    current.density = 0.0f;

    // the summation over j includes all particles
    // i와 j가 같을 경우에도 고려한다는 의미
    // https://en.wikipedia.org/wiki/Smoothed-particle_hydrodynamics

    for (size_t j = 0; j < _particles.size(); j++)
    {
      const auto& neighbor = _particles[j];

      const float dist = (current.position - neighbor.position).Length();

      if (dist >= _radius)
        continue;

      current.density += kernel(dist / _radius);
    }

    //Equation of State
    current.pressure = pow(current.density, 7.0f) - 1.0f;
  }
}

void SPH::update_force(void)
{
#pragma omp parallel for
  for (int i = 0; i < _particles.size(); i++)
  {
    auto& current = _particles[i];

    Vector3 v_pressure_force(0.0f);
    Vector3 v_viscosity_force(0.0f);

    const float&   rho_i = current.density;
    const float&   p_i   = current.pressure;
    const Vector3& v_xi  = current.position;
    const Vector3& v_vi  = current.velocity;

    for (size_t j = 0; j < _particles.size(); j++)
    {
      const auto& neighbor = _particles[j];

      if (i == j)
        continue;

      const float&   rho_j = neighbor.density;
      const float&   p_j   = neighbor.pressure;
      const Vector3& v_xj  = neighbor.position;
      const Vector3  v_xij = v_xi - v_xj;
      const Vector3& v_vj  = neighbor.velocity;

      const float dist = (v_xi - v_xj).Length();

      if (dist >= _radius)
        continue;

      if (dist < 1e-3f) // 수치 오류 방지
        continue;

      // cal v_grad_pressure
      const auto q     = dist / _radius;
      const auto df_dq = dkernel_dq(q);

      const Vector3 v_grad_q      = 1.0f / (_radius * dist) * v_xij;
      const Vector3 v_grad_kernel = df_dq * v_grad_q;

      const auto coeff = p_i / (rho_i * rho_i) + p_j / (rho_j * rho_j);

      const Vector3 v_grad_pressure = coeff * v_grad_kernel;

      // cal laplacian_velocity
      const Vector3 v_vij  = v_vi - v_vj;
      const auto    coeff2 = 2 * v_xij.Dot(v_grad_kernel) / (rho_j * v_xij.Dot(v_xij) + 0.01f * _radius * _radius);

      const Vector3 laplacian_velocity = coeff2 * v_vij;

      v_pressure_force -= v_grad_pressure;
      v_viscosity_force += _viscosity * laplacian_velocity;
    }

    current.force = v_pressure_force + v_viscosity_force;
  }
}

float SPH::kernel(const float q) const
{
  REQUIRE(q >= 0.0f, "q should be positive");

  constexpr float coeff = 3.0f / (2.0f * 3.141592f);

  if (q < 1.0f)
    return coeff * (2.0f / 3.0f - q * q + 0.5f * q * q * q);
  else if (q < 2.0f)
    return coeff * pow(2.0f - q, 3.0f) / 6.0f;
  else // q >= 2.0f
    return 0.0f;
}

float SPH::dkernel_dq(const float q) const
{
  assert(q >= 0.0f);

  constexpr float coeff = 3.0f / (2.0f * 3.141592f);

  if (q < 1.0f)
    return coeff * (-2.0f * q + 1.5f * q * q);
  else if (q < 2.0f)
    return coeff * -0.5f * (2.0f - q) * (2.0f - q);
  else // q >= 2.0f
    return 0.0f;
}

} // namespace ms
