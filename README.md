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

## 설명

물을 물리시뮬레이션 하기 위해 다음의 비압축성 유체의 linear momentum equation을 수치해석적으로 푼다.

$$ \frac{dv}{dt} = -\frac{1}{\rho}\nabla p + \nu \nabla^2u + f_{ext} $$

SPH를 사용해서 수치해석하는 과정은 `Document/SPH.md`에 설명되어 있다.

WCSPH와 PCISPH가 구현되어 있으며 WCSPH는 CPU코드로 PCISPH는 CPU/GPU(Compute Shader) 코드로 구현되어 있다.

참고로, 시뮬레이션에 사용된 parameter의 경우 `Document/Simulation Parameter.md`에 어떤 값을 사용하였는지 설명되어 있다.

