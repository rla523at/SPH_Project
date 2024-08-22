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

간략하게 쓰고, Document를 참고할 수 있게 유도

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

### 수치적분

논문을 통해, explicit scheme 중 semi implicit euler과 leap frog 방법이 가장 많이 사용되어 두 scheme의 stability를 비교하였으나 차이는 없었다.

그래서, 계산과정이 더 단순하여 성능상에 이점이 있는 semi implcit euler scheme을 프로젝트에 사용하였다.

### SPH Scheme
SPH를 이용한 수치해석 과정에서 가장 핵심이 되는 부분은 비압축성 가정을 만족하는 압력을 결정하는 방법이다.

압력 결정 방법에 따라 다양한 SPH기법이 존재하며 이번 프로젝트에서는 linear solver가 필요없는 기법인 Weakly Compsressible SPH(WCSPH)와 Prediction Correction Incompressible SPH(PCISPH)를 사용하였다.

압력을 간단한 수식으로 계산할 수 있어, solver가 단순하고 압력 계산을 매우 빠르게 계산할 수 있다는 장점이 있는 WCSPH를 구현하였지만  $\Delta t$가 매우 작은 값만 가능한 WCSPH의 한계로 인해, PCISPH을 구현 및 적용하였으며 결론적으로 동일 simulation 기준 $\Delta t$를 `10배 증가`시킬 수 있었다.