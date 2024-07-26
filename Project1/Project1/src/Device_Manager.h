#pragma once
#include <d3d11.h>
#include <utility>
#include <wrl.h> // ComPtr

namespace ms
{
using Microsoft::WRL::ComPtr;

class Device_Manager
{
public:
  Device_Manager(const HWND main_window, const int num_pixel_width, const int num_pixel_height, const bool is_vsync_on);

public:
  ComPtr<ID3D11Device>        device_cptr(void) const;
  ComPtr<ID3D11DeviceContext> context_cptr(void) const;
  float                       aspect_ratio(void) const;
  void                        switch_buffer(void) const;
  void                        prepare_render(void) const;
  std::pair<int, int>         render_size(void) const;

private:
  void init_device_device_context(void);
  void init_view_port(void);
  void init_rasterizer_state(void);
  void init_depth_stencil_state(void);
  void init_swap_chain_and_depth_stencil_buffer(const HWND main_window);
  void init_render_target_view(void);
  void init_depth_stencil_view(void);

protected:
  bool           _is_vsync_on      = true;
  int            _num_pixel_width  = 0;
  int            _num_pixel_height = 0;
  D3D11_VIEWPORT _screen_viewport  = {};

  ComPtr<ID3D11Device>           _cptr_device;
  ComPtr<ID3D11DeviceContext>    _cptr_context;
  ComPtr<IDXGISwapChain>         _cptr_swap_chain;
  ComPtr<ID3D11RenderTargetView> _cptr_render_target_view;
  ComPtr<ID3D11RasterizerState>  _cptr_rasterizer_state;

  // Depth buffer 관련
  ComPtr<ID3D11Texture2D>         _cptr_depth_stencil_buffer;
  ComPtr<ID3D11DepthStencilView>  _cptr_depth_stencil_view;
  ComPtr<ID3D11DepthStencilState> _cptr_depth_stencil_state;
};

class Device_Manager_Debug
{
public:
  template <typename T>
  static void copy(const T*             data_ptr,
                   const size_t         num_data,
                   ComPtr<ID3D11Buffer> cptr_Sbuffer,
                   ComPtr<ID3D11Buffer> cptr_SRbuffer)
  {
    const auto data_size = sizeof(T);
    const auto copy_size = data_size * num_data;

    //CPU >> Staging Buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    _cptr_context->Map(cptr_Sbuffer.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
    memcpy(ms.pData, data_ptr, copy_size);
    _cptr_context->Unmap(cptr_Sbuffer.Get(), NULL);

    // Staging Buffer >> Shader Resource Buffer
    _cptr_context->CopyResource(cptr_SRbuffer.Get(), cptr_Sbuffer.Get());
  }

public:
  static inline ComPtr<ID3D11Device>        _cptr_device;
  static inline ComPtr<ID3D11DeviceContext> _cptr_context;
  static inline ComPtr<IDXGISwapChain>      _cptr_swap_chain;
  static inline ComPtr<ID3D11Buffer>        _cptr_buffers[10];
};

} // namespace ms