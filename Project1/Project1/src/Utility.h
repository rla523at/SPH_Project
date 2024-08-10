#pragma once
#include "Abbreviation.h"
#include "Buffer_Set.h"

#include <d3d11.h>
#include <vector>



// forward declaration
namespace ms
{
class Device_Manager;
}

// Utility class declaration
namespace ms
{

class Utility
{
public:
  static UINT ceil(const UINT numerator, const UINT denominator);

  // GPU CODE
  static const Read_Write_Buffer_Set& get_indirect_args_from_struct_count(
    const ComPtr<ID3D11UnorderedAccessView>& cptr_ACB_UAV,
    const UINT                        num_thread);

  static void init_for_utility_using_GPU(Device_Manager& DM);

  static ComPtr<ID3D11Buffer> find_max_value_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);

  static ComPtr<ID3D11Buffer> find_max_value_float_opt(
    const ComPtr<ID3D11Buffer> value_buffer,
    const ComPtr<ID3D11Buffer> intermediate_buffer,
    const UINT                 num_value);

  static ComPtr<ID3D11Buffer> find_max_index_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);
  static ComPtr<ID3D11Buffer> find_max_index_float_256(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);

  // GPU performance
  static void  set_time_point(void);
  static float cal_dt(void);

  // overlap을 허용하지 않는다고 가정하면, 바로바로 dt 계산할 수 있음.
  static void  set_time_point2(void);
  static float cal_dt2(void);

private:
  static bool is_initialized(void);

  static ComPtr<ID3D11Buffer> find_max_index_float(
    const ComPtr<ID3D11Buffer> value_buffer,
    const ComPtr<ID3D11Buffer> input_index_buffer,
    const ComPtr<ID3D11Buffer> output_index_buffer,
    const UINT                 num_input_index);

private:
  static inline Device_Manager* _DM_ptr = nullptr;

  static inline ComPtr<ID3D11Buffer>  _cptr_uint4_CONB;
  static inline Read_Write_Buffer_Set _dispatch_indirect_args_RWBS;

  static inline ComPtr<ID3D11ComputeShader> _cptr_find_max_value_float_CS;
  static inline ComPtr<ID3D11ComputeShader> _cptr_find_max_index_float_CS;
  static inline ComPtr<ID3D11ComputeShader> _cptr_find_max_index_float_256_CS;
  static inline ComPtr<ID3D11ComputeShader> _cptr_update_indirect_args_from_structure_count_CS;

  // time stamp
  static inline std::vector<ComPtr<ID3D11Query>> _cptr_start_querys;
  static inline std::vector<ComPtr<ID3D11Query>> _cptr_disjoint_querys;

  // time stamp2
  static inline ComPtr<ID3D11Query> _cptr_start_query2;
  static inline ComPtr<ID3D11Query> _cptr_end_query2;
  static inline ComPtr<ID3D11Query> _cptr_disjoint_query2;

  // static inline std::vector<ComPtr<ID3D11Query>> _cptr_start_querys2;
  // static inline std::vector<ComPtr<ID3D11Query>> _cptr_disjoint_querys2;
};

} // namespace ms