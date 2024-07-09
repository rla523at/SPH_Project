Buffer<float3> positions : register(t0);

struct VS_Output
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VS_Output main(uint vertexID : SV_VertexID)
{
    float3 pos = positions.Load(vertexID);
    
    VS_Output output;    
    output.color = float3(0.7,0.7,0.7);
    output.position = float4(pos, 1.0); //NDC

    return output;
}