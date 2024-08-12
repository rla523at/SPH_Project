#define NUM_THREAD 256

cbuffer CB : register(b0)
{
  float g_rho0;
  float g_m0;
  uint  g_num_fp;
  uint  g_estimated_num_nfp;
};

StructuredBuffer<uint>   ncount_buffer            : register(t0);
StructuredBuffer<float>  W_buffer                 : register(t1);
StructuredBuffer<float>  scailing_factor_buffer   : register(t2);

RWStructuredBuffer<float> density_buffer        : register(u0);
RWStructuredBuffer<float> pressure_buffer       : register(u1);
RWStructuredBuffer<float> density_error_buffer  : register(u2);


[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index     = DTid.x;
  const uint start_index  = fp_index * g_estimated_num_nfp;  

  float rho = 0.0;
  
  const uint num_nfp = ncount_buffer[fp_index];
  for (uint i=0; i<num_nfp; ++i)
  {
    const uint  index = start_index + i;
    const float W     = W_buffer[index];

    rho += W;
  }

  rho *= g_m0;
  
  const float denisty_error = rho - g_rho0;

  density_buffer[fp_index]        = rho;
  density_error_buffer[fp_index]  = denisty_error;  
  pressure_buffer[fp_index]       +=  scailing_factor_buffer[0] * denisty_error;  
  pressure_buffer[fp_index]       = max(0.0, pressure_buffer[fp_index]);  
}