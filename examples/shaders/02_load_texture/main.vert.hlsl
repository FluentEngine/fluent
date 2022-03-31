static const float3 colors[3] = { float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f) };

struct Input
{
    float2 position : POSITION;
    float2 tex_coord : NORMAL;
    uint vertex_index : SV_VertexID;
};

struct Output
{
    float3 frag_color : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
    float4 position : SV_Position;
};

Output main(Input stage_input)
{
    Output stage_output;
    stage_output.position = float4(stage_input.position, 0.0f, 1.0f);
    stage_output.frag_color = colors[int(stage_input.vertex_index)];
    stage_output.tex_coord = stage_input.tex_coord;
    return stage_output;
}
