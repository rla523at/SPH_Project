struct PS_Input
{
  float4 pos : SV_POSITION;
  float2 tex : TEXCOORD;
};

float4 main(PS_Input input) : SV_Target
{  
    const float d = length(input.tex - float2(0.5,0.5));
    const float z = sqrt(0.25 - d*d);

    //if (0.5 <= d)
      //  return float4(1.0,1.0,1.0,1.0);
    //else
      //  return float4(z,z,z,1);

    clip(0.5-d);

    return float4(z,z,z,1);
}