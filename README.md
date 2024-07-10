# 2024.07.10
## Neighbor Search - basic uniform grid 구현

### Key Idea
3차원 Domain의 bounding box를 일정한 크기를 갖는 정육면체로 나누어서 관리하는 데이터 구조.

정육면체를 grid cell이라고 하며, grid cell마다 고유의 grid cell index vector $c$를 갖는다.

$$ c = (i,j,k) $$

정육면체의 한 면을 $h$라고 할 때, $h$가 smoothing length와 같다면, neighborhood를 검색하기 위해 최대 27개의 grid cell만 탐색하면 된다. 

2D 예시

[그림]

정육면체의 한 면을 $h$, bounding box의 최소 좌표를 $(x_{min},y_{min},z_{min})$이라고 한다면 bounding box 내부의 point $x$가 속한 grid cell index vector는 다음과 같다.

$$ c(x) = (\left\lceil \frac{x-x_{min}}{h}  \right\rceil, \left\lceil \frac{y-y_{min}}{h}  \right\rceil, \left\lceil \frac{z-z_{min}}{h}  \right\rceil $$

### 병렬화 문제

**문제점**

particle들의 새로운 position이 주어졌을 떄, 다음 두가지 배열을 업데이트 해야 된다.
* grid cell index마다 속한 particle의 index를 저장하는 이중 배열
* particle index마다 neighbor particle의 index를 저장하는 이중 배열

두 배열 모두, 기존 데이터를 지우고 새로운 데이터를 추가하는 과정이 필요하기 때문에 data dependency가 발생해 병렬화가 어렵다.

**해결 방안**

grid cell index마다 속한 particle의 index를 저장하는 이중 배열
* 각 particle index마다 이전에 어떤 grid cell index에 속했었는지 저장하는 추가적인 배열을 만들어서 data를 제거하는 과정을 최소화 함

particle index마다 neighbor particle의 index를 저장하는 이중 배열
* 이 배열을 더이상 이용하지 않고 27개의 grid cell에 속한 모든 particle index를 넘긴다.
* 문제점
  * particle을 업데이트 할 때, neighbor인지 거리 검사를 해야 된다.
  * density 계산할 때 한번, force 계산할 때 검사해서 중복 검사가 발생한다.
* 장점
  * 






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
