static const float2 positions[3] = { (-0.5f).xx, float2(0.5f, -0.5f), float2(0.0f, 0.5f) };
static const float3 colors[3] = { float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f) };

struct Input
{
    uint vertex_index : SV_VertexID;
};

struct Output
{
    float3 frag_color : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input stage_input)
{
    int vertex_index = int(stage_input.vertex_index);
    Output stage_output;
    stage_output.position = float4(positions[vertex_index], 0.0f, 1.0f);
    stage_output.frag_color = colors[vertex_index];
    return stage_output;
}
