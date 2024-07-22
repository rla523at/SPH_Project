struct VS_Output
{
    float4 position : SV_POSITION;
};

VS_Output main(float3 pos : POSITION)
{
    VS_Output output;
    output.position = float4(pos, 1.0);

    return output;
}