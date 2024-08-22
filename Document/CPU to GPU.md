# CPU 코드 > GPU 코드

## Append Consum StrucrtuedBuffer
Uniform Grid는 다음 3단계의 작업을 한다.

1. 새로운 Geometry Cell로 이동한 Fluid Particle들을 찾는다.
2. Geometry Cell마다 어떤 Fluid Particle이 들어있는지 업데이트한다.
3. Fluid Particle마다 Neighbor Fluid Particle들의 정보를 업데이트한다.

이 때, AppendStructuredBuffer와 ConsumeStructuredBuffer를 활용하여 2단계에서 필요한 만큼만 작업을 하게 하였다.

```hlsl
//find_changed_GCFPT_ID_CS.hlsl (1단계)
AppendStructuredBuffer<Changed_GCFPT_ID_Data> changed_GCFPT_ID_buffer : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 DTID : SV_DispatchThreadID)
{
  //...
  if (prev_GCFPT_id.gc_index != cur_gc_index)
  {
    //...
    changed_GCFPT_ID_buffer.Append(data);
  }
  //...
}
```

```hlsl
//update_GCFPT_CS.hlsl (2단계)
cbuffer Constant_Buffer : register(b0)
{
  uint consume_buffer_count;
}

ConsumeStructuredBuffer<Changed_GCFPT_ID_Data>  changed_GCFPT_ID_buffer : register(u3);

[numthreads(1, 1, 1)]
void main()
{
  for (int i = 0; i < consume_buffer_count; ++i)
  {
    const Changed_GCFPT_ID_Data changed_data = changed_GCFPT_ID_buffer.Consume();
    //...
  }
}
```

## Structured Buffer 크기 한계 문제

**[문제사항]**

CPU 코드에 다음과 같은 자료구조가 있다.

![CPU Data Structure](https://github.com/user-attachments/assets/f988fdee-23f7-436c-8dbb-f538ade96656)

Neighbor_Informations 구조체 내부의 배열들은 neighbor particle의 개수만큼 데이터를 가지게 된다.

이 자료구조를 GPU로 옮기기 위해 Neighbor_Informations_GPU의 StructuredBuffer를 사용하였다.

```cpp
inline constexpr size_t estimated_neighbor = 200;

struct Neighbor_Informations_GPU
{
  UINT    indexes[estimated_neighbor];           //800
  Vector3 translate_vectors[estimated_neighbor]; //2400
  float   distances[estimated_neighbor];         //800
  UINT    num_neighbor = 0;                      // 4
};
```

estimated_neighbor 200은 CPU 코드로 확인해본 결과를 토대로 판단하였다.

하지만 StructuredBuffer의 Type으로 주어지는 구조체는 최대 크기가 2048Byte까지여서 위와 같은 방법을 사용할 수 없다.

**[해결]**

따라서 index texture, tvec texture, distance texture, neighbor_count_buffer로 나누어서 자료구조를 옮겼다.

![GPU data structure](https://github.com/user-attachments/assets/9af88d40-e654-4539-8f77-90c2212ac9fc)


## DXGI FORMAT & UAV 문제

**[문제사항]**

tvec texture을 만들기 위해 DXGI_FORMAT_R32G32B32_FLOAT format으로 Texutre2D를 만들고 UAV를 만들려고 하였으나 오류가 발생하였다.

```
X4596	typed UAV stores must write all declared components.	
```

**[해결]**

확인해보니, DXGI_FORMAT_R32G32B32_FLOAT는 UAV가 지원되지 않는 타입이기 때문이였고 이를 해결하기 위해  DXGI_FORMAT_R32G32B32A32_FLOAT format으로 수정하였다.

## Parallel Reduction Algorithm
CPU 코드 중 어떻게 병렬화 해야 될지 고민이 되는 코드들이 있었다.
1. 배열의 최대값과 index를 찾는 문제
2. 배열의 모든 원소의 합을 구하는 문제

이를 병렬화 하기 위해 parallel reduction algorithm을 사용하였다.

parallel reduction algorithm은 데이터 집합을 재귀적으로 축소하여 최종 결과를 얻는 방식으로 작동한다. 

![paralle reduction algorithm](https://github.com/user-attachments/assets/373ca5ce-1b43-470c-9e96-ccfc34f1f46e)

어떤 단계에서 2N개의 element가 있다면, N개의 thread들이 병렬적으로 binary operation을 수행해서, N개의 결과를 만들어내고 그 다음단계의 input이 되는 과정을 반복하여 최종 결과를 얻는다.

아래는 parallel reduction algorithm을 사용해서 최대값을 찾는 알고리즘의 부분적인 hlsl code와 cpp code이다.

```
//hlsl code

groupshared float shared_value[NUM_THREAD];

//...

  if (data_index < g_num_data)
    shared_value[Gindex] = input[data_index];
  else
    shared_value[Gindex] = g_float_min;
  
  GroupMemoryBarrierWithGroupSync();

//...

  for (uint stride = NUM_THREAD / 2; 0 < stride; stride /= 2)
  {
    if (Gindex < stride)
      shared_value[Gindex] = max(shared_value[Gindex], shared_value[Gindex + stride]);
    
    GroupMemoryBarrierWithGroupSync();
  }
  
  if (Gindex == 0)
    output[Gid.x] = shared_value[0];
```

```cpp
//cpp code  
  //...
  while (true)
  {
    //...
    const auto num_output = Utility::ceil(num_input, num_thread);
    cptr_context->Dispatch(num_output, 1, 1);

    //...    

    if (num_output == 1)
      break;

    num_input = num_output;
    std::swap(input_buffer, output_buffer);
  }
```

parallel reduction algorithm을 사용하여 개발한 코드들은 Google Test FrameWork를 사용하여 test 코드를 작성하여 단위 테스트 검증을 진행하였다.

![paralle reduction algorithm test](https://github.com/user-attachments/assets/522c5b77-10cf-4664-96d1-b36dcb3e309e)


</br></br></br>