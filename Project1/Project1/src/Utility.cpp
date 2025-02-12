#include "Utility.h"

#include "Device_Manager.h"

#include "../../_lib/_header/msexception/Exception.h"

// Data Struct
namespace ms
{

struct UINT2
{
  UINT val1 = 0;
  UINT val2 = 0;
};

} // namespace ms

namespace ms
{

UINT Utility::ceil(const UINT numerator, const UINT denominator)
{
  return (numerator + denominator - 1) / denominator;
}

void Utility::init_for_utility_using_GPU(Device_Manager& DM)
{
  // Test시 DM이 사라졌을 떄 문제가 된다..
  // if (Utility::is_initialized())
  //   return;

  _DM_ptr = &DM;

  _cptr_uint4_CONB = _DM_ptr->create_CONB(16);

  _dispatch_indirect_args_RWBS = _DM_ptr->create_DIAB_RWBS();

  // if (_cptr_find_max_value_float_CS == nullptr) // DM이 사라졌을 때 문제가 된다. DM이 바뀌면 컴파일도 다시해야되는듯..?
  _cptr_find_max_value_float_CS = _DM_ptr->create_CS(L"hlsl/find_max_value_CS.hlsl");

  // if (_cptr_find_max_index_float_CS == nullptr)
  _cptr_find_max_index_float_CS = _DM_ptr->create_CS(L"hlsl/find_max_index_CS.hlsl");

  // if (_cptr_find_max_index_float_256_CS == nullptr)
  _cptr_find_max_index_float_256_CS = _DM_ptr->create_CS(L"hlsl/find_max_index_256_CS.hlsl");

  _cptr_update_indirect_args_from_structure_count_CS = _DM_ptr->create_CS(L"hlsl/create_indirect_args_from_structure_count_CS.hlsl");

  // timer
  _cptr_start_query2    = _DM_ptr->create_time_stamp_query();
  _cptr_end_query2      = _DM_ptr->create_time_stamp_query();
  _cptr_disjoint_query2 = _DM_ptr->create_time_stamp_disjoint_query();
}

const Read_Write_Buffer_Set& Utility::get_indirect_args_from_struct_count(
  const ComPtr<ID3D11UnorderedAccessView>& cptr_ACB_UAV,
  const UINT                        num_thread)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  _DM_ptr->write(&num_thread, _cptr_uint4_CONB);
  cptr_context->CopyStructureCount(_cptr_uint4_CONB.Get(), 4, cptr_ACB_UAV.Get());

  _DM_ptr->set_CONB(0, _cptr_uint4_CONB);
  _DM_ptr->set_UAV(0, _dispatch_indirect_args_RWBS.cptr_UAV);

  _DM_ptr->bind_CONBs_to_CS(0, 1);
  _DM_ptr->bind_UAVs_to_CS(0, 1);

  _DM_ptr->set_shader_CS(_cptr_update_indirect_args_from_structure_count_CS);

  _DM_ptr->dispatch(1, 1, 1);

  return _dispatch_indirect_args_RWBS;
}

ComPtr<ID3D11Buffer> Utility::find_max_value_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value)
{
  constexpr UINT num_thread = 256;

  const auto intermediate_buffer = _DM_ptr->create_STRB<float>(Utility::ceil(num_value, num_thread));

  return Utility::find_max_value_float_opt(value_buffer, intermediate_buffer, num_value);
}

ComPtr<ID3D11Buffer> Utility::find_max_value_float_opt(
  const ComPtr<ID3D11Buffer> value_buffer,
  const ComPtr<ID3D11Buffer> intermediate_buffer,
  const UINT                 num_value)
{
  constexpr UINT num_thread = 256;

  auto num_input     = num_value;
  auto input_buffer  = value_buffer;
  auto output_buffer = intermediate_buffer;

  while (true)
  {
    _DM_ptr->write(&num_input, _cptr_uint4_CONB);

    const auto input_SRV = _DM_ptr->create_SRV(input_buffer.Get());

    const auto output_UAV = _DM_ptr->create_UAV(output_buffer.Get());

    const auto cptr_context = _DM_ptr->context_cptr();
    cptr_context->CSSetConstantBuffers(0, 1, _cptr_uint4_CONB.GetAddressOf());
    cptr_context->CSSetShaderResources(0, 1, input_SRV.GetAddressOf());
    cptr_context->CSSetUnorderedAccessViews(0, 1, output_UAV.GetAddressOf(), nullptr);
    cptr_context->CSSetShader(_cptr_find_max_value_float_CS.Get(), nullptr, NULL);

    const auto num_output = Utility::ceil(num_input, num_thread);
    cptr_context->Dispatch(num_output, 1, 1);

    _DM_ptr->CS_barrier();

    if (num_output == 1)
      break;

    num_input = num_output;
    std::swap(input_buffer, output_buffer);
  }

  return output_buffer;
}

ComPtr<ID3D11Buffer> Utility::find_max_index_float(
  const ComPtr<ID3D11Buffer> value_buffer,
  const UINT                 num_value)
{
  REQUIRE(is_initialized(), "init_for_utility_using_GPU should call first");

  constexpr UINT num_thread = 256;

  auto cptr_index_buffer = Utility::find_max_index_float_256(value_buffer, num_value);
  auto num_index         = Utility::ceil(num_value, num_thread);

  if (num_index == 1)
    return cptr_index_buffer;

  const auto num_input_index         = num_index;
  const auto cptr_input_index_buffer = cptr_index_buffer;

  const auto num_output_index         = Utility::ceil(num_input_index, num_thread);
  const auto cptr_output_index_buffer = _DM_ptr->create_STRB<UINT>(num_output_index);

  _DM_ptr->context_cptr()->CSSetShader(_cptr_find_max_index_float_CS.Get(), nullptr, NULL);

  return Utility::find_max_index_float(
    value_buffer,
    cptr_input_index_buffer,
    cptr_output_index_buffer,
    num_input_index);
}

// 256개 단위 묶어서 그중 가장 큰 index를 저장한 buffer를 리턴한다.
ComPtr<ID3D11Buffer> Utility::find_max_index_float_256(const ComPtr<ID3D11Buffer> float_value_buffer, const UINT num_value)
{
  REQUIRE(is_initialized(), "init_for_utility_using_GPU should call first");

  constexpr UINT num_thread = 256;

  _DM_ptr->write(&num_value, _cptr_uint4_CONB);

  const auto input_SRV = _DM_ptr->create_SRV(float_value_buffer);

  const auto num_output    = Utility::ceil(num_value, num_thread);
  auto       output_buffer = _DM_ptr->create_STRB<UINT>(num_output);
  const auto output_UAV    = _DM_ptr->create_UAV(output_buffer);

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, 1, _cptr_uint4_CONB.GetAddressOf());
  cptr_context->CSSetShaderResources(0, 1, input_SRV.GetAddressOf());
  cptr_context->CSSetUnorderedAccessViews(0, 1, output_UAV.GetAddressOf(), nullptr);
  cptr_context->CSSetShader(_cptr_find_max_index_float_256_CS.Get(), nullptr, NULL);

  cptr_context->Dispatch(num_output, 1, 1);

  _DM_ptr->CS_barrier();

  return output_buffer;
}

void Utility::set_time_point(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  _cptr_disjoint_querys.push_back(_DM_ptr->create_time_stamp_disjoint_query());
  _cptr_start_querys.push_back(_DM_ptr->create_time_stamp_query());

  cptr_context->Begin(_cptr_disjoint_querys.back().Get());
  cptr_context->End(_cptr_start_querys.back().Get());
}

float Utility::cal_dt(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  const auto cptr_start_query = _cptr_start_querys.back();
  _cptr_start_querys.pop_back();

  const auto cptr_disjoint_query = _cptr_disjoint_querys.back();
  _cptr_disjoint_querys.pop_back();

  cptr_context->End(_cptr_end_query2.Get());
  cptr_context->End(cptr_disjoint_query.Get());

  // Wait for data to be available
  while (cptr_context->GetData(cptr_disjoint_query.Get(), NULL, 0, 0) == S_FALSE)
    ;

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
  cptr_context->GetData(cptr_disjoint_query.Get(), &disjoint_data, sizeof(disjoint_data), 0);

  REQUIRE_ALWAYS(!disjoint_data.Disjoint, "GPU Timer has an error.\n");

  UINT64 start_time = 0;
  UINT64 end_time   = 0;
  cptr_context->GetData(cptr_start_query.Get(), &start_time, sizeof(start_time), 0);
  cptr_context->GetData(_cptr_end_query2.Get(), &end_time, sizeof(end_time), 0);

  const auto delta     = end_time - start_time;
  const auto frequency = static_cast<float>(disjoint_data.Frequency);
  return (delta / frequency) * 1000.0f; // millisecond 출력
}

void Utility::set_time_point2(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  cptr_context->Begin(_cptr_disjoint_query2.Get());
  cptr_context->End(_cptr_start_query2.Get());
}

float Utility::cal_dt2(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  cptr_context->End(_cptr_end_query2.Get());
  cptr_context->End(_cptr_disjoint_query2.Get());

  // Wait for data to be available
  while (cptr_context->GetData(_cptr_disjoint_query2.Get(), NULL, 0, 0) == S_FALSE)
    ;

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
  cptr_context->GetData(_cptr_disjoint_query2.Get(), &disjoint_data, sizeof(disjoint_data), 0);

  if (disjoint_data.Disjoint)
  {
    std::cout << "GPU Timer has an error.\n";
    return -1.0f;
  }

  UINT64 start_time = 0;
  UINT64 end_time   = 0;
  cptr_context->GetData(_cptr_start_query2.Get(), &start_time, sizeof(start_time), 0);
  cptr_context->GetData(_cptr_end_query2.Get(), &end_time, sizeof(end_time), 0);

  const auto delta     = end_time - start_time;
  const auto frequency = static_cast<float>(disjoint_data.Frequency);
  return (delta / frequency) * 1000.0f; // millisecond 출력
}

bool Utility::is_initialized(void)
{
  return _DM_ptr != nullptr;
}

ComPtr<ID3D11Buffer> Utility::find_max_index_float(
  const ComPtr<ID3D11Buffer> value_buffer,
  const ComPtr<ID3D11Buffer> input_index_buffer,
  const ComPtr<ID3D11Buffer> output_index_buffer,
  const UINT                 num_input_index)
{
  REQUIRE(is_initialized(), "init_for_utility_using_GPU should call first");

  constexpr UINT num_thread = 256;
  constexpr UINT num_CB     = 1;
  constexpr UINT num_SRV    = 2;
  constexpr UINT num_UAV    = 1;

  _DM_ptr->write(&num_input_index, _cptr_uint4_CONB);

  const auto value_SRV       = _DM_ptr->create_SRV(value_buffer);
  const auto input_index_SRV = _DM_ptr->create_SRV(input_index_buffer);
  const auto output_UAV      = _DM_ptr->create_UAV(output_index_buffer);

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_uint4_CONB.Get(),
  };

  ID3D11ShaderResourceView* SRVs[num_SRV] = {
    value_SRV.Get(),
    input_index_SRV.Get(),
  };

  ID3D11UnorderedAccessView* UAVs[num_UAV] = {
    output_UAV.Get(),
  };

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, num_CB, CBs);
  cptr_context->CSSetShaderResources(0, num_SRV, SRVs);
  cptr_context->CSSetUnorderedAccessViews(0, num_UAV, UAVs, nullptr);

  const auto num_output = Utility::ceil(num_input_index, num_thread);
  cptr_context->Dispatch(num_output, 1, 1);

  _DM_ptr->CS_barrier();

  if (num_output == 1)
    return output_index_buffer;

  return Utility::find_max_index_float(
    value_buffer,
    output_index_buffer, // this depth output become next depth input
    input_index_buffer,
    num_output); // this depth output become next depth input
}

} // namespace ms