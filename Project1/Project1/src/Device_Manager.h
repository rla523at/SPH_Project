#pragma once
#include "Abbreviation.h"

#include <d3d11.h>
#include <utility>
#include <vector>

namespace ms
{
class Device_Manager
{
public:
  Device_Manager(
    const HWND main_window,
    const int  num_row_pixel,
    const int  num_col_pixel,
    const bool do_use_Vsync);

public:
  void bind_OM_RTV_and_DSV(void) const;
  void bind_back_buffer_to_CS_UAV(void) const;

  void create_VS_and_IL(
    const wchar_t*                               file_name,
    ComPtr<ID3D11VertexShader>&                  cptr_VS,
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& input_element_descs,
    ComPtr<ID3D11InputLayout>&                   cptr_IL) const;

  void create_CS(const wchar_t* file_name, ComPtr<ID3D11ComputeShader>& cptr_CS) const;
  void create_PS(const wchar_t* file_name, ComPtr<ID3D11PixelShader>& cptr_PS) const;
  void create_SRV(ID3D11Resource* resource_ptr, ID3D11ShaderResourceView** SRView_pptr) const;
  void create_UAV(ID3D11Resource* resource_ptr, ID3D11UnorderedAccessView** SRView_pptr) const;

  void create_texture_like_back_buffer(ComPtr<ID3D11Texture2D>& cptr_2D_texture) const;

  void copy_back_buffer(const ComPtr<ID3D11Texture2D> cptr_2D_texture) const;

  void unbind_OM_RTV_and_DSV(void) const;
  void unbind_CS_UAV(void) const;

  ComPtr<ID3D11Device>              device_cptr(void) const;
  ComPtr<ID3D11DeviceContext>       context_cptr(void) const;
  ComPtr<ID3D11UnorderedAccessView> back_buffer_UAV_cptr(void) const;
  ComPtr<ID3D11ShaderResourceView>  back_buffer_SRV_cptr(void) const;
  float                             aspect_ratio(void) const;
  void                              switch_buffer(void) const;
  void                              prepare_render(void) const;
  std::pair<int, int>               render_size(void) const;

  void CS_barrier(void) const;

private:
  void init_device_device_context(void);
  void init_view_port(void);
  void init_rasterizer_state(void);
  void init_depth_stencil_state(void);
  void init_swap_chain_and_depth_stencil_buffer(const HWND main_window);
  void init_back_buffer_views(void);
  void init_depth_stencil_view(void);

  ComPtr<ID3D11Texture2D> back_buffer_cptr(void) const;


protected:
  bool           _do_use_Vsync     = true;
  int            _num_pixel_width  = 0;
  int            _num_pixel_height = 0;
  D3D11_VIEWPORT _screen_viewport  = {};

  ComPtr<ID3D11Device>              _cptr_device;
  ComPtr<ID3D11DeviceContext>       _cptr_context;
  ComPtr<IDXGISwapChain>            _cptr_swap_chain;
  ComPtr<ID3D11RenderTargetView>    _cptr_back_buffer_RTV;
  ComPtr<ID3D11UnorderedAccessView> _cptr_back_buffer_UAV;
  ComPtr<ID3D11ShaderResourceView>  _cptr_back_buffer_SRV;
  ComPtr<ID3D11RasterizerState>     _cptr_rasterizer_state;

  // Depth buffer 관련
  ComPtr<ID3D11Texture2D>         _cptr_depth_stencil_buffer;
  ComPtr<ID3D11DepthStencilView>  _cptr_depth_stencil_view;
  ComPtr<ID3D11DepthStencilState> _cptr_depth_stencil_state;
};
} // namespace ms