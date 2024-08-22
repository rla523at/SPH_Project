# SPH

## SPH Frame Work
SPH approximation에 의해 임의의 물리량 $f$에 대한 근사식은 다음과 같다.

$$ f(x) = \sum_j \frac{m_j}{\rho_j}f(x_j) W(x-x_j,h) $$

이 approximation을 통해 비압축성 유체의 linear momentum equation을 푼다.

$$ \frac{du}{dt} = -\frac{1}{\rho}\nabla p + \nu \nabla^2u + f_{ext} $$

전체적인 solution prcess는 다음과 같다.

1. SPH approximation을 통해 $\rho$를 계산한다.
2. 계산된 $\rho$를 통해 $p$를 계산한다.
3. SPH approximation를 활용해 linear momentum equation의 RHS를 계산한다.
4. 계산된 가속도로 속도를 업데이트하고, 속도로 위치를 업데이트한다.

</br></br>

## SPH Approximation
### Integral Representation
임의의 $f:\mathbb{R}^3\rightarrow \mathbb{R}$가 있을 떄, 임의의 $x \in \Omega \subseteq \mathbb{R}^3$에 대해서 다음이 성립한다.

$$ f(x) = \int_{\Omega} f(t)\delta(x-t)dt $$

$\delta$는 dirac-delta function kernel이다.

### Approximation of Integral Representation
$\delta$를 임의의 smooth kernel function $W(x,h):\mathbb{R}^3 \rightarrow \mathbb{R}$로 근사하면 다음과 같다.

$$ f(x) \approx \int_{\Omega} f(t)W(x-t, h)dt $$

이처럼, $\delta$를 임의의 smooth function으로 approximation을 하기 때문에 'smoothed' particle hydronamics라고 한다.

### Particle Approximation
$W$를 이용한 Integral Representation의 approximation은 discretized form으로 한번 더 근사가 될 수 있다.

$$ \begin{aligned} 
f(x) &\approx \int_{\Omega} f(t)W(x-t, h)dt \\ 
     &\approx \sum_{j} f(x_j)W(x-x_j,h) \Delta V_j \\ 
     & = \sum_{j} f(x_j)W(x-x_j,h) \frac{\rho_j \Delta V_j}{\rho_j} \\ 
     &= \sum_{j} f(x_j)W(x-x_j,h) \frac{m_j}{\rho_j} 
\end{aligned}  $$

따라서, SPH frame work상 임의의 물리량 $f$에 대한 최종 근사식은 다음과 같다.

$$ f(x) = \sum_j \frac{m_j}{\rho_j}f(x_j) W(x-x_j,h) $$

</br></br>

## Smooth Kernel Function
$W(x,h)$는 $\delta$를 근사한 함수이기 때문에 다음 세가지 조건을 만족해야한다.

Normalization condition

$$ \int_\Omega W(x-t,h)dt = 1$$

Delta Function Property

$$ \lim_{h\rightarrow 0} W(x-t,h) = \delta(x-t)$$

Compact Supprot Property

$$ \kappa h < |x-t| \implies W(x-t,h) = 0 $$

따라서, $h$에 의해 $W$의 compact support domain이 결정되며, $h$를 support length라고 한다.

![support length](https://github.com/user-attachments/assets/788aa774-2b4f-4b4c-a260-6a9b29e40c33)

</br></br>

## Density Calculation
SPH 근사식에 밀도를 대입하면 다음과 같다.

$$ \rho(x) = \sum_j m_jW(x,x_j,h) $$

비압축성 유체이기 때문에 $m_j$가 일정하다고 가정하면 밀도는 오직 거리의 함수이다. 

밀도는 pressure calculation과 비압축성 가정 $(\frac{\Delta\rho}{\rho_0})^2 \sim (M \le 0.1)$에 의해 다음 범위의 값을 갖는것이 타당하다.

$$ \rho_0 \le \rho \le 1.01 \rho_0 $$

밀도 값에 영향을 주는 값들은 $m$ 그리고 $W$ 값이다.

이 때, $W$의 값은 $h$를 어떻게 결정하냐에 따라 크게 달라진다.

</br></br>

## 물 시뮬레이션

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

### Acceleration by Pressure
압력에 의한 가속도는 다음과 같다.

$$ a_p(x_i) = -\frac{1}{\rho_i}\nabla p_i $$

근사한 물리량의 미분을 구할 때 SPH discretization formular를 바로 미분해서 사용하면 simulation이 unstable 해지는 현상이 있기 때문에 discrete differential operator를 사용한다.

$$ \nabla A_i \approx \rho_i \sum_j m_j (\frac{A_i}{\rho_i^2} + \frac{A_j}{\rho_j^2}) \nabla W_{ij} $$

위의 discrete differential operator를 사용하고 $m$이 모든 particle에서 $m_0$로 일정하다고 가정하면 다음과 같이 표현할 수 있다.

$$ a_p(x_i) = - m_0 \sum_j (\frac{p_i}{\rho_i^2} + \frac{p_j}{\rho_j^2}) \nabla W_{ij} $$

$a_p$의 방향은 $\sum_j -\nabla_iW_{ij}$이다.

> Reference  
> 2022 (Dan et al) A Survey on SPH Methods in Computer Graphics - 2.5 - symmetric formula  

### Acceleration by Viscosity
점성에 의한 가속도는 다음과 같다.

$$ a_v(x_i) = \nu \nabla^2u $$

근사한 물리량의 미분을 구할 때 SPH discretization formular를 바로 미분해서 사용하면 simulation이 unstable 해지는 현상이 있기 때문에 discrete differential operator를 사용한다.

$$ \nabla^2A_i = 2(d+2)\sum_j \frac{m_j}{\rho_j} \frac{A_{ij} \cdot x_{ij}}{x_{ij} \cdot x_{ij}+0.01h^2} \nabla W_{ij} $$

위의 discrete differential operator를 사용하고 $m$이 모든 particle에서 $m_0$로 일정하다고 가정하면 다음과 같이 표현할 수 있다.

$$ a_v(x_i) = 2 \nu (d+2) m_0 \sum_j \frac{1}{\rho_j} \frac{v_{ij} \cdot x_{ij}}{x_{ij} \cdot x_{ij}+0.01h^2} \nabla W_{ij}  $$

> Reference  
> 2022 (Dan et al) A Survey on SPH Methods in Computer Graphics - 2.5.1  Discretization of the Laplace Operator

</br></br>

## SPH Scheme
SPH를 이용한 수치해석 과정에서 가장 핵심이 되는 부분은 비압축성 가정을 만족하는 압력을 결정하는 방법이다.

압력 결정 방법에 따라 다양한 SPH기법이 존재하며 이번 프로젝트에서는 linear solver가 필요없는 기법인 Weakly Compsressible SPH(WCSPH)와 Prediction Correction Incompressible SPH(PCISPH)를 사용하였다.

### Weakly Compressible SPH
WCSPH에서는 압력을 계산하기 위해 Tait's equation이라는 가상의 equation of state를 사용한다.

$$ p = k ((\frac{\rho}{\rho_0})^7 - 1) $$

압력을 간단한 계산으로 매우 빠르게 구할 수 있다는 장점이 있지만, 비압축성을 부과하기 위해 작은 밀도변화에도 압력 변화가 크게 발생하게 디자인되어 있다$(k\sim O(10^6))$.

그 결과 작은 밀도 변화에도 강한 압력이 발생해 서로 밀어내는 힘이 작용함으로 밀도 변화가 작게 유지되게 하는 역할을 하지만 동시에 $\Delta t$가 조금만 커져도 강한 압력에 의해 position과 velocity에 매우 큰 변화가 생긴다.

따라서, $\Delta t$가 매우 제한되는 문제가 생긴다.

## 참고
**2007 (Markus and Matthias) Weakly compressible SPH for free surface flows**
* $B = \rho_0 V_s^2 / \gamma$ ~ 1.1e06 

**2012 (Mostafa et al) A robust weakly compressible SPH method and its comparison with an incompressible SPH**
* Artificial equation of state WCSPH

#### Density 계산 문제
현재 밀도는 압력이 음수가 되지 않게, 그리고 비압축성 가정을 만족하게 제한 되어 있음.

$$ \rho_0 \le \rho \le 1.01 \rho_0 $$

```cpp
cur_rho = std::clamp(cur_rho, rho0, 1.01f * rho0);
```

기존 논문들에는 없는 제약조건이 왜 필요한지 파악 후 개선하기 위해 clamp를 제거후 문제상황을 분석.

![2024-07-23 10 10 49 mass mongham + boundary](https://github.com/user-attachments/assets/2cc5c05a-5131-44a7-9e5a-25ad5021bcef)

물리적으로, 물은 $\rho_0 = 1000$이라는 rest density를 가져야 한다.

하지만 partic들의 $\rho$를 계산하면 450 ~ 460의 값을 갖게 되어 particle들이 순간적으로 매우 강하게 압축되고, 이로 인해 거리가 가까워지면 $\rho$값이 순간적으로 폭발적으로 증가하여 다시 아주 강하게 팽창시키는 힘이 만들어진다.

SPH descritization에서 $x$ 위치에서 밀도는 다음과 같다.

$$ \rho(x) =  \sum_j m_j W(x,x_j,h) $$

이 때, 일반적으로 $m_j$는 particle마다 전부 동일하다는 가정으로 상수값으로 두어 식을 다음과 같이 단순화한다.

$$ \rho(x) = m \sum_j  W(x,x_j,h) $$

즉, $\rho$는 결국 $m$과 $W$에 의해 결정된다.

**[Density 계산 해결]**

다양한 논문을 통해 비교 검증한 결과 kernel 함수 $W$에서는 문제점을 발견하지 못했고, $m$ 계산 방식을 수정

1994 (monaghan) Simulating Free Surface Flows with SPH

$$ m = \rho_0 \frac{V}{N_{particle}} $$

2012 (Mostafa et al) A robust weakly compressible SPH method and its comparison with an incompressible SPH

$$ m = \frac{\rho_0}{\max(\{ \sum_j W(x,x_j,h) \})} $$ 

참고로, $W$ 함수의 성질에 의해 $\sum_j W(x,x_j,h)$ 값은 $x$ 위치에서의 particle의 number density(밀집정도)를 나타내는 값이 된다.

즉, 2012 (Mostafa et al)의 방식은 가장 밀집된 곳에 있는 particle의 밀도가 $\rho_0$가 되게끔 결정하는 방식이다.

**[Boundary Interaction]**

추가적으로, $z=0$ 위치에 있는 particle들이 움직이는 과정에서 $z=0$ boundary와 상호작용이 발생한다.

상호작용이 발생하게 되면 순간적으로 위치와 속도가 조정이 되고 이 영향으로 폭발적으로 팽창하게 되는 문제가 있어, 문제의 초기 조건을 수정하였다.

**[개선결과]**

![2024-07-19 10 33 06](https://github.com/user-attachments/assets/5c27710d-effe-44aa-8c26-651900078a2f)


**[추가 개선]**

질량 계산 방식을 개선한 후에도 partic들의 $\rho$를 계산하면 973, 982, 991, 1000의 4가지 값을 갖게 된다. 

이 떄, 1000보다 작은 밀도를 갖는 particle들이 압축시키는 힘을 만들어내고, 이로 인해 거리가 가까워지면 $\rho$값이 증가하여 다시 팽창시키는 힘이 만들어진다.

이 과정을 통해 압축 팽창이 반복되게 된다.

따라서, 이를 개선하기 위해, 밀도 불균형을 가능한 해결하려고 $m$ 계산 방식을 다음과 같이 개선하였다.

$$ m = \frac{\rho_0}{\text{avg}(\{ \sum_j W(x,x_j,h) \})} $$ 

이 수식의 경우, 평균적인 밀집정도를 갖는 곳에 있는 particle의 밀도가 $\rho_0$가 되게끔 결정하게 되어 density fluctuation $\frac{|\rho-\rho_0|}{\rho_0}$을 줄일 수 있다.

그 결과 압축 팽창 정도를 많이 줄일 수 있었다.

![2024-07-19 11 27 20 avg](https://github.com/user-attachments/assets/23b824c3-40ff-4d7b-9c4b-ca2d41a771cf)


**[최종 개선 결과]**

$\nu = 1.0e-6$과 density clamp 없이 정상동작

![2024-07-23 10 29 33 mass 개선](https://github.com/user-attachments/assets/8b7f3e93-5ef7-4c63-93ac-3c649db72902)

#### Smoothing Length 개선

particle의 initial distance를 $\Delta x$라고 할 때, 기존의 논문들은 smoothing length($h$)를 $h = 1.0 \Delta x \sim 2.0\Delta x$를 사용함을 확인하였다.

하지만 기존에는 $h=0.6 \Delta x$를 사용 중이여서 이를 기존 논문들의 값과 비슷한 값을 사용하게 개선하였다.

* **[문제]**

$h$를 증가시킬 수록 particle별 density 변화량이 심해지는 현상이 발생하였다.

밀도 계산은 particle이 얼마나 밀집되어있는지를 나타내는 number density($n_i$)에 비례한다.

$$ \rho(x_i) = m \sum_j  W(x_i,x_j,h) = mn_i $$

$h$가 증가 될 수록 고려하는 영역이 넓어지게 되어 boundary에 가까운 particle과 아닌 particle간의 neighbor의 수 차이가 커지게 되고 이는 $n_i$의 차이로 이어지고 density의 차이도 커지게 된다.

$$\begin{array}{c|c|c}
                & N_{neighbor,max}  & N_{neighbor,min}  \\ 
\hline
h=0.6 \Delta x  & 7                 &   3               \\
h=1.1 \Delta x  & 33                &   11              \\
\end{array} $$

$$ \frac{\rho_{min}}{\rho_{max}} = 
\begin{cases}
0.97 & h = 0.6 \Delta x \\
0.54 & h = 1.1 \Delta x
\end{cases} $$

이로써, 기존의 질량을 잘 선택해서 밀도 변화량을 줄이는 방법으로는 더이상 해결이 불가능하다.

$$ m = \frac{\rho_0}{\text{avg}(\{ \sum_j W(x,x_j,h) \})} $$

참고로, $\frac{\rho_{min}}{\rho_{max}}$차이가 클 때, 위와 같은 방법을 사용하면, 가장 밀집된 곳에 있는 particle의 밀도가 비정상적으로 커지게 되고 이는 업청난 압력을 야기하게 된다.

그 결과 시작하자마자 모든 particle들이 폭탄처럼 터지게 되는 현상이 발생함으로, $n_i$가 가장 큰 곳에서 rest density를 갖도록 질량은 기존 논문에서 제시했던 방법을 그대로 사용하였다.

$$ m = \frac{\rho_0}{\text{max}(\{ \sum_j W(x,x_j,h) \})} $$

* **[해결 - Free Surface Boundary Condition]**

물리적으로 자유 수면에서는 대기압을 갖게 된다.

따라서, 자유 수면에 위치한 particle의 경우 $p=p_{air} = 0$라는 Dirichlet boundary condition을 줘야한다.

이 때, 공기를 따로 modeling 하지 않은 경우 자유 수면에 위치한 particle의 경우 $n_i$가 급격하게 감소하게 됨으로 다음 조건으로 Free Surface를 판단할 수 있다.

$$ n_i < \beta n_{max} $$

여기서 $\beta$는 free surface parameter로 일반적으로 $0.8 \sim 1$의 값을 사용하며, 값에 따른 결과의 차이는 크지 않다.

$n_i \propto \rho_i$임으로, 다음과 같이 판단할 수 있다.

$$ \rho_i < \beta \rho_0 $$

기존에는 Tait's equation을 활용해 pressure을 계산하고 있었음으로, $p=0$인 조건은 $\rho = \rho_0$인 조건과 동일하다.

$$ p = k ((\frac{\rho}{\rho_0})^7 - 1) $$

따라서, density와 pressure 계산 코드는 다음과 같다.

```cpp
  for (int i = 0; i < _num_fluid_particle; i++)
  {
    auto& rho = densities[i];
    auto& p   = pressures[i];

    //update denisty
    rho = 0.0;

    const auto& neighbor_informations = _uptr_neighborhood->search_for_fluid(i);
    const auto& neighbor_distances    = neighbor_informations.distances;
    const auto  num_neighbor          = neighbor_distances.size();

    for (int j = 0; j < num_neighbor; j++)
    {
      const float distance = neighbor_distances[j];
      rho += _uptr_kernel->W(distance);
    }

    rho *= m0;

    if (rho < _free_surface_parameter * rho0)
      rho = rho0;

    //update pressure
    p = k * (pow(rho / rho0, gamma) - 1.0f);
  }
```

> Reference  
> 1996 (Koshizuka & Oka) Moving-Particle Semi-Implicit Method for Fragmentation of Incompressible Fluid  
> 2008 (ataie-ashtiani et al) Modified incompressible SPH meothod for simulating free surface problems  
> 2016 (Shobeyri) A Simplified SPH Method for Simulation of Free Surface Flows  

* **[결과]**

$h$를 늘리면 simulation 결과에서 물의 출렁거림이 더 잘 표현된다.

$h = 0.6 \Delta x$ (기존)

![2024-07-25 10 24 25 h = 0 6dx](https://github.com/user-attachments/assets/5889c001-046b-4d3e-b2f6-08187a51577e)


$h = 1.1 \Delta x$

![2024-07-25 10 25 13 h = 1 1dx](https://github.com/user-attachments/assets/cc9b8d92-63d6-41ca-8e91-c826281950bb)


$h = 1.6 \Delta x$

![2024-07-25 10 27 06 h = 1 6dx](https://github.com/user-attachments/assets/018aabf2-cf99-48fc-af4b-e70d1c7adce8)

또한, $h$가 늘어나면 $\Delta t$를 증가 시킬 수 있음. 

$$ \Delta t = 5.0e^{-4} \rightarrow 1.0e^{-3} \rightarrow 2.5e^{-3}$$

그러나 꼭, $h$에 비례해서 simulation 결과가 개선되는 것은 아니며 $h$가 증가하면 neighborhood가 증가하여 FPS가 감소하여 성능이 개선되지는 않는다.

$$\text{FPS} = 1200 \rightarrow 500 \rightarrow 200$$


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

#### 수렴문제
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


</br></br>