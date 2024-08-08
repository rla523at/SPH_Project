#pragma once
#include "Abbreviation.h"

#include <d3d11.h>
#include <vector>

namespace ms
{
class Device_Manager;
}

namespace ms
{
struct GPU_DT_Info
{
  ComPtr<ID3D11Query> start_point;
  ComPtr<ID3D11Query> end_point;
};
} // namespace ms

namespace ms
{

class Utility
{
public:
  static UINT ceil(const UINT numerator, const UINT denominator);

  // GPU CODE
  static void                 init_for_utility_using_GPU(const Device_Manager& DM);
  static ComPtr<ID3D11Buffer> find_max_value_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);

  static ComPtr<ID3D11Buffer> find_max_value_float_opt(
    const ComPtr<ID3D11Buffer> value_buffer,
    const ComPtr<ID3D11Buffer> intermediate_buffer,
    const UINT                 num_value);

  static ComPtr<ID3D11Buffer> find_max_index_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);
  static ComPtr<ID3D11Buffer> find_max_index_float_256(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value);

  // GPU performance
  static void        init_GPU_timer(void);
  static void        set_time_point(void);
  static GPU_DT_Info make_GPU_dt_info(void);
  static void        finialize_GPU_timer(void);
  static float       cal_dt(const GPU_DT_Info& info);

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

  static inline ComPtr<ID3D11ComputeShader> _cptr_find_max_value_float_CS;
  static inline ComPtr<ID3D11ComputeShader> _cptr_find_max_index_float_CS;
  static inline ComPtr<ID3D11ComputeShader> _cptr_find_max_index_float_256_CS;

  //time stamp
  static inline std::vector<ComPtr<ID3D11Query>> _cptr_start_querys;
  static inline ComPtr<ID3D11Query>              _cptr_disjoint_query;
  static inline float                            _GPU_frequency;
};

} // namespace ms