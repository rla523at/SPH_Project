struct PS_Input {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float density : DENSITY;
};

float4 main(PS_Input input) : SV_TARGET {
    const float d = length(input.tex - float2(0.5, 0.5));
    const float z = sqrt(0.25 - d * d);
    const float rho = input.density;

    clip(0.5 - d);

    const float density_max = 1010;
    const float density_min = 990;
    const float rho0 = 1000;

    const float3 red = float3(1.0, 0.0, 0.0);
    const float3 blue = float3(0.0, 0.0, 1.0);
    const float3 white = float3(1.0, 1.0, 1.0);

    float3 density_color = float3(0.0, 0.0, 0.0);
    if (rho < rho0)
    {
        const float intensity = (rho - density_min) / (rho0 - density_min);
        density_color = intensity * white + (1-intensity) * blue;
    }
    else
    {
        const float intensity = (rho - rho0) / (density_max - rho0);
        density_color = intensity * red + (1-intensity) * white;
    }
    
    const float lambert = 2*z;

    const float3 color = lambert * density_color;
    return float4(color, 1);
}