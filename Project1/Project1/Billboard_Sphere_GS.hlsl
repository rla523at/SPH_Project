struct GS_Input
{
  float4 pos : SV_Position;
};

struct GS_Output
{
  float4 pos : SV_POSITION;
  float2 tex : TEXCOORD;
};

cbuffer GS_Cbuffer : register(b0)
{
  float   radius;
  float3  v_cam_pos;
  float3  v_cam_up;
  matrix  m_view;
  matrix  m_proj;
};

[maxvertexcount(4)]
void main(point GS_Input input[1], inout TriangleStream<GS_Output> outputStream)
{
    const float3 v_point_pos = input[0].pos.xyz;
    const float3 v_dir = normalize(v_point_pos - v_cam_pos);
    const float3 v_right = cross(v_cam_up, v_dir);

    const float2 offsets[4] = {
        float2(-1.0, 1.0),
        float2(1.0, 1.0),
        float2(-1.0, -1.0),
        float2(1.0, -1.0)
    };

    const float2 texs[4] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.0, 1.0),
        float2(1.0, 1.0)
    };

    GS_Output output;

    for (int i=0; i<4; ++i)
    {
        const float3 v_vertex_pos = v_point_pos + radius * offsets[i].x * v_right + radius * offsets[i].y * v_cam_up;
        
        float4 v_arg_vpos = float4(v_vertex_pos, 1.0);
        v_arg_vpos = mul(m_view, v_arg_vpos);
        v_arg_vpos = mul(m_proj, v_arg_vpos);

        output.pos = v_arg_vpos;
        output.tex = texs[i];
        outputStream.Append(output);
    }
}