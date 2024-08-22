# SPH 프로젝트

## 기간
2024.07.01 ~ 2024.08.22

## 목표
Smoothed Particle Hydronamics(SPH)를 이용하여 물을 물리 시뮬레이션하고 그 결과를 렌더링한다.

## 결과

**[Dam Breaking Simulation]**
| Description ||
|------|:---:|
|Solution Domain(XYZ)|4 X 6 X 0.6 (m)|
|Inital Water Box Domain(XYZ)|1 X 2 X 0.4 (m)|
|intial spacing|0.025 (m)|
|#particle|~56000(55760)|
|dt|0.005 (s)|
|FPS|180-200|
|Simulation Time / Computation Time  | 0.9 - 1.0 |


https://github.com/user-attachments/assets/79b625f8-08e8-4921-9c3a-f678012b0b2e


**[2 Box Simulation]**

| Description ||
|------|:---:|
|Solution Domain(XYZ)|3 X 6 X 3 (m)|
|Inital Water Box Domain(XYZ)|1 X 2 X 1, 1 X 2 X 1 (m)|
|intial spacing|0.035 (m)|
|#particle|~100000(97556)|
|dt|0.008 (s)|
|FPS|100-120|
|Simulation Time / Computation Time  | 0.8 - 0.96 |

https://github.com/user-attachments/assets/f526d379-1bda-407c-8e3b-f91d6302bd47


**[4 Box Simulation]**

| Description ||
|------|:---:|
|Solution Domain(XYZ)|3 X 6 X 3 (m)|
|Inital Water Box Domain(XYZ)|1 X 1 X 1, 1 X 2 X 1, 1 X 2 X 1, 1 X 3 X 1 (m)|
|intial spacing|0.04 (m)|
|#particle|~140000(137904)|
|dt|0.01 (s)|
|FPS|70-90|
|Simulation Time / Computation Time  | 0.7 - 0.9 |


https://github.com/user-attachments/assets/af6ce26a-3918-4c8d-a0f4-892266cb3743



**[Long Box Simulation]**

| Description ||
|------|:---:|
|Solution Domain(XYZ)|4 X 20 X 4 (m)|
|Inital Water Box Domain(XYZ)|0.8 X 15 X 0.6 (m)|
|intial spacing|0.05 (m)|
|#particle|~67000(66521)|
|dt|0.006 (s)|
|FPS|150-170|
|Simulation Time / Computation Time  | 0.9 - 1.12 |


https://github.com/user-attachments/assets/04a60201-abd8-403d-a4be-05ab524502cb

<br><br>

## 프로젝트 설명

### 물리시뮬레이션
물을 물리시뮬레이션 하기 위해 다음의 비압축성 유체의 linear momentum equation을 수치해석적으로 푼다.

$$ \frac{dv}{dt} = -\frac{1}{\rho}\nabla p + \nu \nabla^2u + f_{ext} $$

SPH frame work에서는 유한개의 particle(approximation point)과 kernel 함수 $W$를 이용하여 임의의 물리량을 근사한다.

이 frame work을 통해 비압축성 유체의 linear momentum equation의 우측항(RHS)을 이산화하면 다음과 같다.

$$ \frac{dv}{dt} = f(v,x) $$

$$ f(v,x) = - m_0 \sum_j (\frac{p_i}{\rho_i^2} + \frac{p_j}{\rho_j^2}) \nabla W_{ij} + 2 \nu (d+2) m_0 \sum_j \frac{1}{\rho_j} \frac{v_{ij} \cdot x_{ij}}{x_{ij} \cdot x_{ij}+0.01h^2} \nabla W_{ij} + g $$

이후에 계산된 RHS값과 수치적분을 통해 속도와 위치를 업데이트하면 한 time step($\Delta t)$에 대한 수치해석이 끝이난다.

$$ \begin{aligned}
\frac{v^{n+1} - v^{n}}{\Delta t} &= f(v^n,x^n) \\
\frac{x^{n+1} - x^{n}}{\Delta t} &= v^{n+1}  
\end{aligned} $$

**[수치적분]**

논문을 통해, explicit scheme 중 semi implicit euler과 leap frog 방법이 가장 많이 사용되어 두 scheme의 stability를 비교하였으나 차이는 없었다.

그래서, 계산과정이 더 단순하여 성능상에 이점이 있는 semi implcit euler scheme을 프로젝트에 사용하였다.

<br>

### SPH Scheme
SPH를 이용한 수치해석 과정에서 가장 핵심이 되는 부분은 비압축성 가정을 만족하는 압력을 결정하는 방법이다.

압력 결정 방법에 따라 다양한 SPH기법이 존재하며 이번 프로젝트에서는 linear solver가 필요없는 기법인 Weakly Compsressible SPH(WCSPH)와 Prediction Correction Incompressible SPH(PCISPH)를 사용하였다.

압력을 간단한 수식으로 계산할 수 있어, solver가 단순하고 압력 계산을 매우 빠르게 계산할 수 있다는 장점이 있는 WCSPH를 구현하였지만  $\Delta t$가 매우 작은 값만 가능한 WCSPH의 한계로 인해, PCISPH을 구현 및 적용하였으며 결론적으로 동일 simulation 기준 $\Delta t$를 `10배 증가`시킬 수 있었다.

<br>

## GPU 코드 최적화
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


### Read Back 최적화
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

### Thread 계산 부하 최적화
Thread 계산 부하에 따라서 병렬화 효율의 차이가 발생하여 계산 시간에 큰 영향을 준다.

Thread당 연산부하가 너무 크면 Thread가 충분히 활용되지 못해 병렬화 효율이 떨어질 수 있고 Thread당 연산부하가 너무 작으면 동기화 및 병렬화를 위한 부가 작업 비중이 커져 병렬화 효율이 떨어질 수 있다.

따라서, 상황에 맞게 Thread당 적절한 계산 부하를 갖도록 최적화해야 한다.

예를 들어, 기존에는 한 Thread당 neighbor particle개수 만큼 연산을 수행하는 코드가 있었다.

이 경우에는 neighbor particle의 개수가 최대 200개 까지 될 수 있어 한 Thread당 연산부하가 너무 커 병렬화 효율이 떨어지는 상황이였다.

이를 해결하기 위해, Thread Group당 neighbor particle에 대한 연산을 수행하고 Thread Group내의 Thread는 1개의 neighbor particle에 대한 연산을 수행하도록 수정하였다.

그러나 이 경우에는 Thread별 계산 부하가 너무 작아 동기화 및 병렬화를 위한 부가 작업이 더 계산 작업을 차지하는 상황이였다.

이를 해결하기 위해, Thread당 N개의 neighbor particle에 대한 연산을 수행하도록 수정하였고 함수에 따라서 N을 바꿔가면서 최적의 계산 부하를 찾아 성능을 개선하였다.

![update_number_density](https://github.com/user-attachments/assets/aa4fed50-de4e-43d7-b0c8-a36b1c0d3817)

### 메모리 접근 최적화
일반적으로 register memory에 접근하는 비용은 $O(1)$ clock, group shared memory는 $O(10)$ clock, global memory는 $O(10^2)$ clock이라고 한다.

따라서, 메모리 접근 비용을 낮출 수 있게 최적화 해야 한다.

예를 들어, global memory에 여러번 접근하는 경우에는 groupshared memory를 활용하고 동기화 과정을 거치게 수정하여 성능을 개선하였다.

![update_nfp2](https://github.com/user-attachments/assets/c026aa73-0eed-4cb6-afd3-a8d906da0b1e)

또한, groupshared memory 대신에 register 변수를 사용게 수정하여 성능을 개선하였다.

![메모리접근최적화](https://github.com/user-attachments/assets/ef722e9e-e436-47e9-9225-fb88d1678dc9)

### Thread 활용 최적화
Group 단위로 GPU가 동작하기 때문에, Group 내부에 thread 활용도를 높일수록 있도록 최적화 해야 한다.

기존에 Group당 64개의 Thread를 할당하여, i번째 thread가 모든 Neighbor Geometry Cell에 있는 i번째  particle들에 대한 연산을 수행하는 코드가 있었다.

Geometry Cell에는 최대 64개 까지 Particle이 존재 할 수 있기 때문에 64개의 Thread를 부여하였지만, 일반적으로 약 20-30개의 Particle이 존재함으로 활용되지 않는 Thread가 많이 발생하였다.

이를 개선하기 위해 Group당 32개의 Thread를 할당하여, i번째 thread가 i번째 neighbor geometry cell에 있는 모든  particle들에 대한 연산을 수행하게 수정하였다.

이 수정으로 인해 Thread당 연산부하가 최악의 경우 27개의 particle 연산에서 64개의 particle 연산으로 증가하지만 이런 경우가 많지 않고, 활용되지 않는 thread의 수를 줄여 성능을 개선하였다.

![update_nfp1](https://github.com/user-attachments/assets/fa8df285-cd85-4821-b94b-74ea6edecf5b)

### GPU 코드 최적화 결과
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

### Thread 활용 최적화(Chunk)

#### [문제]
Group당 하나의 particle의 neighbor particle에 대한 연산을 할당하고, 최대 neighbor particle 개수 때문에 Group당 128개의 Thread를 할당하였다.

하지만 일반적으로 neighbor particle의 수는 약 50개로 기존 방식에서는 평균적으로 약 100개에 가까운 Thread가 활용되지 않는 문제가 있었다.

#### [해결]
이를 해결하기 위해 neighbor particle의 개수가 256개 보다 작으면서 가능한 많은 particle들을 묶어서 Chunk를 만들었다.

다음으로 group당 하나의 chunk의 neighbor particle에 대한 연산을 할당하여 대부분의 Thread가 활용될 수 있게 개선하였다.

또한, 하나의 Thread가 N개의 neighbor particle에 대해서 연산을 수행하게 하였고 최적의 연산부하를 갖도록 N을 조절하였다.

#### [개선점]
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

## Detail

### 물리시뮬레이션

물을 물리시뮬레이션 하기 위해 다음의 비압축성 유체의 linear momentum equation을 푼다.

$$ \frac{du}{dt} = -\frac{1}{\rho}\nabla p + \nu \nabla^2u + f_{ext} $$

위의 미분방정식을 이산적으로 풀기 위해 SPH frame work의 근사식을 사용한다.

SPH frame work에서는 유한개의 particle(approximation point)과 kernel 함수 $W$를 이용하여 임의의 물리량 $f$를 다음과 같이 근사한다.

$$ f(x) = \sum_j \frac{m_j}{\rho_j}f(x_j) W(x-x_j,h) $$

이 approximation을 통해 다음 단계를 거쳐 위의 미분방정식을 푼다.

*  SPH approximation을 통해 $\rho$를 계산한다.

$$ \rho(x) = \sum_j m_jW(x,x_j,h) $$

* 계산된 $\rho$를 통해 비압축성 가정을 만족하는 $p$를 계산한다.

$$ P = f(\rho) $$

*  linear momentum equation의 RHS를 계산한다.

$$ -\frac{1}{\rho_i}\nabla p_ = - m_0 \sum_j (\frac{p_i}{\rho_i^2} + \frac{p_j}{\rho_j^2}) \nabla W_{ij} $$

$$ \nu \nabla^2u = 2 \nu (d+2) m_0 \sum_j \frac{1}{\rho_j} \frac{v_{ij} \cdot x_{ij}}{x_{ij} \cdot x_{ij}+0.01h^2} \nabla W_{ij}  $$


### Time Integration

**[semi implicit euler]**

$$ \begin{aligned}
\frac{v^{n+1} - v^{n}}{\Delta t} &= f(v^n,x^n) \\
\frac{x^{n+1} - x^{n}}{\Delta t} &= v^{n+1}  
\end{aligned} $$

**[leap frog - KDK]**

$$ \begin{aligned}
\frac{v^{n+1/2} - v^{n}}{0.5 \Delta t} &= f(v^n,x^n)\\
\frac{x^{n+1} - x^{n}}{\Delta t} &= v^{n+1/2}  \\
\frac{v^{n+1} - v^{n+1/2}}{0.5 \Delta t} &= f(v^{n+1/2},x^{n+1})\\
\end{aligned} $$

**[leap frog - DKD]**

$$ \begin{aligned}
\frac{x^{n+1/2} - x^{n}}{0.5 \Delta t} &= v^{n}  \\
\frac{v^{n+1} - v^{n}}{\Delta t} &= f(v^n,x^{n+1/2})\\
\frac{x^{n+1} - x^{n+1/2}}{0.5 \Delta t} &= v^{n+1}  \\
\end{aligned} $$


### WCSPH

WCSPH에서는 압력을 계산하기 위해 Tait's equation이라는 가상의 equation of state를 사용한다.

$$ p = k ((\frac{\rho}{\rho_0})^7 - 1) $$

압력을 간단한 계산으로 매우 빠르게 구할 수 있다는 장점이 있지만, 비압축성을 부과하기 위해 작은 밀도변화에도 압력 변화가 크게 발생하게 디자인되어 있다$(k\sim O(10^6))$.

그 결과 작은 밀도 변화에도 강한 압력이 발생해 서로 밀어내는 힘이 작용함으로 밀도 변화가 작게 유지되게 하는 역할을 하지만 동시에 $\Delta t$가 조금만 커져도 강한 압력에 의해 position과 velocity에 매우 큰 변화가 생긴다.

따라서, $\Delta t$가 매우 제한되는 문제가 생긴다.

### PCISPH
PCISPH에서는 WCSPH와 다르게 prediction-correction step을 거치면서 비압축성 가정을 만족하는 압력을 찾는다.

![PCISPH알고리즘](https://github.com/user-attachments/assets/1f8a1af7-79b1-4254-bd76-4117e63c8a95)

현재 상태에서 position과 velocity를 predict하고 이 때 비압축성 가정에 발생한 오류를 줄이기 위해 pressure를 correction하는 스텝으로 되어 있다.

PCISPH의 핵심 수식은 비압축성 가정을 위반하는 $\rho_{err} = \rho - \rho_0$라는 밀도장의 오류를 해결하기 위해 필요한 압력을 계산할 수 있는 다음 수식이다.

$$ p_{correct} = \frac{-\rho_{err}}{\beta( - (\sum_j\nabla W_{ij}) \cdot (\sum_j\nabla W_{ij}) - \sum_j (\nabla W_{ij} \cdot \nabla W_{ij}))} $$

핵심 수식을 유도하기 위해 다양한 가정을 함으로 단 한번의 계산으로 정확히 필요한 압력을 계산할 수는 없지만 iteration을 통해 일정 수준의 incompressibility를 만족하게 할 수 있다.

#### 핵심 수식 Detail
SPH descritization에 의해서 다음이 성립한다.

$$ \rho_i^{n+1} = m \sum_j W(x^{n+1}_i - x^{n+1}_j) = m \sum_j W(x^{n}_i - x^{n}_j + \Delta x_i^n - \Delta x_j^n) $$

$\Delta x_i^n - \Delta x_j^n$이 충분히 작아 linear approximation이 가능하다고 가정하면, 다음이 성립한다.

$$ \begin{aligned}
\rho\_i^{n+1} &\approx m \sum\_j (W(x^{n}\_i - x^{n}\_j) + \nabla W(x^{n}\_i - x^{n}\_j) \cdot (\Delta x\_i^n - \Delta x\_j^n)))\\
&= \rho\_i^n + m (\Delta x\_i^n \cdot \sum\_j \nabla W^n\_{ij} - \sum\_j \nabla W^n\_{ij} \cdot \Delta x\_j^n) \\
&= \rho\_i^n + \Delta \rho\_i^n
\end{aligned} $$

이 때, 압력에 의해서만 변위가 바뀐다고 가정 하고 추가적인 가정들을 곁들이면, $\Delta x_i^n$과 $\Delta x_j^n$을 각각 다음과 같이 근사할 수 있다.

$$ \begin{aligned}
    \Delta x_i^n &\approx - \frac{2m p_i^n \Delta t^2}{\rho_0^2} \sum_j \nabla W_{ij} \\
    \Delta x_j^n &\approx  \frac{2m p_i^n \Delta t^2}{\rho_0^2} \nabla W_{ij} \\
\end{aligned} $$

이를 가지고 $\Delta \rho_i^n$를 근사하면 다음과 같다.

$$ \Delta \rho_i^n \approx - \beta p_i^n \left( \sum_j \nabla W_{ij}^n \cdot \sum_j \nabla W^n_{ij} + \sum_j \nabla W^n_{ij} \cdot \nabla W_{ij}^n \right) $$
    
$$ \beta = \frac{2m\Delta t^2}{\rho_0^2} $$

$p_i$에 대해 정리하면 다음과 같다.

$$ p^n_i = \frac{\Delta \rho_i^n}{-\beta( (\sum_j\nabla W_{ij}^n) \cdot (\sum_j\nabla W_{ij}^n) + \sum_j (\nabla W_{ij}^n \cdot \nabla W_{ij}^n))} $$

#### $\Delta x_i^n$ Detail
semi-implicit Euler time integration에 의해, 압력에 의한 변위 $(\Delta x_i^n)_p$는 다음과 같다.

$$ \begin{aligned}
x^{n+1} &= x^n + \Delta t v^n + \Delta t^2 (a\_p(x^n\_i) + a\_{other})  \\
\Delta x^{n}\_i &= \Delta tv^n + \Delta t^2 (a\_p(x^n\_i) + a\_{other}) \\
(\Delta x^{n}\_i)\_p &= \Delta t^2 a\_p(x^n\_i) \\
\end{aligned} $$

이 때, neighbor가 전부 같은 압력을 가지고 있으며($p_i = p_j$), 밀도가 rest density를 갖고 있다고($\rho_i = \rho_j = \rho_0$) 가정하면 압력에 의한 가속도 $a_p(x^n_i)$를 다음과 같이 간단하게 근사할 수 있다.

$$ \begin{aligned}
a_p(x^n_i) &= - m \sum_j (\frac{p_i}{\rho_i^2} + \frac{p_j}{\rho_j^2}) \nabla W_{ij}\\ 
&\approx - \frac{2m p_i}{\rho_0^2}  \sum_j \nabla W_{ij}    
\end{aligned}  $$

이를 semi-implicit Euler time integration에 적용하면 $(\Delta x_i^n)_p$는 다음과 같이 근사할 수 있다.

$$ (\Delta x^{n}\_i)\_p \approx - \frac{2m\Delta t^2 p\_i}{\rho\_0^2}  \sum\_j \nabla W\_{ij} $$

#### $\Delta x_j^n$ Detail
Pressure force는 symmetric 하기 때문에, 위와 동일한 가정을 하면, $p_i$에 의해 $x_j^n$에서 발생하는 가속도는 다음과 같이 근사할 수 있다.

$$ a_{p_i}(x_j^n) \approx \frac{2m p_i}{\rho_0^2} \nabla W_{ij} $$

이 때, $a_p(x_j^n) \approx a_{p_i}(x_j^n)$라고 가정하고 semi-implicit Euler time integration을 적용하면 다음과 같다.

$$ (\Delta x^{n}\_j)\_p \approx - \frac{2m\Delta t^2 p\_i}{\rho\_0^2}  \nabla W\_{ij} $$

단, 여기서 구한 $\Delta x_j^n$는 $x_i$에서 $p_i$와 $\Delta \rho_i$의 관계를 유도하기 위해서 근사한 값일 뿐이다. 따라서, $x_j$에서 $p_j$와 $\Delta \rho_j$의 관계를 유도할 때는 여기서 구한 값을 사용하면 안된다.
