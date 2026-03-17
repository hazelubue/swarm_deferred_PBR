
#include "stochasticSSRHF.h"
#include "ShaderInterop_Postprocess.h"

float3 mix(float3 a, float3 b, float t)
{
    return a + (b - a) * t;
}

float3 WorldToView(in float3 position)
{
    float4 viewPos = mul(float4(position, 1.0f), g_View);
    return viewPos.xyz;
}
//float3 ClipToScreen(float3 UV)
//{
//    /* return float3(UV.x * 0.5f + 0.5f, 0.5f - UV.y * 0.5f, UV.z * 0.5f + 0.5f);*/
//    float x = float(UV.x * 0.5f + 0.5f);
//    float y = float(0.5f - UV.y * 0.5f);
//    float z = float(UV.z * 0.5f + 0.5f);
//
//    return float3(x, y, z);
//}
//
//float3 WorldToUV(in float3 position)
//{
//    float4 viewProj = mul(float4(position, 1.0f), GetViewProj());
//    viewProj.xyz /= viewProj.w;
//    viewProj.xyz * 2.0f - 1.0f;
//    float3 clipView = ClipToScreen(viewProj.xyz);
//    //clipView.x = 1.0 -clipView.x;
//    return clipView.xyz;
//}

// unrolled view matrix for modification

float4x4 manual_ViewMatrix(float4x4 view)
{
    float3 right = float3(view[0][0], view[1][0], view[2][0]);
    float3 up = float3(view[0][1], view[1][1], view[2][1]);
    float3 forward = float3(view[0][2], view[1][2], view[2][2]);

    // GOT YA BITCH

    float3 cameraPosition = float3(
        -dot(float3(view[0][0], view[0][1], view[0][2]), float3(view[3][0], view[3][1], view[3][2])),
        -dot(float3(view[1][0], view[1][1], view[1][2]), float3(view[3][0], view[3][1], view[3][2])),
        -dot(float3(view[2][0], view[2][1], view[2][2]), float3(view[3][0], view[3][1], view[3][2])));

    float pitch = asin(-forward.y);
    float yaw = atan2(forward.x, forward.z);
    float roll = atan2(-right.y, up.y);

    float cp = cos(pitch);
    float sp = sin(pitch);
    float cy = cos(yaw);
    float sy = sin(yaw);
    float cr = cos(roll);
    float sr = sin(roll);

    float3 reconstructed_right = float3(
        cy * cr - sy * sp * sr,
        cp * sr,
        sy * cr + cy * sp * sr);

    float3 reconstructed_up = float3(
        -cy * sr - sy * sp * cr,
        cp * cr,
        -sy * sr + cy * sp * cr);

    float3 reconstructed_forward = float3(
        sy * cp,
        -sp,
        cy * cp);

    float4x4 viewMatrix = float4x4(
        float4(reconstructed_right.x, reconstructed_right.y, reconstructed_right.z, 0),
        float4(reconstructed_up.x, reconstructed_up.y, reconstructed_up.z, 0),
        float4(reconstructed_forward.x, reconstructed_forward.y, reconstructed_forward.z, 0),
        float4(-dot(reconstructed_right, cameraPosition), -dot(reconstructed_up, cameraPosition), -dot(reconstructed_forward, cameraPosition), 1.0));

    return viewMatrix;
}

// PROPER CLIP METHOD

float4 ClipToScreen(in float4 clipPosition)
{
    float3 ndc = clipPosition.xyz * rcp(clipPosition.w);

    float4 screenPos = (float4)0;

    screenPos.y = float(0.5f - ndc.y * 0.5f);
    screenPos.x = float(ndc.x * 0.5f + 0.5f);
    screenPos.z = float(ndc.z * 0.5f + 0.5f);
    screenPos.w = clipPosition.w;
    return screenPos;
}

// standard view 

float4 WorldToUV_ViewProj(in float3 position)
{
    float4 clipPos = mul(float4(position, 1.0f), GetViewProj());
    //clipPos.xyz /= clipPos.w;
    ////viewProj.xyz *= 2.0f - 1.0f;
    //float4 clipView = ClipToScreen(clipPos.xyz);
    //clipView.xyz *= rcp(clipView.w);
    //clipView.x = 1.0 -clipView.x;
    //return clipView.xyzw;
    return ClipToScreen(clipPos);
}

// view to projection

float4 WorldToClipUV(in float3 position)
{

    //float4x4 g_m_View = manual_ViewMatrix(g_View);

    float4 clipPos = mul(mul(float4(position, 1.0f), g_View), g_Proj);
    return ClipToScreen(clipPos);
}

float3 ScreenToClip(float3 screenUV)
{
    return float3(screenUV.x * 2.0f - 1.0f, 1.0f - screenUV.y * 2.0f, screenUV.z * 2.0f - 1.0f);
}

// might have some useful realism stuff in accordance to reflection visual rendering not particularly reflection view construction.

float3 readPos(in float3 coord)
{
    float3 clipPos = ScreenToClip(coord);

    float4 worldPos = mul(float4(clipPos.xyz, 1.0f), GetViewProjInv());
    worldPos.xyz *= rcp(worldPos.w);
    return float3(worldPos.xyz);
}

float LinearizeDepth(float d, float f, float n)
{
    return n * f / (f + d * (n - f));
}

float3 readView(in float2 coord, sampler input_depth)
{
    float2 invResolution = 1.0f.xx / g_vecFullScreenTexel;
    float3 pos = tex2D(input_depth, float2(coord.x + invResolution.x / 2.0f, coord.y + invResolution.y / 2.0f)).xyz;
    return float3(pos.x, pos.y, pos.z);
}

float readDepth_Normalized(in float2 coord, sampler input_depth)
{
    float3 viewPos = readView(coord, input_depth);
    float depth_linear = LinearizeDepth(viewPos.z, g_zFar, g_zNear);

    return depth_linear / 4000.0f;
}

float3 ScreenToWorld(in float3 screenUV)
{
    float3 clipPos = ScreenToClip(screenUV);
    float4 worldPos = mul(float4(clipPos, 1.0f), GetViewProjInv());
    return worldPos.xyz / worldPos.w;
}

float2 get_uv_from_position(in float3 pos, in float3 uvtoviewMUL, in float3 uvtoviewADD)
{
    return pos.xy / (uvtoviewMUL.xy * pos.z) - uvtoviewADD.xy / uvtoviewMUL.xy;
}

float4 ViewToUV(in float3 viewPos)
{
    float4 clipPos = mul(float4(viewPos, 1.0f), g_Proj);
    clipPos.xyz /= clipPos.w;
    return ClipToScreen(clipPos);
}

// extracted camera position from g_View;

float3 matrixViewtoPosition(float4x4 m)
{
    float3 right = float3(m._m00, m._m10, m._m20);
    float3 up = float3(m._m01, m._m11, m._m21);
    float3 forward = float3(m._m02, m._m12, m._m22);
    float3 translation = float3(m._m30, m._m31, m._m32);

    float3 cameraPosition = float3(
        -dot(right, translation),
        -dot(up, translation),
        -dot(forward, translation));

    return cameraPosition;
}

float3 perform_iblSample(sampler input_sampler, float2 UV)
{
    float3 color = (float3)0;

    float3 color_base = tex2D(input_sampler, UV);
    float3 color_right = tex2D(input_sampler, UV + float2(g_vecFullScreenTexel.x, 0));
    float3 color_left = tex2D(input_sampler, UV + float2(-g_vecFullScreenTexel.x, 0));
    float3 color_up = tex2D(input_sampler, UV + float2(0, g_vecFullScreenTexel.y));
    float3 color_down = tex2D(input_sampler, UV + float2(0, -g_vecFullScreenTexel.y));

    color = (color_base + color_right + color_left + color_up + color_down) * rcp(10.0f);

    return color;
}

static float3 g_lastcamPos = (float3)0;
static float3 g_cachedIBL = (float3)0;
static bool g_iblNeedsUpdate = true;
static float g_camMovementThreshold = 0.1f;

// TODO - rewrite this whole thing

// TLDR - we mimic reflection scale it how we scaled IBL originally and blur it.

float3 UpdateIBL_OnWorldMovement(float2 uv, float3 worldNormals, float roughness, float3 view, sampler input_sampler, sampler input_depth, float depthMask)
{
    // real function,  non - static variant takes g_View as input.

    //float3 currentcamPos = matrixViewtoPosition(g_staticView);

    float3 V = normalize(-view);

    float3 movementDelt = V - g_lastcamPos;
    float movementDist = length(movementDelt);

    if (movementDist > g_camMovementThreshold || g_iblNeedsUpdate)
    {

        // use -view for original calculation use ONLY view it already calculates direction along surface point.
        // ss ibl
        // NOTE need to scale viewport down HALF then up. this is to create proper ambient reflections without side views interferring as well as to make them smooth.

        //float3 V = normalize(-view);

        float2 texeltoCamera = float2(g_vecFullScreenTexel.x / V.x,
            g_vecFullScreenTexel.y / V.y);

        float2 hSFator = float2(0.5f, 0.5f);
        float2 scaledTexel = texeltoCamera * hSFator;

        float vOffset = float3(scaledTexel.x, scaledTexel.y, 0) * 1000.0f;

        float3 cameraPositionScaled = V + vOffset;
        cameraPositionScaled += worldNormals * 9990.0f;
        float3 V2 = normalize(cameraPositionScaled);

        float sceneDepth = tex2Dlod(input_depth, float4(uv.xy, 0, 0)).r;


        float3 iblRayPos = (float3)0;

        float3 rayStartWorld = ScreenToWorld(float3(uv.xy, sceneDepth));

        iblRayPos = rayStartWorld;

        float3 ibl = (float3)0;

        float4 H = (float4)0;

        if (roughness > 0.05f)
        {
            float3x3 tangentBasis = GetTangentBasis(worldNormals);
            float3 tangentV = mul(tangentBasis, -V2);

            const float2 bluenoise = blue_noise(uv).xy;
            float2 Xi = bluenoise.xy;
            Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

            H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
            H.xyz = mul(H.xyz, tangentBasis);
            ibl = reflect(V2, H.xyz);
        }
        else
        {
            ibl = reflect(V2, worldNormals);
        }

        float3 RayDirIBL = normalize(ibl);
        float RayStepIBL = (0.2 + 0.05) * sqrt(sceneDepth) * rcp(1e-3 + saturate(1 - dot(RayDirIBL, -view)));

        float2 iblUV = (float2)0;

        float g_flRayStepDiv = 0.5;
        float g_flDepthDiffMax = 0.01;
        int maxIterations = 8;
        int refinementSteps = 4;
        //float depthMask = 1.0f;

        //float depth_current = sceneDepth / 100000.0f;
        //float depth_compare = readDepth_Normalized(WorldToClipUV(iblRayPos.xyz), input_depth) + 0.1f;

        int j = 0;
        while (j++ < maxIterations)
        {

            float4 iblScreen = WorldToClipUV(iblRayPos.xyz);
            iblUV = iblScreen.xy;

            sceneDepth = tex2Dlod(input_depth, float4(iblUV.xy, sceneDepth, 0)).r;
            float3 iblWorldPos = readPos(float3(iblUV, sceneDepth));

            for (int i = 0; i < refinementSteps; i++)
            {
                iblRayPos.xyz -= RayDirIBL * RayStepIBL;
                RayStepIBL *= g_flRayStepDiv;

                //depth_current = sceneDepth / 100000.0f;
                //depth_compare = readDepth_Normalized(WorldToClipUV(iblRayPos.xyz), input_depth);

                //float depthDiff = abs(depth_compare - depth_current);
                //depthMask = step(depthDiff, g_flDepthDiffMax);
            }

            iblRayPos = V.xyz + RayDirIBL * RayStepIBL;
            RayStepIBL = length(iblRayPos.xyz - iblWorldPos) * g_flRayStepDiv;
        }

        g_cachedIBL = perform_iblSample(input_sampler, iblUV) * depthMask;

        g_lastcamPos = V;
        g_iblNeedsUpdate = false;
    }

    return g_cachedIBL;
}

float3 ssrComposition_simple(float roughness, float metallic, float2 uv, float3 N, sampler input_sampler, sampler input_depth, float4 worldPos, float3 view)
{

    float2 screenUV = uv.xy;
    float3 WorldNormals = N;

    float4 vWorldPos = worldPos;

    // view

    float3 V = normalize(-view);

    // reflection UV construced from views against normals.

    float3 reflection = (float3)0;

    // Importance sampling normal

    float4 H = (float4)0;

    // Importance sampling

    if (roughness > 0.05f)
    {
        float3x3 tangentBasis = GetTangentBasis(WorldNormals);
        float3 tangentV = mul(tangentBasis, -V);

        const float2 bluenoise = blue_noise(uv).xy;
        float2 Xi = bluenoise.xy;
        Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

        H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
        H.xyz = mul(H.xyz, tangentBasis);
        reflection = reflect(V, H.xyz);
    }
    else
    {
        reflection = reflect(V, WorldNormals);
    }

    float sceneDepth = tex2Dlod(input_depth, float4(screenUV.xy, 0, 0)).r;

    float3 rayStartWorld = ScreenToWorld(float3(screenUV.xy, sceneDepth));
    // this was changed when converting to g_proj to g_view pipeline
    //float3 rayStartView = WorldToView(rayStartWorld);

    // keep in world space.
    float3 rayStartView = rayStartWorld;

    float3 RayDir = normalize(reflection);

    float3 RayCurPos = rayStartView.xyz;

    float RayStep = (0.2 + 0.05) * sqrt(sceneDepth) * rcp(1e-3 + saturate(1 - dot(RayDir, -view)));

    //float3 reflectionUV = float3(screenUV.xy, sceneDepth);
    float2 reflectionUV = (float2)0;
    float2 iblUV = (float2)0;

    float3 reflectionWorldPos = (float3)0;
    float4 rayScreen = (float4)0;

    float depth_current = sceneDepth / 100000.0f;
    float depth_compare = readDepth_Normalized(WorldToClipUV(RayCurPos.xyz), input_depth) + 0.1f;


    float g_flRayStepDiv = 0.5;
    float g_flDepthDiffMax = 0.01;
    int maxIterations = 8;
    int refinementSteps = 4;
    float depthMask = 1.0f;

    int j = 0;
    while (j++ < maxIterations)
    {
        rayScreen = WorldToClipUV(RayCurPos.xyz);
        //rayScreen = ClipToScreen(RayCurPos.xyz);

        // PROPER CLIP METHOD
        //float3 NDCpos = rayScreen.xyz * rcp(rayScreen.w);
        //reflectionUV = NDCpos.xy * 0.5f + 0.5f;
        //reflectionUV.y = 1.0 - reflectionUV.y;

        //old
        //reflectionUV = float3(rayScreen.xy, sceneDepth) * 2.0f - 1.0f;
        reflectionUV = rayScreen.xy;
        //reflectionUV.y = 1.0 - reflectionUV.y;

        //float aspectRatio = g_vecFullScreenTexel.x / g_vecFullScreenTexel.y;
        //reflectionUV = float3((rayScreen.xy * 2.0f - 1.0f) * float2(1.0f, aspectRatio), sceneDepth * 2.0f - 1.0f);

        sceneDepth = tex2Dlod(input_depth, float4(reflectionUV.xy, sceneDepth, 0)).r;
        reflectionWorldPos = readPos(float3(reflectionUV, sceneDepth));

        for (int i = 0; i < refinementSteps; i++)
        {
            RayCurPos.xyz -= RayDir * RayStep;
            RayStep *= g_flRayStepDiv;

            depth_current = sceneDepth / 100000.0f;
            depth_compare = readDepth_Normalized(WorldToClipUV(RayCurPos.xyz), input_depth);

            float depthDiff = abs(depth_compare - depth_current);
            depthMask = step(depthDiff, g_flDepthDiffMax);
        }

        RayCurPos.xyz += RayDir * RayStep;
        RayStep = length(RayCurPos.xyz - reflectionWorldPos) * g_flRayStepDiv;
    }

    float UVmask = (reflectionUV.x < 0 || reflectionUV.x > 1 ? 0 : 1)
        * (reflectionUV.y < 0 || reflectionUV.y > 1 ? 0 : 1);

    reflectionUV = saturate(reflectionUV);
    float2 reflectionClip = reflectionUV * 2.0f - 1.0f;
    float mask = length(reflectionClip);
    mask *= mask;
    mask = 1.0f - mask;

    float fresnelMask = dot(-view, WorldNormals);
    fresnelMask = saturate(fresnelMask);
    fresnelMask = 1.0f - fresnelMask;

    fresnelMask = lerp(fresnelMask, pow(fresnelMask, 5.0f), 1.0f - metallic);

    // yo use that reflection that we have made, use it against the current frame !!
    // and the ibl

    float3 reflectionframe = tex2D(input_sampler, reflectionUV);
    float3 iblframe = UpdateIBL_OnWorldMovement(uv, WorldNormals, roughness, view, input_sampler, input_depth, depthMask);

    float ssrWeight = UVmask * mask * depthMask;
    //float3 finalReflector = lerp(iblframe, reflectionframe, ssrWeight);
    float3 finalReflector = mix(iblframe, reflectionframe, ssrWeight);

    return float3(finalReflector * fresnelMask);

    //return float3(reflectionframe * fresnelMask * UVmask * mask * depthMask);
}

float3 readNormals(in float2 coord, sampler2D input_normal_sampler)
{
    return tex2D(input_normal_sampler, float2(coord.x + g_vecFullScreenTexel.x / 2.0f, coord.y + g_vecFullScreenTexel.y / 2.0f)).xyz;
}

float readDepth(in float2 coord, sampler input_depth_sampler)
{
    float depth = tex2D(input_depth_sampler, float2(coord.x + g_vecFullScreenTexel.x / 2.0f, coord.y + g_vecFullScreenTexel.y / 2.0f)).r;
    float depth_linear = LinearizeDepth(depth, g_zFar, g_zNear);

    return depth_linear / 4000.0f;
}

float3 tex2Ddepth(float roughness, float metallic, float3 N, float3 desireUV, float2 UV, sampler input_depth_sampler, sampler2D input_normal_sampler, sampler input_sampler, float4 worldPos, float3 view)
{
    float3 blurUV = float3(desireUV.x, desireUV.z, desireUV.y);

#if HORIZONTAL == 1
    float2 offsetUV = float2(blurUV.x + blurUV.z * g_vecFullScreenTexel.x, blurUV.y);
#else
    float2 offsetUV = float2(blurUV.x, blurUV.y + blurUV.z * g_vecFullScreenTexel.y);
#endif

    float3 compare = ssrComposition_simple(roughness, metallic, offsetUV, N, input_sampler, input_depth_sampler, worldPos, view);
    float compare_depth = readDepth(offsetUV, input_depth_sampler);
    float3 compare_normal = normalize(readNormals(offsetUV, input_normal_sampler));

    float3 result = ssrComposition_simple(roughness, metallic, UV, N, input_sampler, input_depth_sampler, worldPos, view);
    float result_depth = readDepth(UV, input_depth_sampler);
    float3 result_normal = normalize(readNormals(UV, input_normal_sampler));

    return (abs(compare_depth - result_depth) > g_blurArea || dot(compare_normal, result_normal) < g_blurAngle) ? result : compare;
}

//float3 perform_GuassianDepthAwareRoughness_SSR(float roughness, float metallic, float3 N, float2 uv, sampler input_sampler, sampler input_depth_sampler, sampler2D input_normal_sampler, float4 worldPos, float3 view)
//{
//    float blurSize = g_blurSize * roughness;
//
//    float3 sum = 0;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * 4, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.0162162162;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * 3, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.0540540541;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * 2, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.1216216216;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * 1, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.1945945946;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, 0, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.2270270270;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * -1, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.1945945946;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * -2, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.1216216216;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * -3, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.0540540541;
//    sum += tex2Ddepth(roughness, metallic, N, float3(uv.x, blurSize * -4, uv.y), uv, input_depth_sampler, input_normal_sampler, input_sampler, worldPos, view) * 0.0162162162;
//
//    return sum;
//}

float3 perform_GuassianDepthAwareRoughness_SSR(float roughness, float metallic, float3 N, float2 uv, sampler input_sampler, sampler input_depth_sampler, sampler2D input_normal_sampler, float4 worldPos, float3 view)
{
    float blurSize = g_blurSize * roughness;

    float2 noise = blue_noise(uv).xy;
    float angle = noise.x * 6.28318530718;

    float3 sum = 0;
    float totalWeight = 0;

    for (int i = -4; i <= 4; i++)
    {
        float offset = float(i) * blurSize;

        offset += (noise.y * 2.0 - 1.0) * blurSize * 0.5;

#if HORIZONTAL == 1
        float2 sampleUV = uv + float2(offset * g_vecFullScreenTexel.x, 0);
#else
        float2 sampleUV = uv + float2(0, offset * g_vecFullScreenTexel.y);
#endif

        float weights[9] = { 0.0162162162, 0.0540540541, 0.1216216216, 0.1945945946, 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };
        float weight = weights[i + 4];

        float3 sample = ssrComposition_simple(roughness, metallic, sampleUV, N, input_sampler, input_depth_sampler, worldPos, view);
        sum += sample * weight;
        totalWeight += weight;
    }
    return sum / totalWeight;
}