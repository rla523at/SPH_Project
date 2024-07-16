# 2024.07.15
## Rendering 관련 참고자료 학습
* 2009 (Wladimir et al) Screen Space Fluid Rendering with Curvature Flow
* [note-GDC] 2010 (Simon) Screen Space Fluid Rendering for Games

## Particle Rendering using Spherical billboard

spherical billboards를 활용하여 particle rendering 구현.

![2024-07-16 11 03 28](https://github.com/user-attachments/assets/36080d1b-5973-4cbe-8f5c-be716dfbfa4c)


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

neighborhood를 검색하기 위해 기존의 검색범위인 모든 particle을 줄이기 위해 uniform grid는 다음과 같은 방법으로 검색 범위를 줄인다.

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
