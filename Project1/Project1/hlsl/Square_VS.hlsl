struct VS_Input
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD;
};

struct VS_Output
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

cbuffer VS_Cbuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

VS_Output main(VS_Input input)
{
    float4 pos_out = float4(input.pos, 1.0f);
    pos_out = mul(model, pos_out);
    pos_out = mul(view, pos_out);
    pos_out = mul(projection, pos_out);

    VS_Output output;
    output.pos = pos_out;
    output.tex = input.tex;
    
    return output;
}
