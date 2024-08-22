# Simulation Parameter

## Smoothing Length
$\Delta x$를 initial particle spacing이라고 할 때, 이 프로젝트에서는 Smoothing length $h=1.2\Delta x$를 사용하였다.

### 참고
**1996 (Koshizuka & Oka) Moving-Particle Semi-Implicit Method for Fragmentation of Incompressible Fluid**
* IV.B. Calculation Parameters
  * $h = 1.4 - 1.7 \Delta x$

**1999 (Sharen and Rudman) An SPH Projection Method**
* 2.1. Weakly Compressible SPH
  * $h = 1 - 1.5 \Delta x$
* 3. Simulations
  * $h=1.3 \Delta x$

**2000 (Joseph) Simulating surface tension with SPH**
* $h = 1 - 1.5 \Delta x$

**2007 (Markus and Matthias) Weakly compressible SPH for free surface flows**
* $h = \Delta x$

**2012 (Mostafa et al) A robust weakly compressible SPH method and its comparison with an incompressible SPH**
* $h = 1.6 \Delta x$

**2014 (Goffin et al) Validation of a SPH Model For Free Surface Flows**
* 2.2 Some Implementation Aspects
  * $h = 1.2\Delta x$

**CPP-Fluid-Particles**
* const float sphSmoothingRadius = 2.0f * sphSpacing;
* pSystem = std::make_shared<SPHSystem>(fluidParticles, boundaryParticles, pSolver,
* SPHSystem::SPHSystem
  * _sphSmoothingRadius = sphSmoothingRadius
* SPHSystem::step
* BasicSPHSolver::step
  * radius = sphSmoothingRadius
* static inline __device__ float cubic_spline_kernel(const float r, const float radius)
  * h = 0.5radius

### Smoothing Length에 따른 Stability
Smoothing Length가 줄어들면 solution이 발산한다.

### Smoothing Length에 따른 initial density distribution
$h = \Delta x$일 때, 999, 850, 719, 606의 4가지 density를 갖음.

$h = 1.5\Delta x$일 때, 1001, 977, 953, 929, 735, 716, 697, 537, 523, 392의 가지 density를 갖음.

### Smoothing Length에 따른 Solution 변화

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

<br><br>

## Free Surface parameter
이 프로젝트에서는 0.95를 사용하였다.

**1996 (Koshizuka & Oka) Moving-Particle Semi-Implicit Method for Fragmentation of Incompressible Fluid**
* IV.B. Calculation Parameters
  * $\beta = 0.8 - 0.99$

**2008 (ataie-ashtiani et al) Modified incompressible SPH meothod for simulating free surface problems**
* 4.2 Free surface
* 6.1.1. Density error analysis for the dam-break problem
  * $\beta = 0.8 - 0.99$

**2016 (Shobeyri) A Simplified SPH Method for Simulation of Free Surface Flows**
* 3.2.1 Free Surfaces
  * $\beta = 0.8 - 0.95$
