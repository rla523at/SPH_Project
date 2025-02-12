# cubic spline Kernel Function
**1992 (monaghan) Smoothed Particle Hydrodynamics**
* kernel based on spline functions

**2010 (Liu and Liu) SPH an overview and recent developments**

**2014 (Markus et al) SPH Fluids in Computer Graphics**

**https://pysph.readthedocs.io/en/latest/reference/kernels.html**

**https://pysph.readthedocs.io/en/latest/_modules/pysph/base/kernels.html#CubicSpline.gradient_h**
* gradient, dwdq

**SPlisHSPlasH**
* class CubicKernel
  * static void setRadius(Real val)
  * m_radius = support_radius
  * h = m_radius
* Simulation::initKernels
  * supprot radius = 4 * particle radius
  * m_k = 8.0 / pi h^3

# Density Calculation
* 1994 (monaghan) Simulating Free Surface Flows with SPH
  * 2. THE SPH EQUATIONS
  * approximate the rate of change of the density

* 2007 (Crespo et al) Boundary Conditions Generated by Dynamic Particles in SPH Methods
  * 2.3 Continuity equation
    * solve continuity equation
    * summation of mass terms, which leads to an artificial density decrease near fluid interfaces

* 2010 (Mocho et al) State-of-the-art of classical SPH for free-surface flows
  * 3.2 Conservation of mass
    * not the best option in fluid calculations since it leads to a density drop near the free surface of the fluid as stated by Monaghan (1992).

* 2012 (Xiaowei et al) Local Poisson SPH For Viscous Incompressible Fluids
  * [RL96] to improve the accuracy near free surface boundaries and material interfaces.
    * 2010 (Mocho et al) State-of-the-art of classical SPH for free-surface flows - 2.6.1.1 Zeroth order – Shepard filter 에 나온것과 같은 형태이다.

* 2014 (Markus et al) SPH Fluids in Computer Graphics
  * 6.1. Liquid-Liquid Interface
    * In the computer graphics literature, the density summation equation is preferred over evolving the density with the continuity equation

* 2014 (Goffin et al) Validation of a SPH Model For Free Surface Flows
  * 2.1 Equations and Smoothing Functions 
    * solve continuity equation

* SPlisHSPlasH
  * TimeStepWCSPH::step
  * TimeStep::computeDensities

* Book Liu & Liu
  * density.f



# Artificial Viscosity Term
* 2014 (Markus et al) SPH Fluids in Computer Graphics 식이 잘못되어 있음
* 2005 (monaghan) Smoothed Particle Hydrodynamics
* 2022 (Dan et al) A Survey on SPH Methods in Computer Graphics

# Surface Tension
* 2000 (Joseph) Simulating surface tension with SPH

# Position Update
* 2007 (Crespo et al) Boundary Conditions Generated by Dynamic Particles in SPH Methods
  * 2.5 Moving the particles
    * using XSPH variant

* 2010 (Mocho et al) State-of-the-art of classical SPH for free-surface flows
  * 3.6 Moving the particles
    * using XSPH variant

# Tensil Instability
**2000 (monaghan) SPH without a Tensil Instability**

# $\Delta t$
**2009 (solenthaler and Pajarola) Predictive-Corrective Incompressible SPH**
* PCISPH
  * BOundary condition 적용해도 dt에 아무런 영향을 못줌

**2019 (Xi et al) Survey on Smoothed Particle Hydrodynamics and the Particle Systems**
* C. PREDICTIVE-CORRECTIVE INCOMPRESSIBLE SMOOTHED PARTICLE HYDRODYNAMICS (PCISPH)




# Neighborhood Search
## Uniform Grid
**자신이 속한 grid cell을 찾는다.**

grid cell마다 고유의 grid cell index vector $c$를 갖는다.

$$ c = (i,j,k) $$

임의의 particle의 위치를 $x$라고 할 때, $x$가 속한 grid cell의 $c$는 다음과 같다.

$$ c(x) = (\left\lceil \frac{x-x_{min}}{h}  \right\rceil, \left\lceil \frac{y-y_{min}}{h}  \right\rceil, \left\lceil \frac{z-z_{min}}{h}  \right\rceil $$

이 떄, $h$는 정육면체의 길이, $(x_{min},y_{min},z_{min})$는 bounding box의 최소 좌표이다.

정육면체의 한 면을 $h$라고 할 때, $h$가 smoothing length와 같다고 가정하자.

Domain bounding box에 속한 모든 $c$의 집합을 $C$, $x$가 속한 grid cell의 index vector를 $c_x = (i_x,j_x,k_x)$라고 한다면 집합 $N$은 다음과 같다.

$$ N = \{ (i,j,k) | (i_x \pm 1,j_x \pm 1, k_x \pm 1) \in C \} $$

즉, 최대 27개의 grid cell만 탐색하면 된다. 


Domain bounding box가 $x,y,z$ 방향으로 $N_{x,y,z}$개로 나뉘었다고 하자.

그러면 $c$의 고유의 index $c_i$를 다음과 같이 구할 수 있다.

$$ c_i = i + j  N_x + k N_x N_y $$

따라서, grid cell마다 속한 particle을 저장하기 위해서 이중배열이면 충분하다.

## Index Sort
unifrom grid는 동시에 같은 memory에 접근하는 race condition으로 parallel construction이 어렵다.

memory conflict를 피하기 위해 먼저 particle을 cell index로 sorting한다.

## Spatial Hashing


# 영상
* https://www.youtube.com/watch?v=uaoT37-NhCE
* https://www.youtube.com/watch?v=7kDVjZkc_TI
* https://www.youtube.com/watch?v=9YxseonuAJ8 (droplet)
