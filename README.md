</br></br></br>

# 2024.08.19
## 최적화 결과
```
_dt_avg_update                                              3.94271       ms
================================================================================
_dt_avg_update_neighborhood                                 1.09558       ms
_dt_avg_update_number_density                               0.172424      ms
_dt_avg_update_scailing_factor                              0.017785      ms
_dt_avg_init_fluid_acceleration                             0.270125      ms
_dt_avg_init_pressure_and_a_pressure                        0.00504521    ms
_dt_avg_copy_cur_pos_and_vel                                0.0187321     ms
_dt_avg_predict_velocity_and_position                       0.0388008     ms
_dt_avg_predict_density_error_and_update_pressure           0.558781      ms
_dt_avg_cal_max_density_error                               0             ms
_dt_avg_update_a_pressure                                   0.819914      ms
_dt_avg_apply_BC                                            0.00597577    ms
================================================================================

Neighborhood_Uniform_Grid_GPU Performance Analysis Result Per Frame
================================================================================
_dt_avg_update                                              1.04887       ms
================================================================================
_dt_avg_update_GCFP                                         0.0223155     ms
_dt_avg_rearrange_GCFP                                      0.0147891     ms
_dt_avg_update_nfp                                          0.854544      ms
================================================================================
```

## Neighborhood 코드 최적화 - update_nfp
update_nfp는 각 Particle마다 Neighbor Geometry Cell에 들어 있는 모든 Particle들에 대한 연산이 필요한 함수이다.

기존에는 Geometry Cell에 최대 64개 까지 Particle이 존재 할 수 있다는 사실에 기반하여 64개의 Thread를 사용하여 i번째 thread는 모든 Neighbor Geometry Cell에 들어있는 i번째 particle들에 대한 연산을 수행하였다. 

하지만 일반적으로 Neighbor Geometry Cell에는 64개 보다 훨씬 적은 Particle이 존재함으로 작업을 하지 않는 Thread가 많이 발생할 수 있다.

이를 개선하기 위해, i번째 thread는 i번 째 Neighbor Geometry Cell에 있는 모든 particle들에 대한 연산을 수행하도록 수정하였다.

warp 단위를 맞추기 위해 32개의 thread를 사용하였고, 대부분의 Cell이 27개의 neighbor geometry cell을 갖음으로 작업을 하지 않는 thread를 줄일 수 있을것이라고 판단하였다.

![update_nfp1](https://github.com/user-attachments/assets/fa8df285-cd85-4821-b94b-74ea6edecf5b)

개선전 대비 약 `5%` 계산시간이 감소하였다.

||개선전|개선후|
|---|---|---|
|Computation Time(ms)|0.899932|0.855475|

### 실패한 개선
InterLockAdd함수가 병목에 원인이 될 수 있다고 판단하여, local 변수를 활용하고 GroupMemoryBarrierWithGroupSync를 사용해 InterLockAdd함수를 제거하였다.

하지만 개선전에 비해 계산시간이 증가하였고, GroupMemoryBarrierWithGroupSync로 인해 오히려 더 성능이 떨어진것으로 보인다.

GroupMemoryBarrierWithGroupSync를 줄이는 추가적인 개선을 해보았지만 유의미한 성능 개선은 없었다.

![update_nfp2](https://github.com/user-attachments/assets/ea47f084-05b8-4742-aa97-7829c8ceeb59)

</br></br></br>

# 2024.08.16
## 결과

최종 최적화 결과는 다음과 같다.

```
PCISPH_GPU Performance Analysis Result per frame
================================================================================
_dt_avg_update                                              4.00479       ms
================================================================================
_dt_avg_update_neighborhood                                 1.14807       ms
_dt_avg_update_number_density                               0.173746      ms
_dt_avg_update_scailing_factor                              0.0178631     ms
_dt_avg_init_fluid_acceleration                             0.271192      ms
_dt_avg_init_pressure_and_a_pressure                        0.00498217    ms
_dt_avg_copy_cur_pos_and_vel                                0.0187285     ms
_dt_avg_predict_velocity_and_position                       0.0399128     ms
_dt_avg_predict_density_error_and_update_pressure           0.545056      ms
_dt_avg_cal_max_density_error                               0             ms
_dt_avg_update_a_pressure                                   0.814734      ms
_dt_avg_apply_BC                                            0.00592225    ms
================================================================================

Neighborhood_Uniform_Grid_GPU Performance Analysis Result Per Frame
================================================================================
_dt_avg_update                                              1.09833       ms
================================================================================
_dt_avg_update_GCFP                                         0.0222131     ms
_dt_avg_rearrange_GCFP                                      0.0146923     ms
_dt_avg_update_nfp                                          0.895872      ms
================================================================================
```

## PCISPH 코드 최적화 - init_fluid_acceleration
기존 코드는 한 thread당 neighbor particle개수 만큼 v_laplacian velocity를 계산하였다.

neighbor particle개수는 최대 200까지 될 수 있어 한 Thread의 연산부하가 너무 커 병렬화 효율이 떨어질 수 있다.

따라서, 한 Thread가 N개의 neighbor particle에 대해 v_laplacian velocity를 계산하게 수정하여 병렬처리 성능을 개선하였다.

![update_init_fluid_acceleration](https://github.com/user-attachments/assets/933f0311-ea1d-4c27-89e8-d9f87bf5aa7c)

N을 바꾸어 가면서 계산시간을 테스트 해보았고, 결론적으로 N=2일 때 개선전 대비 `약 82%`의 계산시간 감소를 얻을 수 있었다.

|N|1(original)|2|4|8|
|---|---|---|---|---|
|Computation Time(ms)|1.48523|0.270606|0.308372|0.356083|


## PCISPH 코드 최적화 - update_number_density
기존 코드는 한 thread당 neighbor particle개수 만큼 W값을 계산하였다.

neighbor particle개수는 최대 200까지 될 수 있어 한 Thread의 연산부하가 너무 커 병렬화 효율이 떨어질 수 있다.

따라서, 한 Thread가 N개의 neighbor particle에 대해 W를 계산하게 수정하여 병렬처리 성능을 개선하였다.

![update_number_density](https://github.com/user-attachments/assets/aa4fed50-de4e-43d7-b0c8-a36b1c0d3817)

N을 바꾸어 가면서 계산시간을 테스트 해보았고, 결론적으로 N=2일 때 개선전 대비 `약 48%`의 계산시간 감소를 얻을 수 있었다.

|N|1(original)|2|4|8|
|---|---|---|---|---|
|Computation Time(ms)|0.329368|0.172437|0.206938|0.228883|

## PCISPH 코드 최적화 - predict density error and update pressure
기존 코드는 한 thread당 neighbor particle개수 만큼 W값을 계산하였다.

neighbor particle개수는 최대 200까지 될 수 있어 한 Thread의 연산부하가 너무 커 병렬화 효율이 떨어질 수 있다.

따라서, 한 Thread가 N개의 neighbor particle에 대해 W를 계산하게 수정하여 병렬처리 성능을 개선하였다.

![update_predict_den_err](https://github.com/user-attachments/assets/0b601fc2-f8d4-46ba-b672-d513e3a4e7e0)


N을 바꾸어 가면서 계산시간을 테스트 해보았고, 결론적으로 개선전 코드가 항상 더 나은 성능을 보였다.

|N|original|2|4|8|
|---|---|---|---|---|
|Computation Time(ms)|0.66788|0.784055|0.927477|1.09307|

따라서, 개선전 코드에서 NUM_THREAD를 바꿔가면서 성능을 테스트해보았고, 결론적으로 NUM_THREAD가 32일 때, 약 17% 계산시간이 감소하였다.

|NUM_THREAD|32|64|128|256|
|---|---|---|---|---|
|Computation Time(ms)|0.553398|0.622605|0.636829|0.671903|

### 고민
다른 코드와 비슷한 형태인데, 왜 이 코드만 개선전이 성능이 더 좋은지 고민중이다.



</br></br></br>

# 2024.08.14
## 결과

최적화 결과는 다음과 같다.

```
PCISPH_GPU Performance Analysis Result per frame
================================================================================
_dt_avg_update                                              5.56771       ms
================================================================================
_dt_avg_update_neighborhood                                 1.15773       ms
_dt_avg_update_scailing_factor                              0.345694      ms
_dt_avg_init_fluid_acceleration                             1.48523       ms
_dt_avg_init_pressure_and_a_pressure                        0.00508841    ms
_dt_avg_copy_cur_pos_and_vel                                0.0187942     ms
_dt_avg_predict_velocity_and_position                       0.0388983     ms
_dt_avg_predict_density_error_and_update_pressure           0.672252      ms
_dt_avg_cal_max_density_error                               0             ms
_dt_avg_update_a_pressure                                   0.81341       ms
_dt_avg_apply_BC                                            0.00583697    ms
================================================================================

Neighborhood_Uniform_Grid_GPU Performance Analysis Result Per Frame
================================================================================
_dt_avg_update                                              1.10597       ms
================================================================================
_dt_avg_update_GCFP                                         0.021993      ms
_dt_avg_rearrange_GCFP                                      0.0145832     ms
_dt_avg_update_nfp                                          0.891356      ms
================================================================================
```

## PCISPH 코드 최적화 - update a_pressure
하나의 Thread에서 하나의 neighbor particle에 대한 정보를 계산하여 Group Shared Memory에 계산값을 저장하고 있었다.

```cpp
if (Gindex < num_nfp)
{
  //...
  v_grad_pressure = coeff * v_grad_kernel;
}

shared_v_grad_pressure[Gindex] = v_grad_pressure;

GroupMemoryBarrierWithGroupSync();
//...
```

이 때, Thread당 연산 부하가 작아 오히려 병렬화 효율이 낮아질 수 있다는걸 알게 되어 하나의 Thread당 N개의 neighbor particle에 대한 정보를 계산하도록 개선하였다.

```cpp
for (uint i=0; i<N; ++i)
{
  const uint index = Gindex * N + i;
    
  if (num_nfp <= index)
      break;

  shared_v_grad_pressure[index] = coeff * v_grad_kernel;
} 
```

그리고 group shared memory에 저장된 값들을 누산하게 되는데, 하나의 Thread에서 계산된 값들은 Register를 이용해서 미리 누산하게 되면 group shared memory에 접근했던 방식을 register에 접근하는 방식으로 개선하였다.

```cpp
float3 v_grad_pressure = float3(0,0,0);

for (uint i=0; i<N; ++i)
{
  const uint index = Gindex * N + i;
    
  if (num_nfp <= index)
      break;

  v_grad_pressure += coeff * v_grad_kernel;
} 

shared_v_grad_pressure[Gindex] = v_grad_pressure;
```

N을 바꾸어 가면서 계산시간을 테스트 해보았고, 결론적으로 N=2일 때 개선전 대비 약 31%의 계산시간 감소를 얻을 수 있었다.

|N|1(original)|2|4|8|
|---|---|---|---|---|
|Computation Time(ms)|1.16928|0.806842|0.904428|1.06757|

### Register와 Groupshared Memory
register는 GPU에서 가장 빠른 메모리 공간으로 커널 내에서 어떠한 qualifier도 없이 선언되는 automatic 변수는 일반적으로 register에 저장된다. 

Groupshared Memory는 on-chip이기 때문에 local 이나 global memory 보다 높은 bandwidth와 낮은 latency를 가지고 있지만 register에 비해서는 느리다. 

## Neighborhood 코드 최적화 - update_nfp
이전에 병렬화 효율을 개선하기 위해 하나의 Thread가 num_ngc * num_gcfp(neighbor grid cell에 들어 있는 모든 particle)에 대한 정보를 계산하던 방식에서 하나의 Thread가 하나의 particle에 대한 정보를 계산하게 수정하였더니 오히려 성능이 안좋아 졌었다.

![2024-08-14 10 49 15](https://github.com/user-attachments/assets/35b007e8-68e6-43c3-9799-01a40cca15e5)

이 또한, Thread당 연산 부하가 작아 오히려 병렬화 효율이 낮아진 경우일 수 있겠다는 판단에 Thread당 neighbor grid cell 개수의 Particle에 대한 정보를 계산하는 방식으로 개선하였다.

![update_nfp](https://github.com/user-attachments/assets/b26f6aae-5fd6-47a0-96cc-678d9319c9c5)


또한, ngc_index_buffer와 GCFP_count_buffer에 접근하는 부분을 group shared memory에 접근하는 방식으로 개선하였다.

![update_nfp2](https://github.com/user-attachments/assets/c026aa73-0eed-4cb6-afd3-a8d906da0b1e)

계산시간 테스트 결과 기존 대비 약 45% 계산시간이 감소한걸 확인할 수 있었다.

||origina|improved|
|---|---|---|
|Computation Time(ms)|1.67129|0.912575|

</br></br></br>

# 2024.08.13
## PCISPH 코드 최적화

### Frame 당 성능
PCISPH update 함수의 Frame당 성능 측정 결과는 다음과 같다.

```
PCISPH_GPU Performance Analysis Result Per Frame
================================================================================
_dt_avg_update                                              8.65898       ms
================================================================================
_dt_avg_update_neighborhood                                 2.37403       ms
_dt_avg_update_scailing_factor                              0.345017      ms
_dt_avg_init_fluid_acceleration                             1.48357       ms
_dt_avg_init_pressure_and_a_pressure                        0.00507673    ms
_dt_avg_copy_cur_pos_and_vel                                0.0191895     ms
_dt_avg_predict_velocity_and_position                       0.0428115     ms
_dt_avg_predict_density_error_and_update_pressure           0.678709      ms
_dt_avg_cal_max_density_error                               0             ms
_dt_avg_update_a_pressure                                   1.54367       ms
_dt_avg_apply_BC                                            0.00599321    ms
================================================================================
```

참고로, 위 결과는 400Frame을 측정하여 평균한 값이다.

### update_a_pressure 최적화

#### [문제]
기존 코드는 한 thread당 num_nfp번 v_grad_pressure를 계산한다.

```cpp
//...
  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i< num_nfp; ++i)
  {
    //...
    const float3 v_grad_q       = 1.0f / (g_h * distance) * v_xij;
    const float3 v_grad_kernel  = dWdq(distance) * v_grad_q;

    const float   coeff           = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
    const float3  v_grad_pressure = coeff * v_grad_kernel;

    v_a_pressure += v_grad_pressure;
  }
//...
```

num_nfp는 최대 200까지 될 수 있음으로, 이 부분을 개선하여 병렬처리 성능을 개선하려고 노력하였다.

#### [시도 - group shared memory]

병렬처리 효율을 개선하기 위해, group shared memory에 v_grad_pressure를 병렬적으로 계산해 저장하는 방식을 생각하였다.

하나의 Group에서는 NUM_THREAD 개수의 particlce에 대한 정보를 계산함으로 다음과 같은 group shared memory가 필요하다.

```
groupshared float3 shared_v_grad_pressure[NUM_THREAD * NUM_NFP_MAX];
```

하지만 이방식은 group shared memory의 크기의 한계 때문에 불가능하다.

NUM_THREAD로 최소한의 warp 단위인 32를 사용한다고 하더라도 필요한 group shared memory는 약 77KB이다.

$$ 32 \times 200 \times 12\text{byte} = 775,200\text{byte} $$

하지만 group shared memory는 그룹당 최대 16KB이다.

#### [해결]
하나의 Group에서 하나의 particle에 대한 정보를 계산하고 각 Thread가 하나의 neighbor particle에 대한 계산을 하게 수정하였다.

그 결과 for loop 대신에 group shared memroy를 사용하여 v_grad_pressure 값을 계산한뒤 최종적으로 그 값들을 전부 더하기만 하면 된다.

```cpp
groupshared float3 shared_v_grad_pressure[NUM_THREAD];

[numthreads(NUM_THREAD,1,1)]
void main(uint3 Gid : SV_GroupID, uint Gindex : SV_GroupIndex)
{
  float3 v_grad_pressure = float3(0,0,0);

  if (Gindex < num_nfp)
  {
    //...
    v_grad_pressure = coeff * v_grad_kernel;
    //...
  }

  shared_v_grad_pressure[Gindex] = v_grad_pressure;

  GroupMemoryBarrierWithGroupSync();

  uint stride = NUM_THREAD/2;
  while(num_nfp < stride)
    stride /=2;

  for (; 0 < stride; stride /= 2)
  {
    if (Gindex < stride)
    {
      const uint index2 = Gindex + stride;
      shared_v_grad_pressure[Gindex] += shared_v_grad_pressure[index2];      
    }
  
    GroupMemoryBarrierWithGroupSync();
  }
  
  if (Gindex == 0)
   v_a_pressure_buffer[fp_index] = -g_m0 * shared_v_grad_pressure[0]; 
}
```

참고로 group의 수는 한 dimension에 최대 65535개를 넘을 수 없어, particle의 수가 65535개가 넘는 경우 num_group_y를 활용하였다.

```cpp
  UINT num_group_x = _num_FP;
  UINT num_group_y = 1;

  if (max_group < _num_FP)
  {
    num_group_x = max_group;
    num_group_y = Utility::ceil(_num_FP, max_group);
  }

  _DM_ptr->dispatch(num_group_x, num_group_y, 1);
```

#### [결과]

개선후 update_a_pressure 함수의 성능 변화는 다음과 같다.

||before|opt|
|---|---|---|
|(ms)|1.54367|1.34131|

대략 0.2ms(13%) 계산시간이 줄어들었다.

#### [추가개선]

shared memory에 있는 값을 더할 때, 단일 Thread로 진행하게 수정하였다.

```
  if (Gindex == 0)
  {
    float3 v_a_pressure = float3(0,0,0);
    
    for (uint i=0; i <num_nfp; ++i)
      v_a_pressure += shared_v_grad_pressure[i];
  
    v_a_pressure_buffer[fp_index] = -g_m0 * v_a_pressure; 
  }    
```

개선후 update_a_pressure 함수의 성능 변화는 다음과 같다.

||before|opt|
|---|---|---|
|(ms)|1.54367|1.17244|

대략 0.37ms(24%) 계산시간이 줄어 들었다.

num_nfp가 큰 particle 보다 작은 particle들이 더 많기 GroupSync를 사용한 병렬화보다 sequential하게 더하는게 더 빨라 성능 향상이 된것 같다.

### 추가 고민1
update_a_pressure의 계산시간을 개선하기는 하였지만, 아직 다른 함수들에 비하면 많은 계산비용이 든다.

계산상에 비효율을 만들어내는 근본적인 이유가 있는 것 같은데, time stamp만으로는 함수 내부에 어떤 비효율이 있는지 파악하기 어렵다...

### 추가 고민2
위의 진행한 최적화와 별도로 중복 계산을 제거하여 성능향상을 시도하였지만 오히려 성능이 크게 떨어진 경우에 대해 그 원인에 대해 계속 고민하고 있다.

update_a_pressure 함수의 성능을 비교해보면, 미리 계산한 값을 활용하는 코드보다 기존 코드가 데이터를 읽어오는 횟수도 약간 더 많고 추가적인 연산이 있음에도 불구하고 앞도적으로 성능이 더 좋은 이유를 더 고민해봐야 겠다.

![2024-08-13 18 29 39](https://github.com/user-attachments/assets/d8323a20-c1bd-4295-9c55-e8585067a5e7)

||기존코드|미리 계산한 값을 활용하는 코드|
|---|:---:|:---:|
|데이터 읽기|4 + 4*num_nfp|1 + 4*num_nfp|
|성능(ms)|1.54367|5.94051|

## Neighborhood 코드 최적화

### Frame 당 성능
Neighborhood update 함수의 Frame당 성능 측정 결과는 다음과 같다.

```
Neighborhood_Uniform_Grid_GPU Performance Analysis Result Per Frame
================================================================================
_dt_avg_update                                              1.96883       ms
================================================================================
_dt_avg_find_changed_GCFPT_ID                               0.00816641    ms
_dt_avg_update_GCFP                                         0.0155696     ms
_dt_avg_rearrange_GCFP                                      0.0152503     ms
_dt_avg_update_nfp                                          1.67129       ms
================================================================================
```

참고로, 위 결과는 400Frame을 측정하여 평균한 값이다.

### update_nfp 최적화

#### [문제]
기존 코드에는 하나의 Thread가 하나의 Particle에 대한 정보를 계산한다.

그래서, neighbor grid cell만큼 loop를 돌고, ngc loop 안에서 각 neighbor grid cell에 들어있는 particle만큼 loop를 돌아서 neighbor인지를 확인한다.

```cpp
  for (uint i=0; i< num_ngc; ++i)
  {
    const uint ngc_index    = ngc_index_buffer[start_index1 + i];
    const uint num_gcfp     = GCFP_count_buffer[ngc_index];
    const uint start_index2 = ngc_index * g_estimated_num_gcfp;

    for (uint j = 0; j < num_gcfp; ++j)
    {
      const uint    nfp_index = fp_index_buffer[start_index2 + j];    
      const float3  v_xj      = fp_pos_buffer[nfp_index];
      const float3  v_xij     = v_xi - v_xj;
      const float   distance  = length(v_xij);

      if (g_divide_length < distance)
        continue;
      
      Neighbor_Information info;
      info.fp_index = nfp_index;
      info.tvector  = v_xij;
      info.distance = distance;

      nfp_info_buffer[start_index3 + neighbor_count] = info;
    
      ++neighbor_count;
    }   
  }
```

즉, 하나의 Thread가 num_ngc * num_gcfp 만큼 반복문을 돌기 때문에 이를 병렬화하여 병렬처리 성능을 개선하고자 하였다.

#### [해결]
이를 위해 Group X,Y,Z를 활용해 Particle 개수 X neighbor grid cell 개수만큼 Group을 생성하였다.

```cpp
  UINT num_group_x = _common_CB_data.num_particle;
  UINT num_group_y = 1;
  UINT num_group_z = g_estimated_num_ngc;

  if (max_group < _common_CB_data.num_particle)
  {
    num_group_x = max_group;
    num_group_y = Utility::ceil(_common_CB_data.num_particle, max_group);
  }

  _DM_ptr->dispatch(num_group_x, num_group_y, num_group_z);
```

그리고 각 그룹에서는 하나의 geometry cell에 들어있는 particle들을 병렬적으로 탐색하여 neighbor를 찾도록 하였다.

```cpp
  if (Gindex < num_gcfp)
  {
    const uint    nfp_index = fp_index_buffer[ngc_index * g_estimated_num_gcfp + Gindex];    
    const float3  v_xj      = fp_pos_buffer[nfp_index];
    const float3  v_xij     = v_xi - v_xj;
    const float   distance  = length(v_xij);

    if (g_divide_length < distance)
      return;
      
    Neighbor_Information info;
    info.fp_index = nfp_index;
    info.tvector  = v_xij;
    info.distance = distance;

    uint nbr_count;
    InterlockedAdd(nfp_count_buffer[cur_fp_index], 1, nbr_count);

    nfp_info_buffer[cur_fp_index * g_estimated_num_nfp + nbr_count] = info;
  }
```

최종적으로 개선 전과 후의 코드는 다음과 같다.

![2024-08-14 10 49 15](https://github.com/user-attachments/assets/35b007e8-68e6-43c3-9799-01a40cca15e5)


#### [결과]

update_nfp 함수의 성능 변화는 다음과 같다.

||개선 전|개선 후|
|---|---|---|
|(ms)|1.67129|2.85834|

오히려 성능이 안좋아졌다.

예상되는 성능 저하의 원인은 다음과 같다.
1. Num Thread로 64를 사용하지만 64개의 particle을 포함하는 cell은 많지 않고 대부분의 cell이 훨씬 적은 수의 particle을 포함하고 있어서 오히려 thread가 비효율적으로 사용될 수 있다.
2. 중간에 InterlockedAdd 함수로 동기화 과정이 들어가 있다.

## 메모리 접근과 연산 속도
NVIDIA GeForce RTX 3060 12 GB 제품의 이론적인 FP32 연산 성능은 12.74TFLOPS이고 Memory Bandwidth는 360GB/s 임으로 Memory를 1byte 읽어오는 동안 약 36번의 floating point 연산을 할 수 있다.

> Reference  
> [techpowerup - geforce-rtx-3060-12-gb.c3682](https://www.techpowerup.com/gpu-specs/geforce-rtx-3060-12-gb.c3682)
> [blog](https://computing-jhson.tistory.com/27)

</br></br></br>

# 2024.08.12
## GPU 코드 최적화

결론적으로 약 1374ms를 줄여 32%정도 계산 시간을 줄였다.

$$ \frac{1374}{4325} \times 100 \sim 32 \% $$

```
                                                          before          after
===================================================     ===========     ==========
_dt_sum_update                                           4325.56 ms     2951.74 ms
===================================================     ===========     ==========
_dt_sum_update_neighborhood                              1575.66 ms     823.75   ms
_dt_sum_update_scailing_factor                           139.575 ms     138.884  ms
_dt_sum_init_fluid_acceleration                          598.65  ms     595.153  ms
_dt_sum_init_pressure_and_a_pressure                     1.97872 ms     2.04208  ms
_dt_sum_copy_cur_pos_and_vel                             7.52288 ms     7.57351  ms
_dt_sum_predict_velocity_and_position                    19.9792 ms     15.6958  ms
_dt_sum_predict_density_error_and_update_pressure        356.088 ms     269.273  ms
_dt_sum_cal_max_density_error                            149.539 ms     0        ms
_dt_sum_update_a_pressure                                839.965 ms     616.974  ms
_dt_sum_apply_BC                                         2.32352 ms     2.34109  ms
===================================================      ==========     ==========     
```

### Neighborhood Search 추가 병렬화
#### [문제]
Uniform Grid Neighborhood Search를 위한 데이터중 Geometry Cell마다 몇 개의 particle이 속해 있는지 저장하는 `GCFP_count_buffer`가 있다.

그리고 update_GCFP은 Geometry Cell이 바뀐 Particle들에 대한 정보를 업데이트하는 함수이며, 이 함수의 로직중에는 Geometry Cell의 `GCFP_count`를 늘리는 부분이 포함되어 있다.

`GCFP_count`를 병렬적으로 늘릴 경우 race condition이 발생할 수 있기 때문에 기존에는 병렬화가 되어 있지 않았다.

```
ConsumeStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer  : register(u3);

[numthreads(1, 1, 1)]
void main()
{
  //...

  for (uint i = 0; i < g_consume_buffer_count; ++i)
  {
    const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();

    GCFP_ID cur_id;
    cur_id.gc_index   = data.cur_gc_index;
    cur_id.gcfp_index = GCFP_count_buffer[data.cur_gc_index];

    //...

    //prev_id가 지워진 정보는 rearrange_GCFP_CS에서 업데이트함
    GCFP_count_buffer[cur_id.gc_index]++; //이 부분 때문에 병렬화가 안됨
  }
}
```

#### [해결]
hlsl 내장 함수 중 InterlockedAdd를 사용하면 thread group 내부에서는 물론 thread group간에서도 덧셈에 대해서 atomic 연산을 할 수 있다.

따라서, 위의 코드를 병렬화하고 InterlockedAdd를 사용해 GCFP_count_buffer에 문제가 생기지 않게 하였다.

```
ConsumeStructuredBuffer<Changed_GCFP_ID_Data> changed_GCFP_ID_buffer  : register(u3);

[numthreads(NUM_THREAD, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  //...
  const Changed_GCFP_ID_Data data = changed_GCFP_ID_buffer.Consume();
    
  GCFP_ID cur_id;
  cur_id.gc_index = data.cur_gc_index;
  InterlockedAdd(GCFP_count_buffer[data.cur_gc_index], 1, cur_id.gcfp_index);
  //...
}
```

#### [결과]
Neighborhood을 성능 측정한 결과는 다음과 같다.

```
                                            before        after
===================================       ==========     ==========
_dt_sum_update                            1575.94  ms    798.814 ms
===================================       ==========     ==========
_dt_sum_find_changed_GCFPT_ID             3.25735  ms    3.26042  ms
_dt_sum_update_GCFP                       784.848  ms    6.28652  ms
_dt_sum_rearrange_GCFP                    6.21322  ms    6.30793  ms
_dt_sum_update_nfp                        667.078  ms    669.123  ms
===================================       ==========     ==========
```

Neighborhood update만 비교를 해보면 약 780ms가 줄어서 약 50%의 성능 개선 효과가 있었다.

$$ \frac{780}{1576} \times 100 \sim 50 \% $$

### 중복 계산 제거(문제)

#### [동기]

SPH Framework의 수식들을 살펴보니 다음과 같은 중복 계산을 줄일 수 있음을 파악하였다.

* **[neighbor information에서 중복 계산]**

$x_{ij} = x_i - x_j$라고 한다면, $x_{ij} = -x_{ji}$이고, $|x_{ij}| = |x_{ji}|$이다.

따라서, $i$와 $j$가 neighbor이고 $i$의 neighbor information을 저장하기 위해 $x_{ij}$와 $|x_{ij}|$를 계산하였다면, $j$의 neighbor information을 저장할 때에는 $x_{ji}$와 $|x_{ji}|$를 다시 계산할 필요는 없다.

* **[밀도에서 중복 계산]**

$d_{ij} = ||x_i - x_j||$라고 하면, $i$와 $j$ particle에서 밀도 식은 다음과 같다.

$$ \rho(x_i) = m_0 \sum_j W(d_{ij}) $$

이 때, 수식을 자세히 살펴보면, 다음이 성립함을 알 수 있다.

$$ W(d_{ij}) = W(d_{ji}) $$

따라서, $i$의 밀도를 계산할 때, $W(d_{ij})$를 계산했다면, $j$의 밀도를 계산할 때는 $W(d_{ji})$를 다시 계산할 필요가 없다.

* **[압력에 의한 가속도에서 중복 계산]**

$c^p_{ij} = \frac{p_i}{\rho_i^2} + \frac{p_j}{\rho_j^2}$라고 하면, 압력에 의한 가속도 계산식은 다음과 같다.

$$ a_p(x_i) = - m_0 \sum_j c^p_{ij} \nabla W_{ij} $$

이 때, 수식을 자세히 살펴보면, 다음이 성립함을 알 수 있다.

$$ c^p_{ij} = c^p_{ji}, \quad \nabla W_{ij} = -\nabla W_{ji} $$

따라서, $a_p(x_i)$를 계산하였다면, $a_p(x_j)$를 계산할 때는, $c^p_{ji}$와 $\nabla W_{ji}$를 다시 계산할 필요가 없다.

* **[점성에 의한 가속도에서 중복 계산]**

$c^v_{ij} = \frac{v_{ij} \cdot x_{ij}}{x_{ij} \cdot x_{ij}+0.01h^2}$라고 하면, 점성에 의한 가속도 계산식은 다음과 같다.

$$ a_v(x_i) = 2 \nu (d+2) m_0 \sum_j \frac{1}{\rho_j} c^v_{ij} \nabla W_{ij}  $$

이 때, 수식을 자세히 살펴보면, 다음이 성립함을 알 수 있다.

$$ c^v_{ij} = c^v_{ji}, \quad \nabla W_{ij} = -\nabla W_{ji} $$

따라서, $a_v(x_i)$를 계산하였다면, $a_v(x_j)$를 계산할 때는, $c^v_{ji}$와 $\nabla W_{ji}$를 다시 계산할 필요가 없다.

#### [해결]

neighbor information은 중복없이 계산하고, 추가로 $W_{ij}, \nabla W_{ij}, c^p_{ij},c^v_{ij}$를 중복없이 계산 후 저장해 놓고 다른 계산에서 사용한다.

* **[neighbor information은 중복없이 계산]**

```
      if (nbr_fp_index != cur_fp_index)
      {
        uint this_ncount;
        InterlockedAdd(ncount_buffer[cur_fp_index], 1, this_ncount);

        uint neighbor_ncount;
        InterlockedAdd(ncount_buffer[nbr_fp_index], 1, neighbor_ncount);

        Neighbor_Information info;      
        info.nbr_fp_index   = nbr_fp_index;
        info.neighbor_index = neighbor_ncount;
        info.v_xij          = v_xij;
        info.distance       = distance;
        info.distnace2      = distance2;      

        ninfo_buffer[start_index3 + this_ncount] = info; 
      
        Neighbor_Information info2;
        info2.nbr_fp_index    = cur_fp_index;
        info2.neighbor_index  = this_ncount;
        info2.v_xij           = -v_xij;
        info2.distance        = distance;
        info2.distnace2       = distance2;      

        ninfo_buffer[start_index4 + neighbor_ncount]  = info2;
      }
      else
      {
        uint this_ncount;
        InterlockedAdd(ncount_buffer[cur_fp_index], 1, this_ncount);   
      
        Neighbor_Information info;      
        info.nbr_fp_index   = cur_fp_index;
        info.neighbor_index = this_ncount;
        info.v_xij          = v_xij;
        info.distance       = distance;
        info.distnace2      = distance2;      

        ninfo_buffer[start_index3 + this_ncount] = info;      
      }
```

* **[grad_W 중복없이 계산 및 저장]**

```
  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i = 0; i < num_nfp; ++i)
  {
    const uint index1 = start_index + i;
    const uint nbr_fp_index = ninfo_buffer[index1].nbr_fp_index;

    if (nbr_fp_index < fp_index)
      continue;   
    
    const float d = ninfo_buffer[index1].distance;
    
    if (d == 0.0f) 
      continue;

    const float3 v_xij    = ninfo_buffer[index1].v_xij;      
    const float3 v_grad_q = 1.0f / (g_h * d) * v_xij;
    const float3 v_grad_W = dWdq(d) * v_grad_q;

    const uint index2 = nbr_fp_index * g_estimated_num_nfp + ninfo_buffer[index1].neighbor_index;
    
    grad_W_buffer[index1] = v_grad_W;
    grad_W_buffer[index2] = -v_grad_W;
  }
```

* **[저장된 값 활용]**
```
  //...
  for (uint i=0; i< num_nfp; ++i)
  {
    const float d = ninfo_buffer[index1].distance;
    
    if (d == 0.0)
      continue;

    const float3  v_grad_kernel   = v_grad_W_buffer[index1];
    const float   coeff           = a_pressure_coeff_buffer[index1];
    const float3  v_grad_pressure = coeff * v_grad_kernel;

    v_a_pressure += v_grad_pressure;
  }
```

#### [결과]

```
                                                          before          after
===================================================     ==========      ==========  
_dt_sum_update                                          2951.74 ms      13167.5 ms  
===================================================     ==========      ==========  
_dt_sum_update_neighborhood                             823.75   ms     13.4758  ms 
_dt_sum_update_scailing_factor                          138.884  ms     46.7306  ms 
_dt_sum_init_fluid_acceleration                         595.153  ms     822.914  ms 
_dt_sum_init_pressure_and_a_pressure                    2.04208  ms     2.01104  ms 
_dt_sum_copy_cur_pos_and_vel                            7.57351  ms     7.55457  ms 
_dt_sum_predict_velocity_and_position                   15.6958  ms     15.4654  ms 
_dt_sum_predict_density_error_and_update_pressure       269.273  ms     122.208  ms 
_dt_sum_cal_max_density_error                           0        ms     0        ms 
_dt_sum_update_a_pressure                               616.974  ms     2385.12  ms 
_dt_sum_apply_BC                                        2.34109  ms     2.38333  ms 
_dt_sum_update_ninfo                                                    2381.93  ms            
_dt_sum_update_W                                                        1933.35  ms  
_dt_sum_update_grad_W                                                   2360.36  ms  
_dt_sum_update_laplacian_vel_coeff                                      597.336  ms  
_dt_sum_update_a_pressure_coeff                                         1478.12  ms  
===================================================                     ==========     
```

* **[고민1]**

update_ninfo의 경우 InterlockedAdd 함수를 호출하기 때문에 느릴 수 있음이 이해가 되지만, 그 외에 update_W ~ update_a_pressure_coeff 함수의 경우 기존 함수들과 비슷하거나더 적은 양의 계산을 하는데 왜 이렇게 오래 걸리는지 고민 중.

**update_W**

```
  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i = 0; i < num_nfp; ++i)
  {
    //...
    const Neighbor_Information ninfo = ninfo_buffer[index1];
    //...
    
    if (nbr_fp_index < fp_index)
      continue;    
    
    const float d     = ninfo.distance;
    const float value = W(d);
    
    const uint index2 = nbr_fp_index * g_estimated_num_nfp + ninfo.neighbor_index;

    W_buffer[index1] = value;
    W_buffer[index2] = value;    
  }
```

**기존 predict_density_error_and_update_pressure**

```
  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i<num_nfp; ++i)
  {
    const uint ninfo_index  = start_index + i;
    const uint nfp_index    = ninfo_buffer[ninfo_index].fp_index;

    const float3  v_xj      = pos_buffer[nfp_index];
    const float   distance  = length(v_xi-v_xj);

    rho += W(distance);
  }
```

* **[고민2]**

init_fluid_acceleration 함수와 update_a_pressure함수의 경우, 기존에는 계산을 하였고 개선 후에는 값을 읽어오기만 하는데 왜 더 느려졌는지 고민 중.

**저장된 값 활용**

```
  //...
  for (uint i=0; i< num_nfp; ++i)
  {
    const float d = ninfo_buffer[index1].distance;
    
    if (d == 0.0)
      continue;

    const float3  v_grad_kernel   = v_grad_W_buffer[index1];
    const float   coeff           = a_pressure_coeff_buffer[index1];
    const float3  v_grad_pressure = coeff * v_grad_kernel;

    v_a_pressure += v_grad_pressure;
  }
```

**기존 코드**

```
  for (uint i=0; i< num_nfp; ++i)
  {
    //..
    const float3  v_xij = v_xi - v_xj;
    const float   distance = length(v_xij);
    
    if (distance == 0.0)
      continue;

    const float3 v_grad_q         = 1.0f / (g_h * distance) * v_xij;
    const float3 v_grad_kernel    = dWdq(distance) * v_grad_q;
    const float   coeff           = (pi / (rhoi * rhoi) + pj / (rhoj * rhoj));
    const float3  v_grad_pressure = coeff * v_grad_kernel;

    v_a_pressure += v_grad_pressure;
  }

```


</br></br></br>

# 2024.08.09
## GPU 코드 최적화

### Update 최적화

#### [문제]
현재 Update 함수의 핵심 부분은 다음과 같다.
```cpp
//...
  while (_allow_density_error < density_error || num_iter < _min_iter)
  {
    this->predict_velocity_and_position();
    this->predict_density_error_and_update_pressure();
    density_error = this->cal_max_density_error();
    this->update_a_pressure();

    ++num_iter;

    if (_max_iter < num_iter)
      break;
  }
//...
```

이 떄, `cal_max_density_error`만 GPU에서 계산한 결과를 CPU로 받아오는 과정이 포함되어 있으며, 나머지는 전부 Compute Shader를 이용해 GPU로 계산만 하는 함수이다.

이를 간략화 하면 다음과 같다.

![before_opt](https://github.com/user-attachments/assets/56cb079b-a360-42c2-81d7-fe55996ddf7e)

이 때, GPU에서 CPU로 data를 읽어오는 과정 때문에 다음과 같은 비효율이 발생한다.
* CPU는 Produce Work 후에도 Read Data나 다음 Produce Work를 수행하지 못하고 GPU의 Consume Work을 기다려야 한다.
* GPU는 Consume Work 후에도, CPU가 Read Data와 Produce Work를 수행하는 동안 기다려야 한다.

#### [해결]

결론적으로, GPU->CPU data를 읽어오는 과정은 iteration 수를 결정하기 위함임으로 고정된 수만큼 iteration하면 GPU->CPU data를 읽어오는 과정이 필요 없어진다.

그러면 CPU는 Produce Work를 GPU는 Consume Work를 비동기적으로 수행할 수 있다.

![opt](https://github.com/user-attachments/assets/48f538d1-1a24-4268-979b-c309b787b65e)

이를 위해 코드를 다음과 같이 수정하였다.

```cpp
//...
  for (UINT i=0; i<_max_iter; ++i)
  {
    this->predict_velocity_and_position();
    this->predict_density_error_and_update_pressure();
    this->update_a_pressure();  
  }
//...
```

`_min_iter=1`, `_max_iter=3`인 상황에서 성능 분석 결과는 다음과 같다.

```
                                                            Before         After    
====================================================     ==========     ===========
_dt_sum_predict_velocity_and_position                    20.0431 ms      15.7366  ms
_dt_sum_predict_density_error_and_update_pressure        352.638 ms      269.75   ms 
_dt_sum_cal_max_density_error                            128.067 ms      0        ms      
_dt_sum_update_a_pressure                                804.587 ms      621.542  ms
====================================================     ==========     ===========
                                                         1305.3351 ms    907.0286 ms
```

Iteration과 관련된 부분만 비교를 해보면 약 400ms가 줄어서 약 30%의 성능 개선 효과가 있었다.

$$ \frac{400}{1305} \times 100 \sim 30\\% $$

성능 개선 효과가 두드러졌던 이유를 생각해보면 _min_iter와 _max_iter 차이가 적고 기존에도 _max_iter만큼 iteration이 되는 경우가 많았기 때문에 iteration을 비동기적으로 _max_iter만큼 수행하는게 동기화 과정을 포함하여 iteration 수를 줄이는 것보다 더 높은 성능을 보였던것 같다.

### Num Thread
단일 함수중 가장 cost가 큰 update_a_pressure의 경우 Group당 Thread개수에 따라 성능 차이가 발생하였다.

warp 단위를 만족하는 숫자중 128, 256, 512, 1024를 테스트 해보았고 결과는 다음과 같다.

| #thread |average (ms)|
|------|:---:|
|128|831|
|256|830|
|512|806|
|1024|910|

1024 thread를 사용했을 떄와, 512 thread를 사용했을 떄를 비교해보면 104ms가 줄어서 약 11%의 성능 개선 효과가 있었다.

$$ \frac{104}{910} \times 100 \sim 11\\% $$


</br></br></br>

# 2024.08.08
## GPU 코드 최적화

### [진행사항]

성능 측정 코드를 작성하고, PCISPH 업데이트 함수의 각 부분별로 성능을 측정하였다.

성능 측정 코드는 다음과 같이 작성하였다.

```cpp
std::vector<ComPtr<ID3D11Query>> _cptr_start_querys;
std::vector<ComPtr<ID3D11Query>> _cptr_disjoint_querys;
ComPtr<ID3D11Query> _cptr_end_query2;
```

```cpp
void Utility::set_time_point(void)
{
  const auto cptr_context = _DM_ptr->context_cptr();

  _cptr_disjoint_querys.push_back(_DM_ptr->create_time_stamp_disjoint_query());
  _cptr_start_querys.push_back(_DM_ptr->create_time_stamp_query());

  cptr_context->Begin(_cptr_disjoint_querys.back().Get());
  cptr_context->End(_cptr_start_querys.back().Get());
}
```

```cpp
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
  return (delta / frequency) * 1000.0f; //millisecond 출력
}
```

### [결과]

```
PCISPH_GPU Performance Analysis Result
======================================================================
_dt_sum_update                                              4154.15 ms
======================================================================
_dt_sum_update_neighborhood                                 1450.77 ms
_dt_sum_update_scailing_factor                              139.718 ms
_dt_sum_init_fluid_acceleration                             602.5 ms
_dt_sum_init_pressure_and_a_pressure                        2.04115 ms
_dt_sum_copy_cur_pos_and_vel                                7.46986 ms
_dt_sum_predict_velocity_and_position                       19.9418 ms
_dt_sum_predict_density_error_and_update_pressure           354.673 ms
_dt_sum_cal_max_density_error                               123.229 ms
_dt_sum_update_a_pressure                                   834.01 ms
_dt_sum_apply_BC                                            2.35636 ms
======================================================================
```

### [문제]
update 함수 전체의 cost를 측정한 결과와 update 내부의 함수들의 cost를 측정한 결과의 합이 차이가 많이 난다.

* 4154.15 ms (update 함수의 cost)
* 3536.70917ms (내부 함수들의 cost sum)

성능 측정 방식이 잘못 된것은 아닌지 고민중.

## 향후 진행
* GPU performance analaysis 결과를 바탕으로 시간이 가장 오래걸린 순서로 최적화 시도
* water rendering
* interaction with rigid body


</br></br></br>

# 2024.08.07
## PCISPH CPU >> GPU

### [진행사항]

* GPU 코드 검증 완료

### [결과]

**Dam Breaking Simulation**

| Description ||
|------|:---:|
|Solution Domain(XYZ)|4 X 6 X 0.6 (m)|
|Inital Water Box Domain(XYZ)|1 X 2 X 0.4 (m)|
|intial spacing|0.04 (m)|
|#particle|~15000(14586)|
|dt|0.01 (s)|
|FPS|300-350|
|Simulation Time / Computation Time  | 3.0 - 3.5 |

CPU 병렬화 코드 대비 FPS가 7-8배 정도 증가 되었다.

![DamBreaking-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/ba77e52f-b39a-4d07-b57c-4b1ba45d2905)


**Double Dam Breaking Simulation1**

| Description ||
|------|:---:|
|Solution Domain(XYZ)|3 X 6 X 3 (m)|
|Inital Water Box Domain(XYZ)|1 X 2 X 0.4 (m)|
|intial spacing|0.04 (m)|
|#particle|~70000(68952)|
|dt|0.01 (s)|
|FPS|40-50|
|Simulation Time / Computation Time  | 0.4 - 0.5 |

![DoubleDamBreaking0 04-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/08a617a9-d4cc-479d-a3c3-922026fbdee1)


**Double Dam Breaking Simulation2**

| Description ||
|------|:---:|
|Solution Domain(XYZ)|3 X 6 X 3 (m)|
|Inital Water Box Domain(XYZ)|1 X 2 X 0.4 (m)|
|intial spacing|0.05 (m)|
|#particle|~35000(35280)|
|dt|0.01 (s)|
|FPS|100-110|
|Simulation Time / Computation Time  | 1.0 - 1.1 |

![DoubleDamBreaking0 05-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/53c1e83b-9471-4b23-b83b-a35a692140f5)


### [문제]

GPU 코드 최적화
* CPU 코드는 visual studio에서 제공하는 성능 프로파일링을 사용하여 CPU 사용량을 체크해 어떤 함수의 어떤 부분을 개선해야 하는지 파악할 수 있었다.
* GPU 코드의 경우 어떻게 성능 프로파일링을 하여 어떤 함수의 어떤 부분에서 computation cost가 얼마나 드는지를 파악할 수 있는지 잘 모르겠다...
* GPU 코드의 hot zone을 먼저 파악하고, 그 부분부터 성능 개선을 시도할 예정

Iteration 코드
* CPU 코드로 iteration loop를 돌고, iteration 내부에서 필요한 계산들은 GPU 코드로 되어있다.
* CPU에서 iteration 종료 조건을 판단하기 위해, float 1개 이기는 하지만 GPU -> CPU 메모리 복사가 매 iteration 마다 한번씩 발생한다.
* Iteration 자체를 GPU 코드로 만들 수는 없는지 고민중...

```cpp
void PCISPH_GPU::update(void)
{
  _uptr_neighborhood->update(_fluid_v_pos_RWBS);

  this->update_scailing_factor();
  this->init_fluid_acceleration();
  this->init_pressure_and_a_pressure();

  float  density_error = 1000.0f;
  size_t num_iter      = 0;

  const auto cptr_context = _DM_ptr->context_cptr();
  cptr_context->CopyResource(_fluid_v_cur_pos_RWBS.cptr_buffer.Get(), _fluid_v_pos_RWBS.cptr_buffer.Get());
  cptr_context->CopyResource(_fluid_v_cur_vel_RWBS.cptr_buffer.Get(), _fluid_v_vel_RWBS.cptr_buffer.Get());

  while (_allow_density_error < density_error || num_iter < _min_iter)
  {
    this->predict_velocity_and_position();
    this->predict_density_error_and_update_pressure();
    density_error = this->cal_max_density_error();
    this->update_a_pressure();

    ++num_iter;

    if (_max_iter < num_iter)
      break;
  }

  this->predict_velocity_and_position();
  this->apply_BC();
}
```

## 향후 진행 
* GPU 코드 최적화
* water rendering
* interaction with rigid body

</br></br></br>

# 2024.08.06
## PCISPH CPU >> GPU
PCISPH는 다음 단계로 이루어져 있다.

1. scailing factor를 계산한다.
2. pressure을 제외한 acceleration을 계산한다.
3. Pressure acceleration 계산을 위한 Iteration을 돈다.
   1. velocity와 position을 예측한다.
   2. density와 density error를 계산하고 pressure을 업데이트 한다.
   3. pressure acceleration을 계산한다.
4. velocity와 position을 업데이트한다.

**[진행사항]**

* 4 단계(velocity와 position을 업데이트한다)까지 GPU 코드 작성 완료
* GPU 코드 디버깅 중

</br></br></br>

# 2024.08.05
## PCISPH CPU >> GPU
PCISPH는 다음 단계로 이루어져 있다.

1. scailing factor를 계산한다.
2. pressure을 제외한 acceleration을 계산한다.
3. Pressure acceleration 계산을 위한 Iteration을 돈다.
   1. velocity와 position을 예측한다.
   2. density와 density error를 계산하고 pressure을 업데이트 한다.
   3. pressure acceleration을 계산한다.
4. velocity와 position을 업데이트한다.

**[진행사항]**

* 1단계 (scailing factor 계산) GPU 코드 작성 및 검증 완료
* 2단계 (pressure을 제외한 acceleration을 계산) GPU 코드 작성 및 검증 완료
* 3-1 단계 (velocity와 position 예측) GPU 코드 작성 완료
* 3-1 단계 (velocity와 position 예측) GPU 코드 디버깅

### parallel reduction algorithm
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


# 2024.08.02
## Uniform Grid CPU >> GPU
* GPU 코드 결과 검증 완료

### Texture2D > StructuredBuffer
기존에 Texture2D로 구현된 데이터들이 있었다.

```
// geometry cell마다 neighbor geometry cell의 index들을 저장한 texture
ComPtr<ID3D11Texture2D>          _cptr_ngc_texture;

// geometry cell마다 속해있는 fluid particle의 index들을 저장한 texture
ComPtr<ID3D11Texture2D>           _cptr_GCFP_texture;

// fluid particle마다 neighbor fluid particle의 index를 저장한 texture
ComPtr<ID3D11Texture2D>           _cptr_nfp_texture;

...
```

실제로 이 데이터들을 사용할 때, data access는 width 방향으로만 이루어진다.

하지만 Texture2D에서는 Tiling하여 데이터를 저장하기 때문에 전체 width에 대해서 spatial locality가 보장되지 않아 적합하지 않은 데이터 구조이다.

따라서, 현재 data access pattern에는 Texture2D보다 StructuredBuffer가 더 적합하기 때문에 Texture2D를 StructuredBuffer로 전부 수정하였다.

```
// geometry cell * estimated ngc만큼 ngc index을 저장한 buffer
ComPtr<ID3D11Buffer>             _cptr_ngc_index_buffer;

// geometry cell * estimated gcfp만큼 fp index를 저장한 buffer
ComPtr<ID3D11Buffer>              _cptr_fp_index_buffer;

//fluid particle * estimated neighbor만큼 Neighbor_Information을 저장한 Buffer
ComPtr<ID3D11Buffer>              _cptr_nfp_info_buffer;
```

## PCISPH CPU >> GPU
PCISPH는 다음 단계로 이루어져 있다.

1. scailing factor를 계산한다.
2. pressure을 제외한 acceleration을 계산한다.
3. Pressure acceleration 계산을 위한 Iteration을 돈다.
   1. velocity와 position을 예측한다.
   2. density와 density error를 계산하고 pressure을 업데이트 한다.
   3. pressure acceleration을 계산한다.
4. velocity와 position을 업데이트한다.

**[진행사항]**

* 1단계 (scailing factor 계산) GPU 코드 작성 중

</br></br></br>

# 2024.08.01
## Uniform Grid CPU >> GPU

**[진행사항]**

* GPU 코드 작성 완료
* GPU 코드 디버깅 중
  
### Structured Buffer 크기 한계 문제

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


### DXGI FORMAT & UAV 문제

**[문제사항]**

tvec texture을 만들기 위해 DXGI_FORMAT_R32G32B32_FLOAT format으로 Texutre2D를 만들고 UAV를 만들려고 하였으나 오류가 발생하였다.

```
X4596	typed UAV stores must write all declared components.	
```

**[해결]**

확인해보니, DXGI_FORMAT_R32G32B32_FLOAT는 UAV가 지원되지 않는 타입이기 때문이였고 이를 해결하기 위해  DXGI_FORMAT_R32G32B32A32_FLOAT format으로 수정하였다.

</br></br></br>

# 2024.07.31
## Uniform Grid CPU >> GPU
Uniform Grid는 다음 3단계의 작업을 한다.

1. 새로운 Geometry Cell로 이동한 Fluid Particle들을 찾는다.
2. Geometry Cell마다 어떤 Fluid Particle이 들어있는지 업데이트한다.
3. Fluid Particle마다 Neighbor Fluid Particle들의 정보를 업데이트한다.

### 진행사항
이 중 1,2 단계에 대한 GPU 코드 작성을 완료.

3단계에 대한 GPU 코드 작성 중

### 1단계 병렬화

 1단계와 3단계는 병렬화가 가능하지만 CPU 코드는 3단계만 병렬화가 되어 있었다.

```cpp
  for (size_t fpid = 0; fpid < num_fluid_particles; ++fpid)
  {
    // 1단계
    //...
    if (prev_gcid != cur_gcid) 
    {      
      // 2단계
      // ...
    }
  }

  this->update_fpid_to_neighbor_fpids(fluid_particle_pos_vectors); // 3단계
```

이를 GPU 코드로 옮기면서, 1단계도 병렬화를 진행하였다.

### Append Consum StrucrtuedBuffer
AppendStructuredBuffer와 ConsumeStructuredBuffer를 활용하여 2단계에서 필요한 만큼만 작업을 하게 하였다.

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

</br></br></br>

# 2024.07.30
## CPU >> GPU
* Compute Shader 관련 기본적인 내용 학습
* Uniform Grid를 활용한 Neighbor search CPU 코드 >> GPU 코드 작업 중

**향후 계획**
* Uniform Grid를 활용한 Neighbor search GPU 코드 검증 및 성능 테스트
* PCISPH CPU코드 >> GPU 코드

</br></br></br>

# 2024.07.29

## PCISPH 구현
14586 Particle, $h = 1.2 \Delta x$ 기준으로 PCISPH가 $\Delta t = 1.0e-2$, $FPS = 43$정도이다.

Simulation Time 1초당 Physical Time 0.43초 임으로 WCSPH 기준 약 $43\%$의 성능 개선이 있었다.

$$ \frac{0.43}{0.33} = 1.4333... $$

![2024-07-29180807dx0 04dt0 01PCISPH43FPS-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/da01abd4-33f9-44bf-9c2a-6374234c8684)

### 수렴 문제
논문에 나와 있는데로, 항상 error가 수렴하지는 않는다.

![PCISPH 수렴](https://github.com/user-attachments/assets/0112ed0a-22f6-4543-8792-85bf37cc37c5)

```
0.18s (수렴하지 않는 예시)
Iter / Error
0 403.463
1 207.675
2 225.677
3 227.161
4 497.61
5 614.645
6 1016.82

0.7s (수렴하는 예시)
Iter / Error
0 162.331
1 63.2604
2 50.6395
3 43.057
4 38.1294
```

따라서, 적절한 iteration 탈출 조건을 위해 max_iteration 변수를 추가하였다.

그리고 항상 error가 수렴하지는 않기 때문에, max_iteration을 많이 준다고 더 나은 결과가 나오지는 않으며 오히려 unstable한 해를 얻게 되는 경우가 있다.

[min3 max10]

![2024-07-29180423min3max10-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/4b47264d-264d-4769-8c58-3e1169c19e15)

[min0 max3]

![2024-07-29180339min0max3-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/1df53159-fa02-4284-9eb3-10be9afbb125)

따라서, problem dependent하게 min max value를 정해줘야 할 필요가 있다.

</br></br></br>

# 2024.07.26
## Solver 개선
dt를 증가시키기 위해, PCISPH 구현 및 디버깅 중

* 2009 (solenthaler and Pajarola) Predictive-Corrective Incompressible SPH

</br></br></br>

# 2024.07.25
## Solver 개선
dt를 증가시키기 위해, PCISPH 학습 및 구현 중

* 2009 (solenthaler and Pajarola) Predictive-Corrective Incompressible SPH

</br></br></br>


# 2024.07.24

## Solver 개선

### smoothing length 증가

**[장점]**

기존 smoothing length $(h)$ 보다 $h$를 늘리면 simulation 결과에서 물의 출렁거림이 더 잘 표현됨.

$h = 0.6 \Delta x$ (기존)

![2024-07-25 10 24 25 h = 0 6dx](https://github.com/user-attachments/assets/5889c001-046b-4d3e-b2f6-08187a51577e)


$h = 1.1 \Delta x$

![2024-07-25 10 25 13 h = 1 1dx](https://github.com/user-attachments/assets/cc9b8d92-63d6-41ca-8e91-c826281950bb)


$h = 1.6 \Delta x$

![2024-07-25 10 27 06 h = 1 6dx](https://github.com/user-attachments/assets/018aabf2-cf99-48fc-af4b-e70d1c7adce8)

또한, $h$가 늘어나면 $\Delta t$를 증가 시킬 수 있음. 

$$ \Delta t = 5.0e^{-4} \rightarrow 1.0e^{-3} \rightarrow 2.5e^{-3}$$

**[단점]**

꼭, $h$에 비례해서 simulation 결과가 개선되는 것은 아님.

$h$가 증가하면 neighborhood가 증가로 인해 FPS가 감소함.

$$\text{FPS} = 1200 \rightarrow 500 \rightarrow 200$$

$h$가 증가함에 따라서, particle의 density 변화량이 매우 커져서 기존 방식으로는 해결이 불가능해 이에 대한 처리가 필요함. ($\rho \sim n$이기 때문, $n$은 particle number density)

$$ \frac{\rho_{min}}{\rho_{max}} = 0.97 \rightarrow 0.54 \rightarrow 0.37 $$

 

</br></br></br>

# 2024.07.23
Solver 개선을 위해 WCSPH를 이용한 Free Surface Flow 참고자료 학습
*  1996 (Koshizuka & Oka) Moving-Particle Semi-Implicit Method for Fragmentation of Incompressible Fluid
*  2007 (Crespo et al) Boundary Conditions Generated by Dynamic Particles in SPH Methods
*  2010 (Mocho et al) State-of-the-art of classical SPH for free-surface flows
*  2014 (Goffin et al) Validation of a SPH Model For Free Surface Flows
*  2016 (Shobeyri) A Simplified SPH Method for Simulation of Free Surface Flows

</br></br></br>

# 2024.07.22
**[Surface Tension 구현]**

Surface Tension을 추가로 구현하여, Zero gravity droplet 테스트를 진행

![droplet](https://github.com/user-attachments/assets/916e7bde-b7af-44fa-a85b-14ad4638114c)

구형을 유지해야 하지만 계속해서 팽창해 나가는 문제가 발생

![2024-07-23 11 02 12 Surface Tension (2)](https://github.com/user-attachments/assets/47a707ef-1d7c-4196-af91-d6efab1e3532)


**[Boundary Particle 구현]**

현재는, boundary처리를 다음과 같은 간단한 수식으로 처리 중.

```cpp
     auto& v_pos = _fluid_position_vectors[i];
     auto& v_vel = _fluid_velocity_vectors[i];

     //left wall
     if (v_pos.x < wall_x_start && v_vel.x < 0.0f)
     {
       v_vel.x *= -cor2;
       v_pos.x = wall_x_start;
     }
     //...
```

이런 boundary condition 때문에, 점성이 강해보일 수 있다고 판단해 boundary particle 도입 시도.

boundary particle $k$가 fluid particle $a$에게 주는 영향은 다음과 같이 계산 됨.

$$ f_{ak} = \frac{m_k}{m_a+m_k} B(x,y) n_k $$

$$ B(x,y) = \Gamma(y) = 0.02 \frac{(v_s)^2}{y}  \begin{cases} \frac{2}{3} \\ 2q - 1.5q^2 \\ 0.5(2-q)^2 \end{cases} $$

$x$는 boundary tangential 거리 $y$는 boundary normal 거리이며, $q=y/h$, $v_s$는 가상의 음속이다.

이를 구현하였지만, 진동 문제가 발생함

![2024-07-23 09 57 13 boundary particle problem](https://github.com/user-attachments/assets/34fded04-116b-4549-87ad-8f05d9beb68d)

시간에 따른 거리 속도 가속도 그래프를 그려보니, 수렴하지 않고 일정한 진폭을 갖는 모습을 보임.

![boundary particel SVA](https://github.com/user-attachments/assets/b473b8ad-8c68-49aa-9d23-af2f6fd3e2a9)

주황색 - 거리, 회색 - 속도, 노란색 - 가속도

액체가 탄성을 갖는 공도 아닌데, 진동하면서 수렴해가는 거동을 하는 것도 이상하고,

velocity ~ 0과 acceleartion ~ 0 을 동시에 맞추는게 상당히 어려울 것 같음.

</br></br></br>

# 2024.07.19
## Solver
Surface Tension Force와 Boundary Particle 관련 내용 학습
* 1994 (monaghan) Simulating Free Surface Flows with SPH
* 2000 (Joseph) Simulating surface tension with SPH
* 2005 (monaghan) Smoothed Particle Hydrodynamics
* 2007 (Markus and Matthias) Weakly compressible SPH for free surface flows
* 2022 (Dan et al) A Survey on SPH Methods in Computer Graphics

<br/><br/><br/>

# 2024.07.18
## Solver

### 압축 후 팽창하는 문제

* 원인: Boundary와의 상호작용.

$z=0$ 위치에 있는 particle들이 움직이는 과정에서 $z=0$ boundary와 상호작용이 발생한다.

상호작용이 발생하게 되면 순간적으로 위치와 속도가 조정이 되고 이 영향으로 폭발적으로 팽창하게 된다

<br/>

* 해결

![2024-07-19 10 33 06](https://github.com/user-attachments/assets/5c27710d-effe-44aa-8c26-651900078a2f)

particle들의 초기 위치를 boundary와 약간(particle지름 만큼) 떨어뜨림으로써, boundary와의 상호작용으로 인한 영향을 없에 폭발적으로 팽창하는 문제는 해결

### 압축 팽창 반복 문제

* 원인 : 밀도 불균형

물리적으로, 물은 $\rho_0 = 1000$이라는 rest density를 가져야 한다.

하지만 partic들의 $\rho$를 계산하면 973, 982, 991, 1000의 4가지 값을 갖게 된다. 

이 떄, 1000보다 작은 밀도를 갖는 particle들이 압축시키는 힘을 만들어내고, 이로 인해 거리가 가까워지면 $\rho$값이 증가하여 다시 팽창시키는 힘이 만들어진다.

이 과정을 통해 압축 팽창이 반복되게 된다.

<br/>

* 밀도 불균형의 원인 : SPH descritization

partic들의 $\rho$가 왜 모두 1000이란 값으로 계산될 수 없는지는  SPH descritization에 근본적인 한계 때문이다.

SPH descritization에서 $x$ 위치에서 밀도는 다음과 같다.

$$ \rho(x) =  \sum_j m_j W(x,x_j,h) $$

이 때, 일반적으로 $m_j$는 particle마다 전부 동일하다는 가정으로 상수값으로 두어 식을 다음과 같이 단순화한다.

$$ \rho(x) = m \sum_j  W(x,x_j,h) $$

즉, $\rho$는 결국 $m$과 $W$에 의해 결정된다.

이 때, $W$ 함수의 성질에 의해 $\sum_j W(x,x_j,h)$ 값은 $x$ 위치에서의 particle의 number density(밀집정도)를 나타내는 값이 된다.

따라서, boundary에서 particle의 number density는 감소하게 됨으로 $\sum_j W(x,x_j,h)$ 값의 불균형으로 인한 density의 불균형은 SPH descritization에서 필연적이다.

<br/>

* 밀도 불균형 해결 : $m$을 잘 결정하자

$\rho$는 $m$을 어떻게 결정하는지에 따라서도 달라진다.

현재 $m$을 2012 (Mostafa et al)에서 제시한 다음 수식으로 결정한다.

$$ m_i = \frac{\rho_0}{\max(N)} $$

이 떄, $N$은 particle의 number density의 집합이다.

즉, 가장 밀집된 곳에 있는 particle의 밀도가 $\rho_0$가 되게끔 결정되고 있다.

이를 다음과 같이 수정한다.

$$ m_i = \frac{\rho_0}{\text{avg}(N)} $$

즉, 평균적인 밀집정도를 갖는 곳에 있는 particle의 밀도가 $\rho_0$가 되게끔 결정하게 되고

이는 density fluctuation $\frac{|\rho-\rho_0|}{\rho_0}$을 줄일 수 있다.

그 결과 압축 팽창 정도를 많이 줄일 수 있다.

![2024-07-19 11 27 20 avg](https://github.com/user-attachments/assets/23b824c3-40ff-4d7b-9c4b-ca2d41a771cf)

* 추가 고려 사항  
  * 실제 simulation에서 어떤 차이를 보이는지 테스트해볼 예정


### Time Integration

많은 논문에서 Time integration으로 사용하는 semi implicit euler scheme과 leap frog scheme을 구현하여 dt를 바꿔가며 stability를 비교하였다.

결론적으로, dt를 10-4 단위로 바꿔가면서 test 해본 결과 둘의 stability는 유의미하게 차이가 나지 않았다.

계산 algorithm이 단순하여 가장 성능이 좋은 semi implicit euler scheme을 사용하기로 결정하였다.

<br/><br/><br/>

# 2024.07.17
## Solver
solver의 문제점을 파악하기 위해 밀도값 제한을 풀어서 시뮬레이션을 돌림

**문제점**
* SPH Particle들이 압축 후 팽창하는 현상 발견

![2024-07-18 10 41 02](https://github.com/user-attachments/assets/43a267f1-e410-414d-a5bf-320eba1cfb6a)

**해결 방안**
* density, pressure, acceleration 값들을 보면서, 문제 원인 분석
* 여러 논문으로 solver에 구현된 수식 교차 검증
* 여러 논문으로 solver에 적용된 상수 값 교차 검증

<br/><br/><br/>

# 2024.07.16
## Rendering
* view space에서 particle sphere의 normal과 depth 계산 방법 학습
  * pixel shader에서 spherrical billboards의 texture coordinate 활용

## Solver
* 피드백: 시뮬레이션 결과가 물에 비해 점성 커보임
  * dynamic viscosity 값을 줄여서 점성을 줄이면 개선 가능
  * 그러나 충분히 줄이기 전에 particle이 계속 진동하는 현상 발생 --> solver 개선 필요

Solver 개선을 위해 참고자료 학습
* 2022 (Dan et al) A Survey on SPH Methods in Computer Graphics
* 2013 (Markus et al) Implicit Incompressible SPH
* 2006 (Guermond et al) An overview of projection methods for incompressible flows

<br/><br/><br/>

# 2024.07.15
## Rendering 관련 참고자료 학습
* 2009 (Wladimir et al) Screen Space Fluid Rendering with Curvature Flow
* [note-GDC] 2010 (Simon) Screen Space Fluid Rendering for Games

## Particle Rendering using Spherical billboard

spherical billboards를 활용하여 particle rendering 구현.

![2024-07-16 11 10 29](https://github.com/user-attachments/assets/b3ca0729-112b-42fd-94a9-70095319341b)

<br/><br/><br/>


# 2024.07.12
## Rendering 관련 참고자료 학습
* 2009 (Wladimir et al) Screen Space Fluid Rendering with Curvature Flow
* [note-GDC] 2010 (Simon) Screen Space Fluid Rendering for Games

## Spherical billboards rendering
spherical billboards 구현 완료

![Spherical billboards](https://github.com/user-attachments/assets/227ba3d1-679b-4b86-a9d3-2e30abe2c566)

이를 Particle에 적용하게 확장 필요.

## 바닥 rendering 
사각형에 texture를 입혀 간단하게 바닥을 rendering

## Camera 구현
시점에 따른 rendering 할 수 있게 camera class 구현 완료

<br/><br/><br/>

# 2024.07.11
## dt 관련 참고자료 찾기
* 2013 (Markus et al) Implicit Incompressible SPH

## Rendering 관련 참고자료 학습
* 2009 (Wladimir et al) Screen Space Fluid Rendering with Curvature Flow
* [note-GDC] 2010 (Simon) Screen Space Fluid Rendering for Games

particle을 sphere로 rendering하기 위해 spherical billboards를 활용하는 방법 학습 중

<br/><br/><br/>

# 2024.07.10
## Neighbor Search - basic uniform grid 구현

### Key Idea
3차원 Domain의 bounding box를 일정한 크기를 갖는 정육면체인 grid cell로 나누어서 관리하는 데이터 구조이다.

uniform grid를 사용하면, 각 grid cell마다 자신의 영역안에 포함된 particle들을 저장한다.

neighborhood를 검색을 위한 기존의 검색범위(모든 particle)를 줄이기 위해 uniform grid는 다음과 같은 방법을 사용한다.

* 자신이 속한 grid cell을 찾는다.
* grid cell의 Neighbor grid cell의 집합을 찾는다.
* Neighbor grid cell 속한 particle들로 검색 범위를 줄인다.

### 구현
grid cell은 고유의 grid cell index를 갖고 particle은 고유의 particle index를 갖는다.

따라서, grid cell마다 속한 particle을 저장하는 데이터 구조를 `grid cell index마다 속한 particle의 index를 저장하는 이중 배열`로 구현한다.

### Update 개선
물리적으로 연속성을 띄게 particle들이 움직이기 때문에, 매 Frame마다 grid cell index가 바뀌는 particle들은 많지 않다.

따라서, 매 frame마다 이중 배열을 초기화하고 모든 particle들의 index data를 다시 넣는 방식은 매우 비효율적이다.

이를 개선하기 위해, `particle index마다 이전에 속했던 grid cell index를 저장하는 배열`을 추가하여, 이전 Frame과 현재 Frame에서 바뀐 particle의 정보만 반영하도록 하였다.

```cpp

  // update gcid_to_pids
  for (size_t pid = 0; pid < num_particles; ++pid)
  {
    const auto& v_xi = pos_vectors[pid];

    const auto prev_gcid = _pid_to_gcid[pid];

    const auto cur_gcid = this->grid_cell_index(v_xi);

    if (prev_gcid != cur_gcid)
    {
      //erase data in previous
      auto& prev_pids = _gcid_to_pids[prev_gcid];
      prev_pids.erase(std::find(prev_pids.begin(), prev_pids.end(), pid));

      //insert data in new
      auto& new_pids = _gcid_to_pids[cur_gcid];
      new_pids.push_back(pid);

      //update gcid
      _pid_to_gcid[pid] = cur_gcid;
    }
  }

```

### vector 객체 생성 문제
각 Particle마다 주변의 grid cell에 속한 모든 particle index를 받을 때, vector를 return 하니 vector 객체를 생성하는데 많은 cost가 든다.

```cpp
std::vector<size_t> search(const Vector3& pos) const override;
```

![vector allocation](https://github.com/rla523at/SPH_Project/assets/60506879/b5e240b3-eee6-44e5-b615-cba57de25206)


이를 해결하기 위해, neighbor_index를 받아오는데 사용하기 위한 미리 생성된 배열을 만들어 두고 search 함수 형태를 바꿨다.

```cpp
std::vector<size_t> _neighbor_indexes;

  //fill neighbor particle ids into pids and return number of neighbor particles
  size_t                     search(const Vector3& pos, size_t* pids) const override;
```

![vector allocation개선](https://github.com/rla523at/SPH_Project/assets/60506879/a4bef065-2e84-4337-8675-2cb973016749)


결론적으로 1000 Particle 기준 370 FPS -> 450 FPS로 성능 개선되었다.


### 병렬화 문제

**문제점**

Vector 객체를 생성할 때, _neighbor_index를 받아오는데 병렬화를 하게 되면 race condition이 발생하게 된다.

이를 해결하기 위해, thread마다 고유의 배열을 갖게 수정하였다.

```cpp
  std::vector<std::vector<size_t>> _thread_neighbor_indexes;
```

결론적으로, 병렬화가 가능해 졌고, 병렬화를 하면 8000Particle 기준 400FPS까지 나온다.

### 중복 업데이트 문제
neighbor를 계산하는 과정이 현재는 density 계산할 때 한번, force 계산할 때 한번 검사해서 중복 검사가 발생한다.

Uniform Grid에 `particle index마다 neighbor particle의 index를 저장하는 이중 배열`을 추가하고 uniform grid를 업데이트할 때, 한번만 업데이트 되고, search 함수에서는 업데이트된 data만 전달하는 형태로 바꿨다.

```cpp
std::vector<std::vector<size_t>> _pid_to_neighbor_pids;

const std::vector<size_t>& search(const size_t pid) const override; //업데이트된 정보만 전달
```

그러면, 중복 계산을 제거할 수 있으면 추가적으로 기존의 particle 관련 함수에서 거리를 비교하는 부분을 제거할 수 있다.

```cpp
    for (int j = 0; j < num_neighbor; j++)
    {
      const size_t neighbor_index = neighbor_indexes[j];

      const auto& v_xj = _position_vectors[neighbor_index];

      const float dist = (v_xi - v_xj).Length();
      const auto  q    = dist / h;

      //if (q > 2.0f)
      //  continue;

      cur_rho += m0 * W(q);
    }
```

결론적으로, 8000Particle 기준 400FPS -> 600FPS로 개선하였다.
![2024-07-11 12 28 26](https://github.com/rla523at/SPH_Project/assets/60506879/21b8d312-e91f-44ba-ab0b-0a3c3b466021)

<br/><br/><br/>

# 2024.07.09

## Vsync 제거
실제 FPS를 알기 위해 Vsync를 제거.

1331 Particle 기준 134FPS

## SPH 코드 성능 개선
$dt = 1.0e-3$인 상황에서 134FPS로는 부자연스러워 FPS를 높이기 위해 성능 개선 시도.

Neighbor Search까지 CPU로 작업 후, 성능 개선이 더 필요한 경우 GPU 계산을 활용할 예정.

### Neighbor Search
2011 (Markus et al)  A paralle SPH implementation on multi-core CPUs 학습

### Cache 친화적인 Data 구조 도입

메인 계산 루틴중 update_density는 position 정보만, update_pressure는 density 정보만 필요.

하지만 기존 Data 구조상 position 정보와 density 정보는 띄엄 띄엄 떨어져 있음.

메모리상 spatial locality를 높여 속도를 개선하는 시도를 함.

![메모리](https://github.com/rla523at/SPH_Project/assets/60506879/3d4e7863-43cd-4c68-9e76-1a106c93270c)


**결과**

1331Particle 기준 134 FPS -> 139 FPS로 아주 약간 개선됨.

유의미한 변화가 아니였던 이유를 유추해보면 기존 Particle 구조체가 44byte이고 시스템 L1캐시가 1.4MB니까 약 32000개 이상의 Particle이 있었으면 조금 더 유의미한 변화가 있었을 것 같다.

### Open MP를 통한 병렬화
모든 계산 루틴이 data dependency가 없어서 병렬화가 가능한 for loop이기 때문에 openMP를 이용한 for loop 병렬화를 시도.

**결과**

1331Particle 기준 1000 FPS정도 나옴.

$dt = 1.0e-3$인 상황에서 400FPS 이상 나와야 실제 물 처럼 거동을 하며, 현재 2197Particle 기준 400FPS 정도 나옴

![2197particle 400FPS](https://github.com/rla523at/SPH_Project/assets/60506879/c417e26f-5a07-401e-9c41-f1a28ed32adc)

<br/><br/><br/>

# 2024.07.08
* SPH 코드 개발
  * 질량 계산 방식 수정    
    * 1994 (monaghan) Simulating Free Surface Flows with SPH
  * Smoothing Length 계산 루틴 추가
    * #Neighbor Particle 50 +-이 되게
  * 밀도 값 보정
    * 비 물리적인 Density fluctuation이 생기는 경우 clamp로 밀도값 수정
  * Pressure Constant 보정
    * 논문을 기반으로 simulation 결과가 타당해 보이게 약간의 조정
    * 2007 (Markus and Matthias) Weakly compressible SPH for free surface flows
  * 3차원으로 확장 (XYZ 2m 2m 1m 박스에, XYZ 1m 1m 1m 물이 박스형태로 있는 상황 시뮬레이션) 
 
  ![2024-07-09 10 50 47](https://github.com/rla523at/SPH_Project/assets/60506879/dfa4d791-71f1-468d-8445-ab75995fdac2)

* SPH Neighbor Search
  * Neighbor search를 brute force로 하고 있기 때문에 #Particle이 1000 +- 정도가 한계
  * 관련 참고 자료 찾기
    * 2011 (Markus et al)  A paralle SPH implementation on multi-core CPUs
    * 2003 (Matthias et al) Optimized Spatial Hashing for Collision Detection of Deformable Objects

<br/><br/><br/>

# 2024.07.05
* 참고 자료 학습
  * 2003 (Matthias and Markus) Particle-Based Fluid Simulation for Interactive Applications
  * 2007 (Markus and Matthias) Weakly compressible SPH for free surface flows
  * 2014 (Markus et al) SPH Fluids in Computer Graphics

* SPH simulation Rendering 관련 참고 자료 찾기
  * [note-GDC] 2010 (Simon) Screen Space Fluid Rendering for Games
  * 2009 (Wladimir et al) Screen Space Fluid Rendering with Curvature Flow

* SPH 코드 개발
  * 밀도 값이 이상한 문제(미해결)
    * 예상되는 문제점들
      * 상수 값
        * Pressure Constant
        * Smoothing Length
        * Particle Radius      
        * Number of Particles
      * Kernel 함수
        * 논문은 3차원 기준으로 작성
        * 상수배 차이이지만 혹시...      
  * 힘이 너무 큰 문제(미해결)
    * Pressure driven force가 너무 크다. 
    * gradient kernel 값이 너무 크다.

* 2주차 계획 설정
  * 2차원 개발 후 3차원 확장 고려 --> 3차원 확장후 문제 해결을 고려
    * 논문이 전부 3차원 기준으로 작성
    * SPH 특성상 2차원 개발과 3차원이 크게 다르지 않음
    * 문제의 원인이 될 변수를 줄이기 위해(e.g. kernel 함수) 바로 3차원으로 확장 후 문제 해결을 고려

<br/><br/><br/>

# 2024.07.04
* 참고 자료 학습
  * 2014 (Markus et al) SPH Fluids in Computer Graphics.pdf
    * viscosity coefficient
  * 2007 (Markus and Matthias) Weakly compressible SPH for free surface flows
    * pressure coefficient
    * time step

* SPH 코드 개발
  * 2014 논문 기반으로 update 함수 개발 완료
  * 밀도 값이 이상한 문제(미해결)
    * 밀도 계산 시, 기준 밀도 보다 작아지는 현상
      * 입자가 혼자 있을 떄, 초기 밀도보다 작아짐 --> 비 물리적
      * **mass 계산을 kernel에 맞춰 수정하면 해결 가능** 
    * 밀도 계산 시, 밀도가 너무 커지는 현상 --> 밀도 변화율이 1%이상 --> 비압축성 가정에 위배
      * kernel 계산 값이 너무 큼
      * **임시방편으로 upper bound 걸어놓음**
  * 힘이 너무 큰 문제(미해결)     
    * 초기 조건이 잘못된건지 확인 필요  
    * 중력보다 Pressure driven force가 너무 커서 물이 떨어지지 않고 폭발함 
    ![2024-07-05 11 14 20](https://github.com/rla523at/SPH_Project/assets/60506879/f3e74559-92f8-46e9-921d-951e026e9114)

<br/><br/><br/>

# 2024.07.03
* 참고 자료 학습
  * 2014 (Markus et al) SPH Fluids in Computer Graphics.pdf
  * Interpolation in SPH
  * Kernel fuction
  * spatial derivatives 
  * pressure computation - Equation Of State

* 기반코드 개발
  * D3D11CreateDevice 함수 실패 문제 (해결)
    * DXGI_ERROR_SDK_COMPONENT_MISSING 오류
    * D3D11_CREATE_DEVICE_DEBUG flag를 쓰고 debug layer 버전이 부적절해서 발생
    * visual studio installer에서 최신 SDK 설치로 해결

* SPH 코드 개발
  * 2014 논문 기반으로 update 함수 개발 중

<br/><br/><br/>

# 2024.07.02
* 주제 선정 
  * SPH simulation

* 참고 자료 찾기
  * 2014 (Markus et al) SPH Fluids in Computer Graphics.pdf
  * 2019 (Dan et al) SPH Techniques for the Physics Based Simulation of Fluids and Solids
  * https://github.com/InteractiveComputerGraphics/SPlisHSPlasH
  * https://sph-tutorial.physics-simulation.org/
  * https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-30-real-time-simulation-and-rendering-3d-fluids

* 참고 자료 학습
  * 2014 (Markus et al) SPH Fluids in Computer Graphics.pdf
  * 이 리뷰 논문을 기반으로 구현 예정

# 2024.07.01
* 주제 정하기
