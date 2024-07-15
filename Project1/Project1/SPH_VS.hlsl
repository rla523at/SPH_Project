Buffer<float3> positions : register(t0);

struct VS_Output
{
    float4 position : SV_POSITION;
};

VS_Output main(uint vertexID : SV_VertexID)
{
    const float3 pos = positions.Load(vertexID);
    
    VS_Output output;    
    output.position = float4(pos, 1.0); 

    return output;
}

// struct VS_Output
// {
//     float4 position : SV_POSITION;
// };

// VS_Output main(float3 pos)
// {
//     VS_Output output;    
//     output.position = float4(pos, 1.0); 

//     return output;
// }