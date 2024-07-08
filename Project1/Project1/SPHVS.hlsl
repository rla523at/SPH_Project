struct VS_Output
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

struct Particle
{
    float3 position;
    float3 velocity;
    float3 force;
    float density;
    float pressure;
    //bool is_alive;
};

StructuredBuffer<Particle> particles : register(t0);

VS_Output main(uint vertexID : SV_VertexID)
{
    Particle p = particles[vertexID];
    
    VS_Output output;    
    //output.color = float3(0.7,0.7,0.7) * float(p.is_alive);
    output.color = float3(0.7,0.7,0.7);
    output.position = float4(p.position.xyz, 1.0); //NDC

    return output;
}
