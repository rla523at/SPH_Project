struct PS_Input {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float density : DENSITY;
};

float4 main(PS_Input input) : SV_TARGET {
    const float d = length(input.tex - float2(0.5, 0.5));
    const float z = sqrt(0.25 - d * d);
    const float density = input.density;

    clip(0.5 - d);

    const float density_max = 1010;
    const float density_min = 990;
    const float density_intensity = (density - density_min) / (density_max - density_min);

    const float3 red = float3(1.0, 0.0, 0.0);
    const float3 blue = float3(0.0, 0.0, 1.0);
    
    const float lambert = 2*z;
    const float3 density_color = density_intensity * red + (1-density_intensity) * blue;

    const float3 color = lambert * density_color;
    return float4(color, 1);
}