Texture2D<float4> u_texture : register(t1);
SamplerState u_sampler : register(s0);

struct Input
{
    float3 color : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
};

struct Output
{
    float4 out_color : SV_Target0;
};

Output main(Input stage_input)
{
    Output stage_output;
    stage_output.out_color = u_texture.Sample(u_sampler, stage_input.tex_coord);
    return stage_output;
}
