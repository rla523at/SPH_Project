#pragma once
#include <d3d11.h>
#include <wrl.h>

namespace ms
{
using Microsoft::WRL::ComPtr;
}

namespace ms
{

class Texture_Manager
{
public:
  static void make_immutable_texture2D(
    const ComPtr<ID3D11Device> cptr_device,
    ID3D11Texture2D**          pp_texture_2D,
    const wchar_t*             texture_file_name);

private:
  Texture_Manager(void) = default;
};

} // namespace ms