﻿#pragma once
#include "Abbreviation.h"

#include "../../_lib/_header/msexception/Exception.h"
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
  template <typename T>
  ComPtr<ID3D11Buffer> create_structured_buffer(const size_t num_data, T* init_data_ptr = nullptr) const;

  template <typename T>
  std::vector<T> download(const ComPtr<ID3D11Buffer> staging_buffer) const;

  template <typename T>
  void upload(const T* data_ptr, const ComPtr<ID3D11Buffer> cptr_buffer) const;

public:
  void create_VS_and_IL(
    const wchar_t*                               file_name,
    ComPtr<ID3D11VertexShader>&                  cptr_VS,
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& input_element_descs,
    ComPtr<ID3D11InputLayout>&                   cptr_IL) const;

  ComPtr<ID3D11ComputeShader>       create_CS(const wchar_t* file_name) const;
  void                              create_PS(const wchar_t* file_name, ComPtr<ID3D11PixelShader>& cptr_PS) const;

  ComPtr<ID3D11ShaderResourceView>  create_SRV(ID3D11Resource* resource_ptr) const;
  ComPtr<ID3D11UnorderedAccessView> create_UAV(ID3D11Resource* resource_ptr) const;
  ComPtr<ID3D11UnorderedAccessView> create_AC_UAV(ID3D11Resource* resource_ptr, const UINT num_data) const;

  ComPtr<ID3D11Buffer> create_constant_buffer(const UINT data_size) const;
  ComPtr<ID3D11Buffer> create_staging_buffer_read(const ComPtr<ID3D11Buffer> cptr_target_buffer) const;
  ComPtr<ID3D11Buffer> create_staging_buffer_write(const ComPtr<ID3D11Buffer> cptr_target_buffer) const;

  void create_texture_like_back_buffer(ComPtr<ID3D11Texture2D>& cptr_2D_texture) const;
    
  void bind_OM_RTV_and_DSV(void) const;
  void bind_back_buffer_to_CS_UAV(void) const;

  void unbind_OM_RTV_and_DSV(void) const;
  void unbind_CS_UAV(void) const;

  void copy_back_buffer(const ComPtr<ID3D11Texture2D> cptr_2D_texture) const;
  void CS_barrier(void) const;
  UINT read_count(const ComPtr<ID3D11UnorderedAccessView> UAV) const;


  ComPtr<ID3D11Device>              device_cptr(void) const;
  ComPtr<ID3D11DeviceContext>       context_cptr(void) const;
  ComPtr<ID3D11UnorderedAccessView> back_buffer_UAV_cptr(void) const;
  ComPtr<ID3D11ShaderResourceView>  back_buffer_SRV_cptr(void) const;
  float                             aspect_ratio(void) const;
  void                              switch_buffer(void) const;
  void                              prepare_render(void) const;
  std::pair<int, int>               render_size(void) const;


private:
  void init_device_device_context(void);
  void init_view_port(void);
  void init_rasterizer_state(void);
  void init_depth_stencil_state(void);
  void init_swap_chain_and_depth_stencil_buffer(const HWND main_window);
  void init_back_buffer_views(void);
  void init_depth_stencil_view(void);

  ComPtr<ID3D11Texture2D> back_buffer_cptr(void) const;
  ComPtr<ID3D11Buffer>    create_staging_buffer_count(void) const;


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

  ComPtr<ID3D11Buffer> _cptr_count_staging_buffer;

  // Depth buffer 관련
  ComPtr<ID3D11Texture2D>         _cptr_depth_stencil_buffer;
  ComPtr<ID3D11DepthStencilView>  _cptr_depth_stencil_view;
  ComPtr<ID3D11DepthStencilState> _cptr_depth_stencil_state;
};

} // namespace ms

//template definition
namespace ms
{
template <typename T>
ComPtr<ID3D11Buffer> Device_Manager::create_structured_buffer(const size_t num_data, T* init_data_ptr) const
{
  const auto data_size = sizeof(T);

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(num_data * data_size);
  desc.Usage               = D3D11_USAGE_DEFAULT;
  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  desc.CPUAccessFlags      = NULL;
  desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.StructureByteStride = data_size;

  D3D11_SUBRESOURCE_DATA init_data = {};
  init_data.pSysMem                = init_data_ptr;
  init_data.SysMemPitch            = 0;
  init_data.SysMemSlicePitch       = 0;

  ComPtr<ID3D11Buffer> cptr_structured_buffer;

  HRESULT result = {};
  if (init_data_ptr == nullptr)
    result = _cptr_device->CreateBuffer(&desc, nullptr, cptr_structured_buffer.GetAddressOf());
  else
    result = _cptr_device->CreateBuffer(&desc, &init_data, cptr_structured_buffer.GetAddressOf());

  REQUIRE(!FAILED(result), "structured buffer creation should succeed");

  return cptr_structured_buffer;
}

template <typename T>
std::vector<T> Device_Manager::download(const ComPtr<ID3D11Buffer> staging_buffer) const
{
  D3D11_BUFFER_DESC desc;
  staging_buffer->GetDesc(&desc);

  constexpr auto data_size = sizeof(T);
  const auto     num_data  = desc.ByteWidth / data_size;

  std::vector<T> datas(num_data);

  D3D11_MAPPED_SUBRESOURCE ms;
  _cptr_context->Map(staging_buffer.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
  memcpy(datas.data(), ms.pData, num_data * data_size);
  _cptr_context->Unmap(staging_buffer.Get(), NULL);

  return datas;
}

template <typename T>
void Device_Manager::upload(const T* data_ptr, const ComPtr<ID3D11Buffer> cptr_buffer) const
{
  D3D11_BUFFER_DESC desc;
  cptr_buffer->GetDesc(&desc);

  D3D11_MAPPED_SUBRESOURCE ms;
  _cptr_context->Map(cptr_buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
  memcpy(ms.pData, data_ptr, desc.ByteWidth);
  _cptr_context->Unmap(cptr_buffer.Get(), NULL);
}

} // namespace ms