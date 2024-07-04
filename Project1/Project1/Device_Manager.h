#pragma once
#include <d3d11.h>
#include <wrl.h> // ComPtr
#include <utility>

namespace ms
{
using Microsoft::WRL::ComPtr;

class Device_Manager
{
public:
  Device_Manager(const HWND main_window, const int num_row_pixel, const int num_col_pixel);

public:
  ComPtr<ID3D11Device>        device_cptr(void) const;
  ComPtr<ID3D11DeviceContext> context_cptr(void) const;
  float                       aspect_ratio() const;
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
  int            _num_pixel_width  = 0;
  int            _num_pixel_height  = 0;
  D3D11_VIEWPORT _screen_viewport = {};

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
} // namespace ms