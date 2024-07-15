struct VS_Input
{
  float3 pos : POSITION;
};

struct VS_Output
{
  float4 pos : SV_Position;
};

VS_Output main(VS_Input input)
{
  VS_Output output;  
  output.pos = float4(input.pos, 1.0);  
  
  return output;
}
