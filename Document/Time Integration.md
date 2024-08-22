# Time Integration

**[semi implicit euler]**

$$ \begin{aligned}
\frac{v^{n+1} - v^{n}}{\Delta t} &= f(v^n,x^n) \\
\frac{x^{n+1} - x^{n}}{\Delta t} &= v^{n+1}  
\end{aligned} $$

**[leap frog - KDK]**

$$ \begin{aligned}
\frac{v^{n+1/2} - v^{n}}{0.5 \Delta t} &= f(v^n,x^n)\\
\frac{x^{n+1} - x^{n}}{\Delta t} &= v^{n+1/2}  \\
\frac{v^{n+1} - v^{n+1/2}}{0.5 \Delta t} &= f(v^{n+1/2},x^{n+1})\\
\end{aligned} $$

**[leap frog - DKD]**

$$ \begin{aligned}
\frac{x^{n+1/2} - x^{n}}{0.5 \Delta t} &= v^{n}  \\
\frac{v^{n+1} - v^{n}}{\Delta t} &= f(v^n,x^{n+1/2})\\
\frac{x^{n+1} - x^{n+1/2}}{0.5 \Delta t} &= v^{n+1}  \\
\end{aligned} $$