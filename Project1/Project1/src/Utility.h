#pragma once
#include "Abbreviation.h"

#include <d3d11.h>

namespace ms
{
class Device_Manager;
}

namespace ms
{

class Utility
{
public:
  static UINT ceil(const UINT numerator, const UINT denominator);

  //GPU CODE
  static void                 init_for_utility_using_GPU(const Device_Manager& DM);
  static ComPtr<ID3D11Buffer> find_max_value_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);
  static ComPtr<ID3D11Buffer> find_max_index_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);
  static ComPtr<ID3D11Buffer> find_max_index_float_256(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);

private:
  static bool is_initialized(void);

  static ComPtr<ID3D11Buffer> find_max_index_float(
    const ComPtr<ID3D11Buffer> value_buffer,
    const ComPtr<ID3D11Buffer> input_index_buffer,
    const ComPtr<ID3D11Buffer> output_index_buffer,
    const UINT                 num_input_index);

private:
  static inline const Device_Manager* _DM_ptr = nullptr;
  static inline ComPtr<ID3D11Buffer>  _cptr_uint_CB;
};

} // namespace ms