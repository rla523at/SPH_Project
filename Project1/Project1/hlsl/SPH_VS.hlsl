StructuredBuffer<float3> position_buffer  : register(t0);
StructuredBuffer<float>  density_buffer   : register(t1);

struct VS_Output
{
    float4 position : SV_POSITION;
    float density : DENSITY;
};

VS_Output main(uint Vid : SV_VertexID)
{
    const float3  pos     = position_buffer[Vid];
    const float   density = density_buffer[Vid];
    
    VS_Output output;    
    output.position = float4(pos, 1.0); 
    output.density = density;

    return output;
}

