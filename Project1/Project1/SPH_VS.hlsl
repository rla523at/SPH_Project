Buffer<float3> positions : register(t0);
Buffer<float> densities : register(t1);

struct VS_Output
{
    float4 position : SV_POSITION;
    float density : DENSITY;
};

VS_Output main(uint vertexID : SV_VertexID)
{
    const float3 pos = positions.Load(vertexID);
    
    VS_Output output;    
    output.position = float4(pos, 1.0); 
    output.density = densities.Load(vertexID);

    return output;
}

