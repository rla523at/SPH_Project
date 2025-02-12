struct PS_Input {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

float4 main(PS_Input input) : SV_TARGET {
    const float d = length(input.tex - float2(0.5, 0.5));
    const float z = sqrt(0.25 - d * d);

    clip(0.5 - d);

    const float3 color = (2 * z) * float3(1.0, 1.0, 1.0);    
    return float4(color, 0.1);
}