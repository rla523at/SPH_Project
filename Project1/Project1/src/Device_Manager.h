#pragma once
#include "Buffer_Set.h"

#include "../../_lib/_header/msexception/Exception.h"
#include <algorithm>
#include <d3d11.h>
#include <utility>
#include <vector>

/*  Abbreviation

    DM      : Device Manager

    ACB`    : Append Consume Buffer
    CONB    : Constant Buffer
    ICONB   : Immutable Constant Buffer
    STRB    : Structured Buffer
    ISTRB   : Immtuable Structured Buffer
    STGB    : Staging Buffer
    DIAB    : Dispatch Indirect Argument Buffer

    RWBS    : Read/Write Buffer Set
    RBS     : Read Buffer Set
    ACBS    : Append Consume Buffer Set

    SRV     : Shader Resource View
    UAV     : Unordered Acess View
*/

inline constexpr UINT g_num_max_view = 7;

namespace ms
{
class Device_Manager
{
public:
  Device_Manager(void);

  Device_Manager(
    const HWND main_window,
    const int  num_row_pixel,
    const int  num_col_pixel,
    const bool do_use_Vsync);

public:
  template <typename T>
  Append_Conssume_Buffer_Set create_ACBS(const UINT num_data) const;

  template <typename T>
  ComPtr<ID3D11Buffer> create_ICONB(T* init_data_ptr) const;

  template <typename T>
  ComPtr<ID3D11Buffer> create_ISTRB(const UINT num_data, const T* init_data_ptr) const;

  template <typename T>
  ComPtr<ID3D11Buffer> create_STRB(const UINT num_data, const T* init_data_ptr = nullptr) const;

  template <typename T>
  Read_Write_Buffer_Set create_STRB_RWBS(const UINT num_data, const T* init_data_ptr = nullptr) const;

  template <typename T>
  Read_Buffer_Set create_ISTRB_RBS(const UINT num_data, const T* init_data_ptr) const;

  template <typename T>
  ComPtr<ID3D11Texture2D> create_texutre_2D(
    const UINT        width,
    const UINT        height,
    const DXGI_FORMAT format,
    T*                init_data_ptr = nullptr) const;

  template <typename T>
  ComPtr<ID3D11Texture2D> create_texutre_2D_immutable(
    const UINT        width,
    const UINT        height,
    const DXGI_FORMAT format,
    T*                init_data_ptr = nullptr) const;

  template <typename T>
  void read(T* dest_ptr, const ComPtr<ID3D11Buffer> cptr_buffer, const UINT num_read_data = -1) const;

  template <typename T>
  void read_STGB(T* dest_ptr, const ComPtr<ID3D11Buffer> cptr_STGB, const UINT num_read_data) const;

  template <typename T>
  std::vector<T> read(const ComPtr<ID3D11Buffer> cptr_buffer) const;

  template <typename T>
  std::vector<std::vector<T>> read(const ComPtr<ID3D11Texture2D> cptr_staging_texture) const;

  template <typename T>
  std::vector<T> read(
    const ComPtr<ID3D11Buffer> cptr_staging_buffer,
    const ComPtr<ID3D11Buffer> cptr_target_buffer) const;

  template <typename T>
  std::vector<std::vector<T>> read(
    const ComPtr<ID3D11Texture2D> cptr_staging_texture,
    const ComPtr<ID3D11Texture2D> cptr_target_texture) const;

  template <typename T>
  T read_front(const ComPtr<ID3D11Buffer> cptr_buffer) const;

  template <typename T>
  T read_front_STGB(const ComPtr<ID3D11Buffer> cptr_STGB) const;

  template <typename T>
  void write(const T* data_ptr, const ComPtr<ID3D11Buffer> cptr_staging_buffer) const;

  template <typename T>
  void write(
    const T*                   data_ptr,
    const ComPtr<ID3D11Buffer> cptr_staging_buffer,
    const ComPtr<ID3D11Buffer> cptr_target_buffer) const;

public:
  void copy_front(const ComPtr<ID3D11Buffer>& cptr_src_buffer, const ComPtr<ID3D11Buffer>& cptr_dest_buffer, const UINT copy_byte) const;
  void copy_struct_count(const ComPtr<ID3D11Buffer>& cptr_src_buffer, const ComPtr<ID3D11UnorderedAccessView>& cptr_ACB_UAV) const;

  void create_VS_and_IL(
    const wchar_t*                               file_name,
    ComPtr<ID3D11VertexShader>&                  cptr_VS,
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& input_element_descs,
    ComPtr<ID3D11InputLayout>&                   cptr_IL) const;

  ComPtr<ID3D11ComputeShader> create_CS(const wchar_t* file_name) const;
  void                        create_PS(const wchar_t* file_name, ComPtr<ID3D11PixelShader>& cptr_PS) const;

  ComPtr<ID3D11ShaderResourceView>  create_SRV(ID3D11Resource* resource_ptr) const;
  ComPtr<ID3D11ShaderResourceView>  create_SRV(const ComPtr<ID3D11Buffer> cptr_buffer) const;
  ComPtr<ID3D11UnorderedAccessView> create_UAV(ID3D11Resource* resource_ptr) const;
  ComPtr<ID3D11UnorderedAccessView> create_UAV(const ComPtr<ID3D11Buffer> cptr_buffer) const;
  ComPtr<ID3D11UnorderedAccessView> create_AC_UAV(ID3D11Resource* resource_ptr, const UINT num_data) const;

  Read_Write_Buffer_Set   create_DIAB_RWBS(void) const;
  ComPtr<ID3D11Buffer>    create_CONB(const UINT data_size) const;
  ComPtr<ID3D11Buffer>    create_STGB_read(const UINT data_size) const;
  ComPtr<ID3D11Buffer>    create_STGB_read(const ComPtr<ID3D11Buffer> cptr_target_buffer) const;
  ComPtr<ID3D11Buffer>    create_STGB_write(const ComPtr<ID3D11Buffer> cptr_target_buffer) const;
  ComPtr<ID3D11Texture2D> create_staging_texture_read(const ComPtr<ID3D11Texture2D> cptr_target_buffer) const;
  ComPtr<ID3D11Query>     create_time_stamp_query(void) const;
  ComPtr<ID3D11Query>     create_time_stamp_disjoint_query(void) const;

public:
  void clear_SRVs_VS(void);
  
  void set_CONB(const UINT slot, const ComPtr<ID3D11Buffer>& cptr_CONB);
  void set_SRV(const UINT slot, const ComPtr<ID3D11ShaderResourceView>& cptr_SRV);
  void set_UAV(const UINT slot, const ComPtr<ID3D11UnorderedAccessView>& cptr_UAV);

public:
  void bind_OM_RTV_and_DSV(void) const;
  void bind_back_buffer_to_CS_UAV(void) const;
  void bind_CONBs_to_CS(const UINT start_slot, const UINT num_CONB) const;
  void bind_SRVs_to_CS(const UINT start_slot, const UINT num_SRV) const;
  void bind_UAVs_to_CS(const UINT start_slot, const UINT num_UAV) const;
  void copy_back_buffer(const ComPtr<ID3D11Texture2D> cptr_2D_texture) const;
  void create_texture_like_back_buffer(ComPtr<ID3D11Texture2D>& cptr_2D_texture) const;
  void CS_barrier(void) const;
  void dispatch(const UINT num_group_x, const UINT num_group_y, const UINT num_group_z) const;
  void dispatch_indirect(const ComPtr<ID3D11Buffer>& DIAB) const;

  void set_shader_CS(const ComPtr<ID3D11ComputeShader>& cptr_CS) const;

  void unbind_OM_RTV_and_DSV(void) const;
  void unbind_CS_UAV(void) const;

  UINT read_count(const ComPtr<ID3D11UnorderedAccessView> UAV) const;

  UINT num_data(const ComPtr<ID3D11Buffer> cptr_buffer, const UINT data_size) const;

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

  ID3D11Buffer*              _CONBs[g_num_max_view];
  ID3D11ShaderResourceView*  _SRVs[g_num_max_view];
  ID3D11UnorderedAccessView* _UAVs[g_num_max_view];
};

} // namespace ms

// template definition
namespace ms
{

template <typename T>
inline Append_Conssume_Buffer_Set Device_Manager::create_ACBS(const UINT num_data) const
{
  Append_Conssume_Buffer_Set result = {};

  result.cptr_buffer = this->create_STRB<T>(num_data);
  result.cptr_UAV    = this->create_AC_UAV(result.cptr_buffer.Get(), num_data);

  return result;
}

template <typename T>
ComPtr<ID3D11Buffer> Device_Manager::create_ICONB(T* init_data_ptr) const
{
  const auto data_size = sizeof(T);

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(data_size);
  desc.Usage               = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags      = NULL;
  desc.MiscFlags           = NULL;
  desc.StructureByteStride = NULL;

  D3D11_SUBRESOURCE_DATA init_data = {};
  init_data.pSysMem                = init_data_ptr;
  init_data.SysMemPitch            = 0;
  init_data.SysMemSlicePitch       = 0;

  ComPtr<ID3D11Buffer> cptr_constant_buffer;

  const auto result = _cptr_device->CreateBuffer(&desc, &init_data, cptr_constant_buffer.GetAddressOf());
  REQUIRE(!FAILED(result), "structured buffer creation should succeed");

  return cptr_constant_buffer;
}

template <typename T>
ComPtr<ID3D11Buffer> Device_Manager::create_STRB(const UINT num_data, const T* init_data_ptr) const
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
ComPtr<ID3D11Buffer> Device_Manager::create_ISTRB(const UINT num_data, const T* init_data_ptr) const
{
  const auto data_size = sizeof(T);

  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = static_cast<UINT>(num_data * data_size);
  desc.Usage               = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags      = NULL;
  desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.StructureByteStride = data_size;

  D3D11_SUBRESOURCE_DATA init_data = {};
  init_data.pSysMem                = init_data_ptr;
  init_data.SysMemPitch            = 0;
  init_data.SysMemSlicePitch       = 0;

  ComPtr<ID3D11Buffer> cptr_structured_buffer;

  const auto result = _cptr_device->CreateBuffer(&desc, &init_data, cptr_structured_buffer.GetAddressOf());
  REQUIRE(!FAILED(result), "immutable structured buffer creation should succeed");

  return cptr_structured_buffer;
}

template <typename T>
Read_Write_Buffer_Set Device_Manager::create_STRB_RWBS(const UINT num_data, const T* init_data_ptr) const
{
  Read_Write_Buffer_Set result = {};
  result.cptr_buffer           = this->create_STRB<T>(num_data, init_data_ptr);
  result.cptr_SRV              = this->create_SRV(result.cptr_buffer);
  result.cptr_UAV              = this->create_UAV(result.cptr_buffer);

  return result;
}

template <typename T>
inline Read_Buffer_Set Device_Manager::create_ISTRB_RBS(const UINT num_data, const T* init_data_ptr) const
{
  Read_Buffer_Set result = {};
  result.cptr_buffer     = this->create_ISTRB(num_data, init_data_ptr);
  result.cptr_SRV        = this->create_SRV(result.cptr_buffer);

  return result;
}

template <typename T>
ComPtr<ID3D11Texture2D> Device_Manager::create_texutre_2D(
  const UINT        width,
  const UINT        height,
  const DXGI_FORMAT format,
  T*                init_data_ptr) const
{
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = width;
  desc.Height               = height;
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = format;
  desc.SampleDesc.Count     = 1;
  desc.Usage                = D3D11_USAGE_DEFAULT;
  desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  desc.CPUAccessFlags       = NULL;
  desc.MiscFlags            = NULL;

  ComPtr<ID3D11Texture2D> cptr_texutre_2D;

  if (init_data_ptr == nullptr)
  {
    const auto result = _cptr_device->CreateTexture2D(&desc, nullptr, cptr_texutre_2D.GetAddressOf());
    REQUIRE(!FAILED(result), "texture 2D creation without initial data should succeed");
  }
  else
  {
    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem                = init_data_ptr;
    init_data.SysMemPitch            = width;
    init_data.SysMemSlicePitch       = 0;

    const auto result = _cptr_device->CreateTexture2D(&desc, &init_data, cptr_texutre_2D.GetAddressOf());
    REQUIRE(!FAILED(result), "texture 2D creation with initial data should succeed");
  }

  return cptr_texutre_2D;
}

template <typename T>
ComPtr<ID3D11Texture2D> Device_Manager::create_texutre_2D_immutable(
  const UINT        width,
  const UINT        height,
  const DXGI_FORMAT format,
  T*                init_data_ptr) const
{
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = width;
  desc.Height               = height;
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = format;
  desc.SampleDesc.Count     = 1;
  desc.Usage                = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags       = NULL;
  desc.MiscFlags            = NULL;

  D3D11_SUBRESOURCE_DATA init_data = {};
  init_data.pSysMem                = init_data_ptr;
  init_data.SysMemPitch            = width;
  init_data.SysMemSlicePitch       = 0;

  ComPtr<ID3D11Texture2D> cptr_texutre_2D;

  const auto result = _cptr_device->CreateTexture2D(&desc, &init_data, cptr_texutre_2D.GetAddressOf());
  REQUIRE(!FAILED(result), "immutable texture 2D creation should succeed");

  return cptr_texutre_2D;
}

template <typename T>
void Device_Manager::read(T* dest_ptr, const ComPtr<ID3D11Buffer> cptr_buffer, const UINT num_read_data) const
{
  D3D11_BUFFER_DESC desc;
  cptr_buffer->GetDesc(&desc);

  constexpr UINT data_size = sizeof(T);
  const auto     num_data  = (std::min)(desc.ByteWidth / data_size, num_read_data);

  ComPtr<ID3D11Buffer> cptr_staging_buffer = cptr_buffer;
  if (desc.Usage != D3D11_USAGE_STAGING)
  {
    cptr_staging_buffer = this->create_STGB_read(cptr_buffer);
    _cptr_context->CopyResource(cptr_staging_buffer.Get(), cptr_buffer.Get());
  }

  D3D11_MAPPED_SUBRESOURCE ms;
  _cptr_context->Map(cptr_staging_buffer.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
  memcpy(dest_ptr, ms.pData, num_data * data_size);
  _cptr_context->Unmap(cptr_staging_buffer.Get(), NULL);
}

template <typename T>
inline void Device_Manager::read_STGB(T* dest_ptr, const ComPtr<ID3D11Buffer> cptr_STGB, const UINT num_read_data) const
{
  constexpr UINT data_size = sizeof(T);

  D3D11_MAPPED_SUBRESOURCE ms;
  _cptr_context->Map(cptr_STGB.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
  memcpy(dest_ptr, ms.pData, num_read_data * data_size);
  _cptr_context->Unmap(cptr_STGB.Get(), NULL);
}

template <typename T>
std::vector<T> Device_Manager::read(const ComPtr<ID3D11Buffer> cptr_buffer) const
{
  D3D11_BUFFER_DESC desc;
  cptr_buffer->GetDesc(&desc);

  constexpr auto data_size = sizeof(T);
  const auto     num_data  = desc.ByteWidth / data_size;

  std::vector<T> datas(num_data);
  this->read(datas.data(), cptr_buffer);

  return datas;
}

template <typename T>
std::vector<std::vector<T>> Device_Manager::read(const ComPtr<ID3D11Texture2D> staging_texture) const
{
  D3D11_TEXTURE2D_DESC desc;
  staging_texture->GetDesc(&desc);

  const UINT width  = desc.Width;
  const UINT height = desc.Height;

  std::vector<std::vector<T>> datas(height, std::vector<T>(width));

  D3D11_MAPPED_SUBRESOURCE ms;

  {
    const auto result = _cptr_context->Map(staging_texture.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
    REQUIRE(!FAILED(result), "Map should be succeed");
  }

  BYTE* source_ptr = reinterpret_cast<BYTE*>(ms.pData);
  for (UINT y = 0; y < height; ++y)
  {
    T* dest_ptr = datas[y].data();
    memcpy(dest_ptr, source_ptr, width * sizeof(T));
    source_ptr += ms.RowPitch;
  }

  _cptr_context->Unmap(staging_texture.Get(), NULL);

  return datas;
}

template <typename T>
inline T Device_Manager::read_front(const ComPtr<ID3D11Buffer> cptr_buffer) const
{
  T data = {};
  read(&data, cptr_buffer, 1);
  return data;
}

template <typename T>
inline T Device_Manager::read_front_STGB(const ComPtr<ID3D11Buffer> cptr_STGB) const
{
  T data = {};
  read_STGB(&data, cptr_STGB, 1);
  return data;
}

template <typename T>
std::vector<T> Device_Manager::read(
  const ComPtr<ID3D11Buffer> cptr_staging_buffer,
  const ComPtr<ID3D11Buffer> cptr_target_buffer) const
{
  _cptr_context->CopyResource(cptr_staging_buffer.Get(), cptr_target_buffer.Get());
  return this->read<T>(cptr_staging_buffer);
}

template <typename T>
std::vector<std::vector<T>> Device_Manager::read(
  const ComPtr<ID3D11Texture2D> cptr_staging_texture,
  const ComPtr<ID3D11Texture2D> cptr_target_texture) const
{
  _cptr_context->CopyResource(cptr_staging_texture.Get(), cptr_target_texture.Get());
  return this->read<T>(cptr_staging_texture);
}

template <typename T>
void Device_Manager::write(const T* data_ptr, const ComPtr<ID3D11Buffer> cptr_buffer) const
{
  D3D11_BUFFER_DESC desc;
  cptr_buffer->GetDesc(&desc);

  const bool is_not_writable = desc.Usage != D3D11_USAGE_DYNAMIC && desc.Usage != D3D11_USAGE_STAGING;

  auto cptr_writable_buffer = cptr_buffer;
  auto map_type             = D3D11_MAP_WRITE;

  if (is_not_writable)
    cptr_writable_buffer = this->create_STGB_write(cptr_buffer);
  else
  {
    if (desc.Usage == D3D11_USAGE_DYNAMIC)
      map_type = D3D11_MAP_WRITE_DISCARD;
  }

  D3D11_MAPPED_SUBRESOURCE ms;

  const auto result = _cptr_context->Map(cptr_writable_buffer.Get(), NULL, map_type, NULL, &ms);
  REQUIRE(!FAILED(result), "Map should be succeed");

  BYTE* dest_ptr = reinterpret_cast<BYTE*>(ms.pData);

  memcpy(ms.pData, data_ptr, desc.ByteWidth);
  _cptr_context->Unmap(cptr_writable_buffer.Get(), NULL);

  if (is_not_writable)
    _cptr_context->CopyResource(cptr_buffer.Get(), cptr_writable_buffer.Get());
}

template <typename T>
void Device_Manager::write(
  const T*                   data_ptr,
  const ComPtr<ID3D11Buffer> cptr_staging_buffer,
  const ComPtr<ID3D11Buffer> cptr_target_buffer) const
{
  this->write(data_ptr, cptr_staging_buffer);
  _cptr_context->CopyResource(cptr_target_buffer.Get(), cptr_staging_buffer.Get());
}

} // namespace ms