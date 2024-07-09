struct GS_Input
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

struct GS_Output
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

[maxvertexcount(64)]
void main(point GS_Input input[1], uint primID : SV_PrimitiveID, inout TriangleStream<GS_Output> outputStream)
{
    const float pi = 3.14159265;
    const float segments = 12;
    const float dtheta = 2*pi / segments;
    const float r = 1.0/ 20.0;

    const float3 center = input[0].pos.xyz;

    GS_Output output;
    output.color = input[0].color;
    
    for (int i=0; i<segments; ++i)
    {
        output.pos = float4(center, 1.0f);    
        outputStream.Append(output);

        float theta = i*dtheta;
        float3 offset = float3(cos(theta), sin(theta), 0) * r;
        
        output.pos = float4(center + offset, 1.0f);
        outputStream.Append(output);

        theta += dtheta;
        offset = float3(cos(theta), sin(theta), 0) * r;

        output.pos = float4(center + offset, 1.0f);
        outputStream.Append(output);

        outputStream.RestartStrip(); // Strip�� �ٽ� ����
    }
}
