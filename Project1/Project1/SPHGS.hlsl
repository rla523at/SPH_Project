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
    const float r = 1.0/ 32.0;

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

        outputStream.RestartStrip(); // Strip을 다시 시작
    }

    


    // const float hw = 1.0 / 16.0; // halfWidth
    // //const float hw = 1.0; // halfWidth
    // const float3 up = float3(0, 1, 0);
    // const float3 right = float3(1, 0, 0);

    // GS_Output output;
    // output.pos.w = 1;
    // output.color = input[0].color;
    
    // output.pos.xyz = input[0].pos.xyz - hw * right - hw * up;
    // outputStream.Append(output);

    // output.pos.xyz = input[0].pos.xyz - hw * right + hw * up;
    // outputStream.Append(output);
    
    // output.pos.xyz = input[0].pos.xyz + hw * right - hw * up;
    // outputStream.Append(output);
    
    // output.pos.xyz = input[0].pos.xyz + hw * right + hw * up;
    // outputStream.Append(output);

    // // 주의: GS는 Triangle Strips으로 출력합니다.
    // // https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-strips

    // outputStream.RestartStrip(); // Strip을 다시 시작

    // 주의: GS는 Triangle Strips으로 출력합니다.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-strips

}
