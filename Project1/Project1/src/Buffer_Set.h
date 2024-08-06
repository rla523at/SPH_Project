#pragma once
#include "Abbreviation.h"

#include <d3d11.h>

namespace ms
{
struct Buffer_Set
{
  ComPtr<ID3D11Buffer>              cptr_buffer;
  ComPtr<ID3D11ShaderResourceView>  cptr_SRV;
  ComPtr<ID3D11UnorderedAccessView> cptr_UAV;
};
} // namespace ms