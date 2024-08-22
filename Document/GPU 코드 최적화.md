# GPU 코드 최적화
GPU 코드를 최적화 하기 위해 ID3D11Query를 활용한 계산시간 측정 코드를 작성한 뒤 2Box Simulation를 기준으로 성능 측정을 하였다.

최적화 전 Frame당 계산시간은 다음과 같다.

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

| Description ||
|------|:---:|
|dt|0.01 (s)|
|FPS|100-120|
|Simulation Time / Computation Time  | 1.0 - 1.2 |

![DoubleDamBreaking0 05-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/53c1e83b-9471-4b23-b83b-a35a692140f5)


## Read Back 최적화
GPU에서 CPU로 read back이 존재하는 경우, 다음과 같은 비효율이 발생하여 계산 시간에 악영향을 준다.

![before_opt](https://github.com/user-attachments/assets/56cb079b-a360-42c2-81d7-fe55996ddf7e)

* CPU는 Produce Work 후에도 Read Data나 다음 Produce Work를 수행하지 못하고 GPU의 Consume Work을 기다려야 한다.
* GPU는 Consume Work 후에도 CPU가 Read Data와 Produce Work를 수행하는 동안 기다려야 한다.

만약 read back을 없앨 수 있다면 앞의 비효율 없이 CPU는 Produce Work를 GPU는 Consume Work를 비동기적으로 수행할 수 있다.

![opt](https://github.com/user-attachments/assets/48f538d1-1a24-4268-979b-c309b787b65e)

따라서, read back을 없애는데 드는 비용이 비효율에 의한 비용보다 작다면 read back을 없애는 것이 낫다.

예를 들어, PCISPH 알고리즘 중에, iteration 중단조건을 판단하기 위해 GPU에서 CPU로 read back하는 코드가 있었다.

이 때, read back 코드를 없애는 대신에 max iteration만큼 추가로 iteration을 진행하게 수정하였고, read back을 없애는데 추가된 계산 시간(추가 iteration)보다 비효율이 없어지면서 줄어든 계산 시간이 훨씬 커 성능을 개선하였다.

![Read Back 최적화](https://github.com/user-attachments/assets/0829effd-8cde-4a5f-994c-a728343f79fe)

## Thread 계산 부하 최적화
Thread 계산 부하에 따라서 병렬화 효율의 차이가 발생하여 계산 시간에 큰 영향을 준다.

Thread당 연산부하가 너무 크면 Thread가 충분히 활용되지 못해 병렬화 효율이 떨어질 수 있고 Thread당 연산부하가 너무 작으면 동기화 및 병렬화를 위한 부가 작업 비중이 커져 병렬화 효율이 떨어질 수 있다.

따라서, 상황에 맞게 Thread당 적절한 계산 부하를 갖도록 최적화해야 한다.

예를 들어, 기존에는 한 Thread당 neighbor particle개수 만큼 연산을 수행하는 코드가 있었다.

이 경우에는 neighbor particle의 개수가 최대 200개 까지 될 수 있어 한 Thread당 연산부하가 너무 커 병렬화 효율이 떨어지는 상황이였다.

이를 해결하기 위해, Thread Group당 neighbor particle에 대한 연산을 수행하고 Thread Group내의 Thread는 1개의 neighbor particle에 대한 연산을 수행하도록 수정하였다.

그러나 이 경우에는 Thread별 계산 부하가 너무 작아 동기화 및 병렬화를 위한 부가 작업이 더 계산 작업을 차지하는 상황이였다.

이를 해결하기 위해, Thread당 N개의 neighbor particle에 대한 연산을 수행하도록 수정하였고 함수에 따라서 N을 바꿔가면서 최적의 계산 부하를 찾아 성능을 개선하였다.

![update_number_density](https://github.com/user-attachments/assets/aa4fed50-de4e-43d7-b0c8-a36b1c0d3817)

## 메모리 접근 최적화
일반적으로 register memory에 접근하는 비용은 $O(1)$ clock, group shared memory는 $O(10)$ clock, global memory는 $O(10^2)$ clock이라고 한다.

따라서, 메모리 접근 비용을 낮출 수 있게 최적화 해야 한다.

예를 들어, global memory에 여러번 접근하는 경우에는 groupshared memory를 활용하고 동기화 과정을 거치게 수정하여 성능을 개선하였다.

![update_nfp2](https://github.com/user-attachments/assets/c026aa73-0eed-4cb6-afd3-a8d906da0b1e)

또한, groupshared memory 대신에 register 변수를 사용게 수정하여 성능을 개선하였다.

![메모리접근최적화](https://github.com/user-attachments/assets/ef722e9e-e436-47e9-9225-fb88d1678dc9)

## Thread 활용 최적화
Group 단위로 GPU가 동작하기 때문에, Group 내부에 thread 활용도를 높일수록 있도록 최적화 해야 한다.

기존에 Group당 64개의 Thread를 할당하여, i번째 thread가 모든 Neighbor Geometry Cell에 있는 i번째  particle들에 대한 연산을 수행하는 코드가 있었다.

Geometry Cell에는 최대 64개 까지 Particle이 존재 할 수 있기 때문에 64개의 Thread를 부여하였지만, 일반적으로 약 20-30개의 Particle이 존재함으로 활용되지 않는 Thread가 많이 발생하였다.

이를 개선하기 위해 Group당 32개의 Thread를 할당하여, i번째 thread가 i번째 neighbor geometry cell에 있는 모든  particle들에 대한 연산을 수행하게 수정하였다.

이 수정으로 인해 Thread당 연산부하가 최악의 경우 27개의 particle 연산에서 64개의 particle 연산으로 증가하지만 이런 경우가 많지 않고, 활용되지 않는 thread의 수를 줄여 성능을 개선하였다.

![update_nfp1](https://github.com/user-attachments/assets/fa8df285-cd85-4821-b94b-74ea6edecf5b)

## GPU 코드 최적화 결과
최종적으로 개선전 대비 약 `54%` 계산 시간을 감소시켰다.

```
PCISPH_GPU Performance Analysis Result Per Frame         개선전           개선후
====================================================    =============    ==============                                                    
_dt_avg_update                                           8.65898   ms    3.94271    ms                                                    
====================================================    =============    ==============                                                    
_dt_avg_update_neighborhood                              2.37403   ms    1.09558    ms                                                    
_dt_avg_update_number_density                            0.345017  ms    0.172424   ms     
_dt_avg_update_scailing_factor                           0.018452  ms    0.017785   ms                                                    
_dt_avg_init_fluid_acceleration                          1.48357   ms    0.270125   ms                                                    
_dt_avg_init_pressure_and_a_pressure                     0.00507673ms    0.00504521 ms                                                    
_dt_avg_copy_cur_pos_and_vel                             0.0191895 ms    0.0187321  ms                                                    
_dt_avg_predict_velocity_and_position                    0.0428115 ms    0.0388008  ms                                                    
_dt_avg_predict_density_error_and_update_pressure        0.678709  ms    0.558781   ms                                                    
_dt_avg_cal_max_density_error                            0         ms    0          ms                                                    
_dt_avg_update_a_pressure                                1.54367   ms    0.819914   ms                                                    
_dt_avg_apply_BC                                         0.00599321ms    0.00597577 ms                                                    
====================================================    =============    ==============
```

| |개선전|개선후|
|------|:---:|:---:|
|dt(s)|0.01|0.01|
|FPS|100-120|290-300|
|Simulation Time / Computation Time  | 1.0 - 1.2 |2.9 - 3.0|

https://github.com/user-attachments/assets/d446a2fd-3a06-49d7-aa58-7c38a04ca3e3

## Thread 활용 최적화(Chunk)

### [문제]
Group당 하나의 particle의 neighbor particle에 대한 연산을 할당하고, 최대 neighbor particle 개수 때문에 Group당 128개의 Thread를 할당하였다.

하지만 일반적으로 neighbor particle의 수는 약 50개로 기존 방식에서는 평균적으로 약 100개에 가까운 Thread가 활용되지 않는 문제가 있었다.

### [해결]
이를 해결하기 위해 neighbor particle의 개수가 256개 보다 작으면서 가능한 많은 particle들을 묶어서 Chunk를 만들었다.

다음으로 group당 하나의 chunk의 neighbor particle에 대한 연산을 할당하여 대부분의 Thread가 활용될 수 있게 개선하였다.

또한, 하나의 Thread가 N개의 neighbor particle에 대해서 연산을 수행하게 하였고 최적의 연산부하를 갖도록 N을 조절하였다.

### [개선점]
Chunk를 활용하기 위해 매 Frame마다 chunk를 계산하는 비용이 추가로 든다.

현재 성능측정 결과는 다음과 같다.
```
PCISPH_GPU Performance Analysis Result per frame    기존          Chunk
=================================================  ==========    ===========  ==
_dt_avg_update                                      3.94271       4.32952     ms
=================================================  ===========   ===========  ==
_dt_avg_update_neighborhood                         1.09558       1.09088     ms
_dt_avg_update_number_density                       0.172424      0.123518    ms +
_dt_avg_update_scailing_factor                      0.017785      0.0178141   ms 
_dt_avg_init_fluid_acceleration                     0.270125      0.188962    ms +
_dt_avg_init_pressure_and_a_pressure                0.00504521    0.00501129  ms 
_dt_avg_copy_cur_pos_and_vel                        0.0187321     0.0186535   ms 
_dt_avg_predict_velocity_and_position               0.0388008     0.0389623   ms 
_dt_avg_predict_density_error_and_update_pressure   0.558781      0.457579    ms +
_dt_avg_cal_max_density_error                       0             0           ms
_dt_avg_update_a_pressure                           0.819914      0.627285    ms +
_dt_avg_apply_BC                                    0.00597577    0.00590625  ms
_dt_avg_update_nbr_chunk_end_index                                0.00625649  ms -
_dt_avg_init_nbr_chunk                                            0.686535    ms -
=================================================                ===========  ==
```

Chunk를 사용함으로써 4개의 함수에서 총 0.45ms의 계산시간을 줄였지만, chunk를 계산하는 비용 약 0.7ms가 추가되어 계산시간이 오히려 증가하였다.

Chunk를 계산하는데 드는 비용을 개선할 필요가 있다.

## 데이터 구조 최적화
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