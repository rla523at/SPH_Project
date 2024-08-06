#define NUM_THREAD 256

cbuffer CB : register(b0)
{
  float g_cor;
  float g_wall_x_start;
  float g_wall_x_end;
  float g_wall_y_start;
  float g_wall_y_end;
  float g_wall_z_start;
  float g_wall_z_end; 
  uint  g_num_fp;
}

RWStructuredBuffer<float3> v_pos_buffer : register(u0);
RWStructuredBuffer<float3> v_vel_buffer : register(u1);

[numthreads(NUM_THREAD,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
  if (g_num_fp <= DTid.x)
    return;
  
  const uint  fp_index = DTid.x;
  float3      v_x      = v_pos_buffer[fp_index];
  float3      v_v      = v_vel_buffer[fp_index];
  
  if (v_x.x < g_wall_x_start && v_v.x < 0.0)
  {
    v_x.x = g_wall_x_start;
    v_v.x *= -g_cor;
  }
  
  if (v_x.x > g_wall_x_end && v_v.x > 0.0)
  {
    v_x.x = g_wall_x_end;
    v_v.x *= -g_cor;
  }
  
  if (v_x.y < g_wall_y_start && v_v.y < 0.0)
  {
    v_x.y = g_wall_y_start;
    v_v.y *= -g_cor;
  }
  
  if (v_x.y > g_wall_y_end && v_v.y > 0.0)
  {
    v_x.y = g_wall_y_end;
    v_v.y *= -g_cor;
  }
  
  if (v_x.z < g_wall_z_start && v_v.z < 0.0)
  {
    v_x.z = g_wall_z_start;
    v_v.z *= -g_cor;
  }
  
  if (v_x.z > g_wall_z_end && v_v.z > 0.0)
  {
    v_x.z = g_wall_z_end;
    v_v.z *= -g_cor;
  }  
  
  v_pos_buffer[fp_index] = v_x;
  v_vel_buffer[fp_index] = v_v;
}