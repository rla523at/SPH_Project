#pragma once
#include "Abbreviation.h"

#include <d3d11.h>

/*  Abbreviation
    SRV       : Shader Resource View
    UAV       : Unordered Acess View
*/

namespace ms
{

struct Read_Buffer_Set
{
  ComPtr<ID3D11Buffer>             cptr_buffer;
  ComPtr<ID3D11ShaderResourceView> cptr_SRV;
};

struct Read_Write_Buffer_Set : public Read_Buffer_Set
{
  ComPtr<ID3D11UnorderedAccessView> cptr_UAV;
};

struct Append_Conssume_Buffer_Set
{
  ComPtr<ID3D11Buffer>              cptr_buffer;
  ComPtr<ID3D11UnorderedAccessView> cptr_UAV;
};

} // namespace ms