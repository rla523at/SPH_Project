﻿#include "Device_Manager.h"

#include "../../_lib/_header/msexception/Exception.h"

#include <d3dcompiler.h>
#include <dxgi.h> // DXGIFactory
#include <string>

namespace ms
{

Device_Manager::Device_Manager(
  const HWND main_window,
  const int  num_row_pixel,
  const int  num_col_pixel,
  const bool do_use_Vsync)
{
  _num_pixel_width  = num_row_pixel;
  _num_pixel_height = num_col_pixel;
  _do_use_Vsync     = do_use_Vsync;

  this->init_device_device_context();
  this->init_view_port();
  this->init_rasterizer_state();
  this->init_depth_stencil_state();
  this->init_swap_chain_and_depth_stencil_buffer(main_window);
  this->init_back_buffer_views();
  this->init_depth_stencil_view();
}

void Device_Manager::bind_OM_RTV_and_DSV(void) const
{
  _cptr_context->OMSetRenderTargets(1, _cptr_back_buffer_RTV.GetAddressOf(), _cptr_depth_stencil_view.Get());
}

void Device_Manager::bind_back_buffer_to_CS_UAV(void) const
{
  _cptr_context->CSSetUnorderedAccessViews(0, 1, _cptr_back_buffer_UAV.GetAddressOf(), NULL);
}

void Device_Manager::unbind_OM_RTV_and_DSV(void) const
{
  ID3D11RenderTargetView* null_RTV = nullptr;
  _cptr_context->OMSetRenderTargets(1, &null_RTV, nullptr);
}

void Device_Manager::unbind_CS_UAV(void) const
{
  ID3D11UnorderedAccessView* null_UAV = nullptr;
  _cptr_context->CSSetUnorderedAccessViews(0, 1, &null_UAV, NULL);
}

ComPtr<ID3D11Device> Device_Manager::device_cptr(void) const
{
  return _cptr_device;
}

ComPtr<ID3D11DeviceContext> Device_Manager::context_cptr(void) const
{
  return _cptr_context;
}

ComPtr<ID3D11UnorderedAccessView> Device_Manager::back_buffer_UAV_cptr(void) const
{
  return _cptr_back_buffer_UAV;
}

ComPtr<ID3D11ShaderResourceView> Device_Manager::back_buffer_SRV_cptr(void) const
{
  return _cptr_back_buffer_SRV;
}

float Device_Manager::aspect_ratio() const
{
  return static_cast<float>(_num_pixel_width) / _num_pixel_height;
}

void Device_Manager::switch_buffer(void) const
{
  if (_do_use_Vsync)
    _cptr_swap_chain->Present(1, 0);
  else
    _cptr_swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void Device_Manager::prepare_render(void) const
{
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  _cptr_context->ClearRenderTargetView(_cptr_back_buffer_RTV.Get(), clearColor);
  _cptr_context->ClearDepthStencilView(_cptr_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  // 비교: Depth Buffer를 사용하지 않는 경우
  // _cptr_context->OMSetRenderTargets(1, _cptr_render_target_view.GetAddressOf(), nullptr);
  _cptr_context->OMSetRenderTargets(1, _cptr_back_buffer_RTV.GetAddressOf(), _cptr_depth_stencil_view.Get());
  _cptr_context->OMSetDepthStencilState(_cptr_depth_stencil_state.Get(), 0);

  _cptr_context->RSSetState(_cptr_rasterizer_state.Get());
}

std::pair<int, int> Device_Manager::render_size(void) const
{
  return {_num_pixel_width, _num_pixel_height};
}

void Device_Manager::create_VS_and_IL(
  const wchar_t*                               file_name,
  ComPtr<ID3D11VertexShader>&                  cptr_VS,
  const std::vector<D3D11_INPUT_ELEMENT_DESC>& input_element_descs,
  ComPtr<ID3D11InputLayout>&                   cptr_IL) const
{
  constexpr auto* entry_point_name           = "main";
  constexpr auto  shader_target_profile_name = "vs_5_0";

#if defined(_DEBUG)
  constexpr UINT complie_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  constexpr UINT complie_flag = NULL;
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
      complie_flag,
      NULL,
      shader_blob.GetAddressOf(),
      error_blob.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" compile failed. vertex shader compiling should succeed");
  }

  {
    const auto result = _cptr_device->CreateVertexShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      nullptr,
      cptr_VS.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" creation failed. vertex shader creation should succeed");
  }

  {
    const auto result = _cptr_device->CreateInputLayout(
      input_element_descs.data(),
      static_cast<UINT>(input_element_descs.size()),
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      cptr_IL.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" input layout creation failed. input layout creation should succeed");
  }
}

void Device_Manager::create_CS(const wchar_t* file_name, ComPtr<ID3D11ComputeShader>& cptr_CS) const
{
  constexpr auto* entry_point_name           = "main";
  constexpr auto  shader_target_profile_name = "cs_5_0";

#if defined(_DEBUG)
  constexpr UINT complie_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  constexpr UINT complie_flag = NULL;
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
      complie_flag,
      NULL,
      shader_blob.GetAddressOf(),
      error_blob.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" compile failed. compute shader compiling should succeed");
  }

  {
    const auto result = _cptr_device->CreateComputeShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      nullptr,
      cptr_CS.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" creation failed. compute shader creation should succeed");
  }
}

void Device_Manager::create_PS(const wchar_t* file_name, ComPtr<ID3D11PixelShader>& cptr_PS) const
{
  constexpr auto* entry_point_name           = "main";
  constexpr auto  shader_target_profile_name = "ps_5_0";

#if defined(_DEBUG)
  constexpr UINT complie_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  constexpr UINT complie_flag = NULL;
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
      complie_flag,
      NULL,
      shader_blob.GetAddressOf(),
      error_blob.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" compile failed. pixel shader compiling should succeed");
  }

  {
    const auto result = _cptr_device->CreatePixelShader(
      shader_blob->GetBufferPointer(),
      shader_blob->GetBufferSize(),
      nullptr,
      cptr_PS.GetAddressOf());

    REQUIRE(!FAILED(result), std::wstring(file_name) + L" creation failed. pixel shader creation should succeed");
  }
}

void Device_Manager::create_SRV(ID3D11Resource* resource_ptr, ID3D11ShaderResourceView** SRView_pptr) const
{
  const auto result = _cptr_device->CreateShaderResourceView(resource_ptr, nullptr, SRView_pptr);
  REQUIRE(!FAILED(result), "shader resource view creation should succeed");
}

void Device_Manager::create_UAV(ID3D11Resource* resource_ptr, ID3D11UnorderedAccessView** SRView_pptr) const
{
  const auto result = _cptr_device->CreateUnorderedAccessView(resource_ptr, nullptr, SRView_pptr);
  REQUIRE(!FAILED(result), "unordered access view creation should succeed");
}

void Device_Manager::create_texture_like_back_buffer(ComPtr<ID3D11Texture2D>& cptr_2D_texture) const
{
  const auto cptr_back_buffer = this->back_buffer_cptr();

  D3D11_TEXTURE2D_DESC desc;
  cptr_back_buffer->GetDesc(&desc);

  const auto result = _cptr_device->CreateTexture2D(&desc, nullptr, cptr_2D_texture.GetAddressOf());
  REQUIRE(!FAILED(result), "texture like back buffer creation should succeed");
}

void Device_Manager::create_structured_buffer(ComPtr<ID3D11Buffer>& cptr_buffer, const size_t num_data, const size_t data_size) const
{
  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = num_data * data_size;
  desc.Usage               = D3D11_USAGE_DEFAULT;
  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  desc.CPUAccessFlags      = NULL;
  desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.StructureByteStride = data_size;

  const auto result = _cptr_device->CreateBuffer(&desc, nullptr, cptr_buffer.GetAddressOf());
  REQUIRE(!FAILED(result), "structured buffer creation should succeed");
}

void Device_Manager::copy_back_buffer(const ComPtr<ID3D11Texture2D> cptr_2D_texture) const
{
  const auto cptr_back_buffer = this->back_buffer_cptr();

  _cptr_context->CopyResource(cptr_2D_texture.Get(), cptr_back_buffer.Get());
}

void Device_Manager::CS_barrier(void) const
{
  constexpr size_t num_max_view = 6;

  ID3D11ShaderResourceView*  nullSRV[num_max_view] = {nullptr};
  ID3D11UnorderedAccessView* nullUAV[num_max_view] = {nullptr};

  _cptr_context->CSSetShaderResources(0, num_max_view, nullSRV);
  _cptr_context->CSSetUnorderedAccessViews(0, num_max_view, nullUAV, NULL);
}

void Device_Manager::init_device_device_context(void)
{
#if defined(_DEBUG)
  constexpr auto flags = D3D11_CREATE_DEVICE_DEBUG;
#else
  constexpr auto flags        = NULL;
#endif

  constexpr D3D_FEATURE_LEVEL featureLevels[2] = {
    D3D_FEATURE_LEVEL_11_0, // 더 높은 버전이 먼저 오도록 설정
    D3D_FEATURE_LEVEL_9_3};

  D3D_FEATURE_LEVEL featureLevel;

  const auto result = D3D11CreateDevice(
    nullptr,                     // Specify nullptr to use the default adapter.
    D3D_DRIVER_TYPE_HARDWARE,    // Create a device using the hardware graphics driver.
    NULL,                        // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
    flags,                       // Set debug and Direct2D compatibility flags.
    featureLevels,               // List of feature levels this app can support.
    ARRAYSIZE(featureLevels),    // Size of the list above.
    D3D11_SDK_VERSION,           // Always set this to D3D11_SDK_VERSION for Microsoft Store apps.
    _cptr_device.GetAddressOf(), // Returns the Direct3D device created.
    &featureLevel,               // Returns feature level of device created.
    _cptr_context.GetAddressOf() // Returns the device immediate context.
  );

  REQUIRE(!FAILED(result), "D3D11 device creation sholud succeed");
  REQUIRE(featureLevel == D3D_FEATURE_LEVEL_11_0, "D3D Feature Level 11 should be supported");
}

void Device_Manager::init_swap_chain_and_depth_stencil_buffer(const HWND output_window)
{
  /*
  Direct3D 11에서 멀티샘플링을 사용할 때, back buffer와 depth stencil buffer의 멀티샘플링 설정이 일치해야 한다.
  따라서, 여기서 한꺼번에 처리한다.

  MSAA는 오래된 안티앨리어싱 기술으로, 더이상 사용하지 않는다.
  */

  REQUIRE(_cptr_device != nullptr, "The _cptr_device must be initialized before calling");

  constexpr auto swap_chain_format           = DXGI_FORMAT_R8G8B8A8_UNORM;
  constexpr auto depth_stencil_buffer_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // MSAA는 사용하지 않음으로, 기본값을 사용한다.
  DXGI_SAMPLE_DESC sample_desc = {};
  sample_desc.Count            = 1;
  sample_desc.Quality          = 0;

  DXGI_MODE_DESC buffer_desc          = {};
  buffer_desc.Width                   = _num_pixel_width;
  buffer_desc.Height                  = _num_pixel_height;
  buffer_desc.RefreshRate.Numerator   = 0;
  buffer_desc.RefreshRate.Denominator = 1;
  buffer_desc.Format                  = swap_chain_format;

  DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
  swap_chain_desc.BufferDesc           = buffer_desc;
  swap_chain_desc.SampleDesc           = sample_desc;
  swap_chain_desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_UNORDERED_ACCESS;
  swap_chain_desc.BufferCount          = 2; // double buffering
  swap_chain_desc.OutputWindow         = output_window;
  swap_chain_desc.Windowed             = TRUE;                                   // windowed/full-screen mode
  swap_chain_desc.Flags                = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
  swap_chain_desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  if (!_do_use_Vsync)
    swap_chain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

  ComPtr<IDXGIFactory> cptr_DXGI_factory;
  {
    const auto result = CreateDXGIFactory(__uuidof(IDXGIFactory), &cptr_DXGI_factory);
    REQUIRE(!FAILED(result), "DXGIFactory creation should succeed");
  }
  {
    const auto result = cptr_DXGI_factory->CreateSwapChain(_cptr_device.Get(), &swap_chain_desc, _cptr_swap_chain.GetAddressOf());
    REQUIRE(!FAILED(result), "IDXGISwapChain creation should succeed");
  }

  // initialize depth stencil buffer
  D3D11_TEXTURE2D_DESC depth_stencil_buffer_desc;
  depth_stencil_buffer_desc.Width          = _num_pixel_width;
  depth_stencil_buffer_desc.Height         = _num_pixel_height;
  depth_stencil_buffer_desc.MipLevels      = 1;
  depth_stencil_buffer_desc.ArraySize      = 1;
  depth_stencil_buffer_desc.Format         = depth_stencil_buffer_format;
  depth_stencil_buffer_desc.SampleDesc     = sample_desc;
  depth_stencil_buffer_desc.Usage          = D3D11_USAGE_DEFAULT;
  depth_stencil_buffer_desc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
  depth_stencil_buffer_desc.CPUAccessFlags = NULL;
  depth_stencil_buffer_desc.MiscFlags      = NULL;

  {
    const auto result = _cptr_device->CreateTexture2D(&depth_stencil_buffer_desc, nullptr, _cptr_depth_stencil_buffer.GetAddressOf());
    REQUIRE(!FAILED(result), "depth stencil buffer creation should succeed");
  }
}

void Device_Manager::init_back_buffer_views(void)
{
  REQUIRE(_cptr_swap_chain != nullptr, "The _cptr_swap_chain must be initialized before calling");

  ComPtr<ID3D11Texture2D> cptr_back_buffer;

  {
    const auto result = _cptr_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), &cptr_back_buffer);
    REQUIRE(!FAILED(result), "Getting the back buffer should succeed");
  }

  {
    const auto result = _cptr_device->CreateRenderTargetView(cptr_back_buffer.Get(), nullptr, _cptr_back_buffer_RTV.GetAddressOf());
    REQUIRE(!FAILED(result), "back buffer render target view creation should succeed");
  }

  {
    const auto result = _cptr_device->CreateShaderResourceView(cptr_back_buffer.Get(), nullptr, _cptr_back_buffer_SRV.GetAddressOf());
    REQUIRE(!FAILED(result), "back buffer shader resource view creation should succeed");
  }

  {
    const auto result = _cptr_device->CreateUnorderedAccessView(cptr_back_buffer.Get(), nullptr, _cptr_back_buffer_UAV.GetAddressOf());
    REQUIRE(!FAILED(result), "back buffer unordered access view creation should succeed");
  }
}

void Device_Manager::init_view_port(void)
{
  REQUIRE(_cptr_context != nullptr, "The _cptr_context must be initialized before calling");

  _screen_viewport.TopLeftX = 0;
  _screen_viewport.TopLeftY = 0;
  _screen_viewport.Width    = static_cast<float>(_num_pixel_width);
  _screen_viewport.Height   = static_cast<float>(_num_pixel_height);
  _screen_viewport.MinDepth = 0.0f;
  _screen_viewport.MaxDepth = 1.0f; // Note: important for depth buffering

  _cptr_context->RSSetViewports(1, &_screen_viewport);
}

void Device_Manager::init_rasterizer_state(void)
{
  REQUIRE(_cptr_device != nullptr, "The _cptr_device must be initialized before calling");

  D3D11_RASTERIZER_DESC rasterizer_desc = {};
  rasterizer_desc.FillMode              = D3D11_FILL_MODE::D3D11_FILL_SOLID;
  rasterizer_desc.CullMode              = D3D11_CULL_MODE::D3D11_CULL_NONE;
  rasterizer_desc.FrontCounterClockwise = FALSE;

  const auto result = _cptr_device->CreateRasterizerState(&rasterizer_desc, _cptr_rasterizer_state.GetAddressOf());
  REQUIRE(!FAILED(result), "ID3D11RasterizerState creation should succeed");
}

void Device_Manager::init_depth_stencil_state(void)
{
  REQUIRE(_cptr_device != nullptr, "The _cptr_device must be initialized before calling");

  D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
  depth_stencil_desc.DepthEnable              = TRUE;
  depth_stencil_desc.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
  depth_stencil_desc.DepthFunc                = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

  const auto result = _cptr_device->CreateDepthStencilState(&depth_stencil_desc, _cptr_depth_stencil_state.GetAddressOf());
  REQUIRE(!FAILED(result), "ID3D11DepthStencilState creation should succeed");
}

void Device_Manager::init_depth_stencil_view(void)
{
  REQUIRE(_cptr_depth_stencil_buffer != nullptr, "The _cptr_depth_stencil_buffer must be initialized before calling");

  const auto result = _cptr_device->CreateDepthStencilView(_cptr_depth_stencil_buffer.Get(), nullptr, _cptr_depth_stencil_view.GetAddressOf());
  REQUIRE(!FAILED(result), "ID3D11DepthStencilView creation should succeed");
}

ComPtr<ID3D11Texture2D> Device_Manager::back_buffer_cptr(void) const
{
  ComPtr<ID3D11Texture2D> cptr_back_buffer;

  {
    const auto result = _cptr_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), &cptr_back_buffer);
    REQUIRE(!FAILED(result), "Getting the back buffer should succeed");
  }

  return cptr_back_buffer;
}

} // namespace ms

// Device_Manager::~Device_Manager()
//{
//   // Cleanup
//   ImGui_ImplDX11_Shutdown();
//   ImGui_ImplWin32_Shutdown();
//   ImGui::DestroyContext();
//
//   DestroyWindow(_main_window);
// }