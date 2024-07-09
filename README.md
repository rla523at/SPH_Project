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
