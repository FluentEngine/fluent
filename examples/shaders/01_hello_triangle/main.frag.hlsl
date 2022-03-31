struct Input
{
    float3 color : TEXCOORD0;
};

struct Output
{
    float4 color : SV_Target0;
};

Output main(Input stage_input)
{
    Output stage_output;
    stage_output.color = float4(stage_input.color, 1.0f);
    return stage_output;
}
