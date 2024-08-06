#define NUM_THREAD 256

cbuffer CB : register (b0)
{
  uint  g_num_fp;
  float g_dt;
};

StructuredBuffer<float3> v_cur_pos_buffer     : register(t0);
StructuredBuffer<float3> v_cur_vel_buffer     : register(t1);
StructuredBuffer<float3> v_accel_buffer       : register(t2);
StructuredBuffer<float3> v_a_pressure_buffer  : register(t3);

RWStructuredBuffer<float3> v_pos_buffer : register(u0);
RWStructuredBuffer<float3> v_vel_buffer : register(u1);

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;

  const uint fp_index = DTid.x;

  const float3 v_x_cur  = v_cur_pos_buffer[fp_index];
  const float3 v_v_cur  = v_cur_vel_buffer[fp_index];
  const float3 v_a      = v_accel_buffer[fp_index] + v_a_pressure_buffer[fp_index];

  const float3 v_v_predict = v_v_cur + g_dt * v_a;
  const float3 v_x_predict = v_x_cur + g_dt * v_v_predict;

  v_vel_buffer[fp_index] = v_v_predict;
  v_pos_buffer[fp_index] = v_x_predict;
}