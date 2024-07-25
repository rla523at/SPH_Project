#include "Texture_Manager.h"

#include "../_lib/_header/msexception/Exception.h"
#include <DirectXTex.h>

namespace ms
{
void Texture_Manager::make_immutable_texture2D(
  const ComPtr<ID3D11Device> cptr_device,
  ID3D11Texture2D**          pp_texture_2D,
  const wchar_t*             texture_file_name)
{
  DirectX::ScratchImage image;
  DirectX::TexMetadata  meta_data;

  {
    const auto result = DirectX::LoadFromWICFile(texture_file_name, DirectX::WIC_FLAGS_NONE, &meta_data, image);
    REQUIRE(!FAILED(result), "load texture from WIC file should be success");
  }

  D3D11_TEXTURE2D_DESC texture_desc = {};
  texture_desc.Width                = static_cast<UINT>(meta_data.width);
  texture_desc.Height               = static_cast<UINT>(meta_data.height);
  texture_desc.MipLevels            = 1;
  texture_desc.ArraySize            = static_cast<UINT>(meta_data.arraySize);
  texture_desc.Format               = meta_data.format;
  texture_desc.SampleDesc.Count     = 1;
  texture_desc.Usage                = D3D11_USAGE_IMMUTABLE;
  texture_desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
  texture_desc.CPUAccessFlags       = NULL;
  texture_desc.MiscFlags            = NULL;

  D3D11_SUBRESOURCE_DATA init_data;
  init_data.pSysMem          = image.GetImage(0, 0, 0)->pixels;
  init_data.SysMemPitch      = static_cast<UINT>(image.GetImage(0, 0, 0)->rowPitch);
  init_data.SysMemSlicePitch = 0;

  cptr_device->CreateTexture2D(&texture_desc, &init_data, pp_texture_2D);
}
} // namespace ms