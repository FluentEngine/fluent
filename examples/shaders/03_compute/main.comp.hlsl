static const uint3 gl_WorkGroupSize = uint3(16u, 16u, 1u);

#ifndef __hlsl_dx_compiler
[[vk::push_constant]]
#endif
cbuffer PushConstants
{
    float iTime;
    float iMouseX;
    float iMouseY;
};

RWTexture2D<float4> uOutputImage : register(u0);

static uint3 gl_GlobalInvocationID;
struct Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

uint2 imageSize(RWTexture2D<float4> Tex, out uint Param)
{
    uint2 ret;
    Tex.GetDimensions(ret.x, ret.y);
    Param = 0u;
    return ret;
}

float2x2 rot(float a)
{
    float c = cos(a);
    float s = sin(a);
    return float2x2(float2(c, s), float2(-s, c));
}

float crossDist(float3 p)
{
    float3 absp = abs(p);
    float maxyz = max(absp.y, absp.z);
    float maxxz = max(absp.x, absp.z);
    float maxxy = max(absp.x, absp.y);
    float cr = 1.0f - (((step(maxyz, absp.x) * maxyz) + (step(maxxz, absp.y) * maxxz)) + (step(maxxy, absp.z) * maxxy));
    float cu = max(maxxy, absp.z) - 3.0f;
    return max(cr, cu);
}

float fractal(inout float3 p)
{
    float scale = 1.0f;
    float dist = 0.0f;
    for (int i = 0; i < 4; i++)
    {
        float3 param = p;
        dist = max(dist, crossDist(param) * scale);
        p = (frac((p - 1.0f.xxx) * 0.5f) * 6.0f) - 3.0f.xxx;
        scale /= 3.0f;
    }
    return dist;
}

float plane(float3 p)
{
    return dot(p, 0.57735002040863037109375f.xxx) - (smoothstep(0.0500000007450580596923828125f, 1.0f, (sin((iTime * 0.095399998128414154052734375f) - 2.5248000621795654296875f) * 0.5f) + 0.5f) * 6.0f);
}

float de(float3 p)
{
    float3 param = p;
    float _181 = fractal(param);
    float3 param_1 = p;
    return max(_181, plane(param_1));
}

float3 normal(float3 p)
{
    float3 param = p;
    float dd = de(param);
    float3 param_1 = p - float3(0.001000000047497451305389404296875f, 0.0f, 0.0f);
    float3 param_2 = p - float3(0.0f, 0.001000000047497451305389404296875f, 0.0f);
    float3 param_3 = p - float3(0.0f, 0.0f, 0.001000000047497451305389404296875f);
    return normalize(float3(dd - de(param_1), dd - de(param_2), dd - de(param_3)));
}

float3 toColor(float3 normal_1)
{
    float3 color = (normal_1 * 0.5f) + 0.5f.xxx;
    color *= float3(0.89999997615814208984375f, 0.699999988079071044921875f, 0.60000002384185791015625f);
    color.z = (cos(color.z * 4.30000019073486328125f) * 0.20000000298023223876953125f) + 0.800000011920928955078125f;
    return color;
}

float3 toGray(float3 color)
{
    return (((color.x + color.y) + color.z) / 3.0f).xxx;
}

void comp_main()
{
    uint _261_dummy_parameter;
    int2 size = int2(imageSize(uOutputImage, _261_dummy_parameter));
    int2 coord = int2(gl_GlobalInvocationID.xy);
    float2 iResolution = float2(float(size.x), float(size.y));
    float2 uv = ((float2(float(coord.x), float(coord.y)) / iResolution) * 2.0f) - 1.0f.xx;
    uv.x *= (iResolution.x / iResolution.y);
    float3 from = float3(-6.0f, 0.0f, 0.0f);
    float3 dir = normalize(float3(uv * 0.60000002384185791015625f, 1.0f));
    float param = 1.5707499980926513671875f;
    float2 _317 = mul(rot(param), dir.xz);
    dir = float3(_317.x, dir.y, _317.y);
    float3 iMouse = float3(iMouseX, iMouseY, 1.0f);
    float2 mouse = ((iMouse.xy / iResolution) - 0.5f.xx) * 0.5f;
    if (iMouse.z < 1.0f)
    {
        mouse = 0.0f.xx;
    }
    float param_1 = ((sin((iTime * 0.0652000010013580322265625f) - 0.5f) * 0.800000011920928955078125f) + (mouse.x * 5.0f)) + 2.5f;
    float2x2 rotxz = rot(param_1);
    float param_2 = 0.300000011920928955078125f - (mouse.y * 5.0f);
    float2x2 rotxy = rot(param_2);
    float2 _370 = mul(rotxy, from.xy);
    from = float3(_370.x, _370.y, from.z);
    float2 _376 = mul(rotxz, from.xz);
    from = float3(_376.x, from.y, _376.y);
    float2 _382 = mul(rotxy, dir.xy);
    dir = float3(_382.x, _382.y, dir.z);
    float2 _388 = mul(rotxz, dir.xz);
    dir = float3(_388.x, dir.y, _388.y);
    float totdist = 0.0f;
    bool set = false;
    float onPlane = 0.0f;
    float3 norm = 0.0f.xxx;
    float ao = 0.0f;
    float3 p = 0.0f.xxx;
    for (int steps = 0; steps < 150; steps++)
    {
        p = from + (dir * totdist);
        float3 param_3 = p;
        float _417 = fractal(param_3);
        float fdist = _417;
        float3 param_4 = p;
        float pdist = plane(param_4);
        float dist = max(fdist, pdist);
        totdist += dist;
        if (dist < 0.0040000001899898052215576171875f)
        {
            set = true;
            onPlane = abs(fdist - pdist);
            float3 param_5 = p;
            norm = normal(param_5);
            ao = float(steps) / 150.0f;
            break;
        }
    }
    float4 fragColor;
    if (set)
    {
        float3 param_6 = norm;
        float3 surfaceColor = toColor(param_6);
        float3 param_7 = surfaceColor;
        surfaceColor = lerp(surfaceColor, toGray(param_7), 0.20000000298023223876953125f.xxx);
        surfaceColor = (surfaceColor * 0.800000011920928955078125f) + 0.20000000298023223876953125f.xxx;
        fragColor = float4(surfaceColor.x, surfaceColor.y, surfaceColor.z, fragColor.w);
        float3 _478 = fragColor.xyz - (totdist * 0.039999999105930328369140625f).xxx;
        fragColor = float4(_478.x, _478.y, _478.z, fragColor.w);
        float3 _488 = fragColor.xyz - (smoothstep(0.0f, 0.300000011920928955078125f, ao) * 0.4000000059604644775390625f).xxx;
        fragColor = float4(_488.x, _488.y, _488.z, fragColor.w);
        float3 _500 = fragColor.xyz + ((surfaceColor * (1.0f - smoothstep(0.0f, 0.0199999995529651641845703125f, onPlane))) * 0.800000011920928955078125f);
        fragColor = float4(_500.x, _500.y, _500.z, fragColor.w);
    }
    else
    {
        float3 param_8 = -dir;
        float3 _507 = toColor(param_8);
        fragColor = float4(_507.x, _507.y, _507.z, fragColor.w);
        float3 param_9 = fragColor.xyz;
        float3 _518 = lerp(toGray(param_9), fragColor.xyz, 0.4000000059604644775390625f.xxx) * 0.800000011920928955078125f;
        fragColor = float4(_518.x, _518.y, _518.z, fragColor.w);
    }
    float3 _525 = clamp(fragColor.xyz, 0.0f.xxx, 1.0f.xxx);
    fragColor = float4(_525.x, _525.y, _525.z, fragColor.w);
    fragColor.w = 1.0f;
    uOutputImage[coord] = fragColor;
}

[numthreads(16, 16, 1)]
void main(Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
