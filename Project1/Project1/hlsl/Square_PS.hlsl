Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct PS_Input
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

float4 main(PS_Input input) : SV_TARGET
{
   return texture0.Sample(sampler0, input.tex);    
   //return float4(1.0, 1.0, 1.0, 1.0);
}