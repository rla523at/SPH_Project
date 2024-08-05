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
  if (Utility::is_initialized())
    return;

  _DM_ptr = &DM;

  _cptr_uint_CB = _DM_ptr->create_CB(16);
}

ComPtr<ID3D11Buffer> Utility::find_max_value_float(const ComPtr<ID3D11Buffer> value_buffer, const UINT num_value)
{
  constexpr UINT num_thread = 256;

  auto num_input     = num_value;
  auto input_buffer  = value_buffer;
  auto output_buffer = _DM_ptr->create_structured_buffer<float>(Utility::ceil(num_value, num_thread));

  const auto CS = _DM_ptr->create_CS(L"hlsl/find_max_value_CS.hlsl");

  while (true)
  {
    _DM_ptr->upload(&num_input, _cptr_uint_CB);

    const auto input_SRV  = _DM_ptr->create_SRV(input_buffer.Get());

    const auto output_UAV = _DM_ptr->create_UAV(output_buffer.Get());

    const auto cptr_context = _DM_ptr->context_cptr();
    cptr_context->CSSetConstantBuffers(0, 1, _cptr_uint_CB.GetAddressOf());
    cptr_context->CSSetShaderResources(0, 1, input_SRV.GetAddressOf());
    cptr_context->CSSetUnorderedAccessViews(0, 1, output_UAV.GetAddressOf(), nullptr);
    cptr_context->CSSetShader(CS.Get(), nullptr, NULL);

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
  const auto cptr_output_index_buffer = _DM_ptr->create_structured_buffer<UINT>(num_output_index);

  const auto CS = _DM_ptr->create_CS(L"hlsl/find_max_index_CS.hlsl");
  _DM_ptr->context_cptr()->CSSetShader(CS.Get(), nullptr, NULL);

  return Utility::find_max_index_float(
    value_buffer,
    cptr_input_index_buffer,
    cptr_output_index_buffer,
    num_input_index);
}

//256개 단위 묶어서 그중 가장 큰 index를 저장한 buffer를 리턴한다.
ComPtr<ID3D11Buffer> Utility::find_max_index_float_256(const ComPtr<ID3D11Buffer> float_value_buffer, const UINT num_value)
{
  REQUIRE(is_initialized(), "init_for_utility_using_GPU should call first");

  constexpr UINT num_thread = 256;

  _DM_ptr->upload(&num_value, _cptr_uint_CB);

  const auto input_SRV = _DM_ptr->create_SRV(float_value_buffer);

  const auto num_output    = Utility::ceil(num_value, num_thread);
  auto       output_buffer = _DM_ptr->create_structured_buffer<UINT>(num_output);
  const auto output_UAV    = _DM_ptr->create_UAV(output_buffer);

  const auto CS = _DM_ptr->create_CS(L"hlsl/find_max_index_256_CS.hlsl");

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CSSetConstantBuffers(0, 1, _cptr_uint_CB.GetAddressOf());
  cptr_context->CSSetShaderResources(0, 1, input_SRV.GetAddressOf());
  cptr_context->CSSetUnorderedAccessViews(0, 1, output_UAV.GetAddressOf(), nullptr);
  cptr_context->CSSetShader(CS.Get(), nullptr, NULL);

  cptr_context->Dispatch(num_output, 1, 1);

  _DM_ptr->CS_barrier();

  return output_buffer;
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

  _DM_ptr->upload(&num_input_index, _cptr_uint_CB);

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
    output_index_buffer, //this depth output become next depth input
    input_index_buffer,
    num_output); //this depth output become next depth input
}

} // namespace ms