#include "Utility.h"

#include "Device_Manager.h"

#include "../../_lib/_header/msexception/Exception.h"

namespace ms
{

UINT Utility::ceil(const UINT numerator, const UINT denominator)
{
  return (numerator + denominator - 1) / denominator;
}

void Utility::init_for_utility_using_GPU(const Device_Manager& DM)
{
  // Test시 DM이 사라졌을 떄 문제가 된다..
  // if (Utility::is_initialized())
  //   return;

  _DM_ptr = &DM;

  _cptr_uint_CB = _DM_ptr->create_CONB(16);

  // if (_cptr_find_max_value_float_CS == nullptr) // DM이 사라졌을 때 문제가 된다. DM이 바뀌면 컴파일도 다시해야되는듯..?
  _cptr_find_max_value_float_CS = _DM_ptr->create_CS(L"hlsl/find_max_value_CS.hlsl");

  // if (_cptr_find_max_index_float_CS == nullptr)
  _cptr_find_max_index_float_CS = _DM_ptr->create_CS(L"hlsl/find_max_index_CS.hlsl");

  // if (_cptr_find_max_index_float_256_CS == nullptr)
  _cptr_find_max_index_float_256_CS = _DM_ptr->create_CS(L"hlsl/find_max_index_256_CS.hlsl");

  //timer
  _cptr_disjoint_query = _DM_ptr->create_time_stamp_disjoint_query();
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
    _DM_ptr->write(&num_input, _cptr_uint_CB);

    const auto input_SRV = _DM_ptr->create_SRV(input_buffer.Get());

    const auto output_UAV = _DM_ptr->create_UAV(output_buffer.Get());

    const auto cptr_context = _DM_ptr->context_cptr();
    cptr_context->CSSetConstantBuffers(0, 1, _cptr_uint_CB.GetAddressOf());
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

  _DM_ptr->write(&num_value, _cptr_uint_CB);

  const auto input_SRV = _DM_ptr->create_SRV(float_value_buffer);

  const auto num_output    = Utility::ceil(num_value, num_thread);
  auto       output_buffer = _DM_ptr->create_STRB<UINT>(num_output);
  const auto output_UAV    = _DM_ptr->create_UAV(output_buffer);

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, 1, _cptr_uint_CB.GetAddressOf());
  cptr_context->CSSetShaderResources(0, 1, input_SRV.GetAddressOf());
  cptr_context->CSSetUnorderedAccessViews(0, 1, output_UAV.GetAddressOf(), nullptr);
  cptr_context->CSSetShader(_cptr_find_max_index_float_256_CS.Get(), nullptr, NULL);

  cptr_context->Dispatch(num_output, 1, 1);

  _DM_ptr->CS_barrier();

  return output_buffer;
}

void Utility::init_GPU_timer(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  cptr_context->Begin(_cptr_disjoint_query.Get());
}

void Utility::set_time_point(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  _cptr_start_querys.push_back(_DM_ptr->create_time_stamp_query());

  cptr_context->End(_cptr_start_querys.back().Get());
}

GPU_DT_Info Utility::make_GPU_dt_info(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  GPU_DT_Info result;

  result.start_point = _cptr_start_querys.back();
  _cptr_start_querys.pop_back();

  result.end_point = _DM_ptr->create_time_stamp_query();
  cptr_context->End(result.end_point.Get());

  return result;
}

void Utility::finialize_GPU_timer(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  cptr_context->End(_cptr_disjoint_query.Get());

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
  while (cptr_context->GetData(_cptr_disjoint_query.Get(), &disjoint_data, sizeof(disjoint_data), 0) != S_OK)
  {
  };

  if (disjoint_data.Disjoint)
  {
    std::cout << "GPU Timer has an error.\n";
    _GPU_frequency = 0.0f;
    return;
  }

  _GPU_frequency = static_cast<float>(disjoint_data.Frequency);
}

float Utility::cal_dt(const GPU_DT_Info& info)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  UINT64 start_time = 0;
  while (cptr_context->GetData(info.start_point.Get(), &start_time, sizeof(start_time), 0) != S_OK)
  {
  };

  UINT64 end_time = 0;
  while (cptr_context->GetData(info.end_point.Get(), &end_time, sizeof(end_time), 0) != S_OK)
  {
  };

  const auto delta = end_time - start_time;

  return (delta / _GPU_frequency) * 1000.0f; //millisecond 출력
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

  _DM_ptr->write(&num_input_index, _cptr_uint_CB);

  const auto value_SRV       = _DM_ptr->create_SRV(value_buffer);
  const auto input_index_SRV = _DM_ptr->create_SRV(input_index_buffer);
  const auto output_UAV      = _DM_ptr->create_UAV(output_index_buffer);

  ID3D11Buffer* CBs[num_CB] = {
    _cptr_uint_CB.Get(),
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