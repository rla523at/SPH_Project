# 2024.07.04
* 참고 자료 학습
  * 2014 (Markus et al) SPH Fluids in Computer Graphics.pdf
    * viscosity coefficient
  * 2007 (Markus and Matthias) Weakly compressible SPH for free surface flows
    * pressure coefficient
    * time step

* SPH 코드 개발
  * 2014 논문 기반으로 update 함수 개발 완료
  * Density가 너무 커지는 문제(미해결)
    * 임시방편으로 upper bound 걸어놓음
  * Negative pressure 문제(미해결)    
    * 임시방편으로 lower bound 걸어놓음 
    * density 문제가 해결되면 자연스럽게 해결될 가능성 높음
  * Pressure driven force가 너무 큰 문제(미해결)
    * 고민중...  

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
