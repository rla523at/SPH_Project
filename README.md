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
* 2단계 (pressure을 제외한 acceleration을 계산) GPU 코드 작성 중

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
