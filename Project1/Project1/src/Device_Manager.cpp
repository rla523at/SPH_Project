#include "Device_Manager.h"

#include "../../_lib/_header/msexception/Exception.h"
#include <dxgi.h> // DXGIFactory

namespace ms
{

Device_Manager::Device_Manager(const HWND main_window, const int num_row_pixel, const int num_col_pixel, const bool is_vsync_on)
{
  _num_pixel_width  = num_row_pixel;
  _num_pixel_height = num_col_pixel;
  _is_vsync_on      = is_vsync_on;

  this->init_device_device_context();
  this->init_view_port();
  this->init_rasterizer_state();
  this->init_depth_stencil_state();
  this->init_swap_chain_and_depth_stencil_buffer(main_window);
  this->init_render_target_view();
  this->init_depth_stencil_view();

  //debug
  Device_Manager_Debug::_cptr_device     = _cptr_device;
  Device_Manager_Debug::_cptr_context    = _cptr_context;
  Device_Manager_Debug::_cptr_swap_chain = _cptr_swap_chain;
  //debug
}

ComPtr<ID3D11Device> Device_Manager::device_cptr(void) const
{
  return _cptr_device;
}

ComPtr<ID3D11DeviceContext> Device_Manager::context_cptr(void) const
{
  return _cptr_context;
}

float Device_Manager::aspect_ratio() const
{
  return static_cast<float>(_num_pixel_width) / _num_pixel_height;
}

void Device_Manager::switch_buffer(void) const
{
  if (_is_vsync_on)
    _cptr_swap_chain->Present(1, 0);
  else
    _cptr_swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void Device_Manager::prepare_render(void) const
{
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  _cptr_context->ClearRenderTargetView(_cptr_render_target_view.Get(), clearColor);
  _cptr_context->ClearDepthStencilView(_cptr_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  _cptr_context->OMSetRenderTargets(1, _cptr_render_target_view.GetAddressOf(), _cptr_depth_stencil_view.Get());
  _cptr_context->OMSetDepthStencilState(_cptr_depth_stencil_state.Get(), 0);

  _cptr_context->RSSetState(_cptr_rasterizer_state.Get());
}

std::pair<int, int> Device_Manager::render_size(void) const
{
  return {_num_pixel_width, _num_pixel_height};
}

void Device_Manager::init_device_device_context(void)
{
#if defined(_DEBUG)
  constexpr auto flags = D3D11_CREATE_DEVICE_DEBUG;
#else
  constexpr auto flags = NULL;
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
  */

  REQUIRE(_cptr_device != nullptr, "The _cptr_device must be initialized before calling");

  // check 4x MSAA is supported
  constexpr auto swap_chain_format           = DXGI_FORMAT_R8G8B8A8_UNORM;
  constexpr auto depth_stencil_buffer_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  constexpr auto num_sampling                = 4;

  UINT quality;

  _cptr_device->CheckMultisampleQualityLevels(swap_chain_format, num_sampling, &quality);
  const bool is_swap_chain_support_MSAA = quality > 1;

  _cptr_device->CheckMultisampleQualityLevels(depth_stencil_buffer_format, num_sampling, &quality);
  const bool is_depth_stencil_buffer_support_MSAA = quality > 1;

  DXGI_SAMPLE_DESC sample_desc = {};

  if (is_swap_chain_support_MSAA && is_depth_stencil_buffer_support_MSAA)
  {
    sample_desc.Count   = num_sampling;
    sample_desc.Quality = quality - 1;
  }
  else
  {
    sample_desc.Count   = 1;
    sample_desc.Quality = 0;
  }

  // intialize swap chain

  DXGI_MODE_DESC buffer_desc          = {};
  buffer_desc.Width                   = _num_pixel_width;
  buffer_desc.Height                  = _num_pixel_height;
  buffer_desc.RefreshRate.Numerator   = 0;
  buffer_desc.RefreshRate.Denominator = 1;
  buffer_desc.Format                  = swap_chain_format;

  DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
  swap_chain_desc.BufferDesc           = buffer_desc;
  swap_chain_desc.SampleDesc           = sample_desc;
  swap_chain_desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.BufferCount          = 2; // double buffering
  swap_chain_desc.OutputWindow         = output_window;
  swap_chain_desc.Windowed             = TRUE;                                   // windowed/full-screen mode
  swap_chain_desc.Flags                = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
  swap_chain_desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  if (!_is_vsync_on)
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

void Device_Manager::init_render_target_view(void)
{
  REQUIRE(_cptr_swap_chain != nullptr, "The _cptr_swap_chain must be initialized before calling");

  ComPtr<ID3D11Texture2D> cptr_back_buffer;

  const auto result1 = _cptr_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), &cptr_back_buffer);
  REQUIRE(!FAILED(result1), "Getting the back buffer should succeed");

  const auto result2 = _cptr_device->CreateRenderTargetView(cptr_back_buffer.Get(), nullptr, _cptr_render_target_view.GetAddressOf());
  REQUIRE(!FAILED(result2), "ID3D11RenderTargetView creation should succeed");
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

} // namespace ms

//Device_Manager::~Device_Manager()
//{
//  // Cleanup
//  ImGui_ImplDX11_Shutdown();
//  ImGui_ImplWin32_Shutdown();
//  ImGui::DestroyContext();
//
//  DestroyWindow(_main_window);
//}