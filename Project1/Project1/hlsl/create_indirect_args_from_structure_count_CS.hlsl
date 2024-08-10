
cbuffer CB : register (b0)
{
  uint g_num_thread;
  uint g_num_count;
}

RWBuffer<uint> arg_buffer : register(u0);

[numthreads(1,1,1)]
void main()
{
  // ceil( g_num_count / g_num_thread )
  const uint num_group_x = (g_num_count + g_num_thread - 1) / g_num_thread;

  arg_buffer[0] = num_group_x;
  arg_buffer[1] = 1;
  arg_buffer[2] = 1;
}