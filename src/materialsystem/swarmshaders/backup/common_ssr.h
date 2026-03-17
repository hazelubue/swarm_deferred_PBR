
#include "stochasticSSRHF.h"
#include "ShaderInterop_Postprocess.h"

float3 mix(float3 a, float3 b, float t)
{
    return a + (b - a) * t;
}


// Pitch must be in the range of [-90 ... 90] degrees and 

// yaw must be in the range of [0 ... 360] degrees.

// Pitch and yaw variables must be expressed in radians.


float4x4 FPSViewRH(float3 eye, float pitch, float yaw)

{


    // I assume the values are already converted to radians.


    float cosPitch = cos(pitch);


    float sinPitch = sin(pitch);


    float cosYaw = cos(yaw);


    float sinYaw = sin(yaw);




    float3 xaxis = { cosYaw, 0, -sinYaw };


    float3 yaxis = { sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };


    float3 zaxis = { sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };




    // Create a 4x4 view matrix from the right, up, forward and eye position vectors


    float4x4 viewMatrix = {


        float4(xaxis.x,            yaxis.x,            zaxis.x,      0),


        float4(xaxis.y,            yaxis.y,            zaxis.y,      0),


        float4(xaxis.z,            yaxis.z,            zaxis.z,      0),


        float4(-dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1)


    };

    return viewMatrix;
}

float4x4 GetProj_Manual(float fovY, float aspect, float zNear, float zFar)
{
    float f = 1.0f / tan(fovY * 0.5f);
    return float4x4(
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (zFar + zNear) / (zNear - zFar), (2 * zFar * zNear) / (zNear - zFar),
        0, 0, -1, 0
    );
}

float4x4 GetViewProj_Manual()
{
    float4x4 g_Manualview = FPSViewRH(g_eyePos, g_pitch, g_yaw);
    float4x4 g_Manualproj = GetProj_Manual(FIELD_OF_VIEW, ASPECT_RATIO, g_zNear, g_zFar);
    //return 0;
    return mul(g_Manualview, g_Manualproj);
}

// PROPER CLIP METHOD

float4 ClipToScreen(in float4 clipPosition)
{
    float3 ndc = clipPosition.xyz * rcp( clipPosition.w );

    float4 screenPos = (float4)0;

    screenPos.y = float(0.5f - ndc.y * 0.5f);
    screenPos.x = float(ndc.x * 0.5f + 0.5f);
    screenPos.z = float(ndc.z * 0.5f + 0.5f);
    screenPos.w = clipPosition.w;
    return screenPos;
}

// from saruna also correct math

float2 ClipToScreenXY(float2 UV)
{
    return float2(UV.x * 0.5f + 0.5f, -UV.y * 0.5f + 0.5f);
}

float3 UVtoViewPos(float2 uv)
{
    float3 frustrumRay = g_vecFrustumCenter +
        uv.x * g_vecFrustumRight +
        uv.y * g_vecFrustumUp;

    return frustrumRay;
}

//float4x4 GetViewProjInv_UVtoVp(float3 viewPos)
//{
//    float4 viewProjPos = mul(float4(viewPos, 1.0), g_Proj);
//
//    return inverse(viewProjPos);
//}

float2 WorldToUV(in float3 viewPos)
{
    float4 viewProjPos = mul(float4(viewPos, 1.0), g_Proj);
    viewProjPos.xyz /= viewProjPos.w;
    return ClipToScreenXY(viewProjPos.xy);
}

// standard view 

float4 WorldToUV_ViewProj(in float3 position)
{
    float4 clipPos = mul(float4(position, 1.0f), GetViewProj());
    return ClipToScreen(clipPos);
}

// view to projection

float4 WorldToClipUV(in float3 position)
{

    //float4x4 g_m_View = manual_ViewMatrix(g_View);

    float4 clipPos = mul(mul(float4(position, 1.0f), g_View), g_Proj);
    return ClipToScreen(clipPos);
}

// world to view

float3 WorldToView(in float3 viewPos)
{
    float4 viewProjPos = mul(float4(viewPos, 1.0), g_Proj);
    return viewProjPos.xyz;
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
    worldPos.xyz *=rcp(worldPos.w );
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

float3 readWorld(in float2 UV, sampler input_depth)
{
    float depth = LinearizeDepth(tex2D(input_depth, UV).a, g_zFar, g_zNear);
    float2 screenUV = UV * 2.0f - 1.0f;
    float3 viewRay = normalize(g_vecFrustumCenter + g_vecFrustumRight * screenUV.x - g_vecFrustumUp * screenUV.y);
    float distance = depth / dot(viewRay, g_ViewForward);
    return g_ViewOrigin + viewRay * distance;
}

float3 perform_iblTaps(sampler input_sampler, float2 UV)
{
    float3 color = (float3)0;

    float3 color_base = tex2D(input_sampler, UV);
    float3 color_right = tex2D(input_sampler, UV + float2(g_vecFullScreenTexel.x, 0));
    float3 color_left = tex2D(input_sampler, UV + float2(-g_vecFullScreenTexel.x, 0));
    float3 color_up = tex2D(input_sampler, UV + float2(0, g_vecFullScreenTexel.y));
    float3 color_down = tex2D(input_sampler, UV + float2(0, -g_vecFullScreenTexel.y));

    return color = (color_base + color_right + color_left + color_up + color_down) * rcp(100.0f);
}

// blur kernel for IBL adjustable via cpu.

float3 perform_iblSample(sampler input_sampler, float2 UV)
{
    float3 average_color = (float3)0;

    int x = (float3)0;
    int y = (float3)0;

    // figure out how to declare this without exceeding maximum temp register limit.
    //int blurAmount = (int)g_iblBlurAmt;
    //blurAmount = max(1, min(blurAmount, 8)); 
    int totalSamples = 0;

    [unroll]
    for (int y = 0; y < 6; y++)
    {
        [unroll]
        for (int x = 0; x < 6; x++)
        {
            //if (x >= blurAmount || y >= blurAmount) continue;

            float2 sampleUV = float2(UV.x + (x - 1.5f) * 0.0025f,
                                     UV.y + (y - 1.5f) * 0.0025f);
            average_color += perform_iblTaps(input_sampler, sampleUV);
            totalSamples++;
        }
    }
#if DEBUG_FXC
    return float3(UV.x + (x - 1.5f) * 0.0025f,
                  UV.y + (y - 1.5f) * 0.0025f, 0.0);
#endif
    average_color *= rcp((float)totalSamples);
    return average_color;
}

static float3 g_cachedIBL = (float3)0;


// TODO - rewrite this whole thing

// TLDR - we mimic reflection scale it how we scaled IBL originally and blur it.

float3 performIBL_screespace(float2 uv, float3 worldNormals, float roughness, float3 view, sampler input_sampler, sampler input_depth, float depthMask)
{
    // real function,  non - static variant takes g_View as input.

    //float3 currentcamPos = matrixViewtoPosition(g_staticView);

    float3 V = normalize(-view);

        // use -view for original calculation use ONLY view it already calculates direction along surface point.
        // ss ibl
        // NOTE need to scale viewport down HALF then up. this is to create proper ambient reflections without side views interferring as well as to make them smooth.

        float sceneDepth = tex2Dlod(input_depth, float4(uv.xy, 0, 0)).r;

        float3 iblRayPos = (float3)0;

        float3 viewRay = normalize(g_ViewForward + g_ViewRight * uv.x - g_ViewUp * uv.y);
        float distance = sceneDepth / dot(viewRay, g_ViewForward);
        float3 WorldPosition = g_ViewOrigin + viewRay * distance;

        iblRayPos = WorldPosition;

        float3 ibl = (float3)0;

        float4 H = (float4)0;

        if (roughness > 0.05f)
        {
            float3x3 tangentBasis = GetTangentBasis(worldNormals);
            float3 tangentV = mul(tangentBasis, -V);

            const float2 bluenoise = blue_noise(uv).xy;
            float2 Xi = bluenoise.xy;
            Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

            H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
            H.xyz = mul(H.xyz, tangentBasis);
            ibl = reflect(V, H.xyz);
        }
        else
        {
            ibl = reflect(V, worldNormals);
        }

        float3 RayDirIBL = normalize(ibl);
        float RayStepIBL = (0.2 + 0.05) * sqrt(sceneDepth) * rcp(1e-3 + saturate(1 - dot(RayDirIBL, -view)));

        float2 iblUV = (float2)0;
        float3 viewPos = (float3)0;

        float g_flRayStepDiv = 0.5;
        float g_flDepthDiffMax = 0.01;
        int maxIterations = 8;
        int refinementSteps = 4;

        int j = 0;
        while (j++ < maxIterations)
        {
            //convert to ray in world pos
            float3 RayWorldPos = readWorld(WorldToUV(RayCurPos.xyz), input_depth);
            //make projectable uv.
            float2 iblScreen = WorldToUV(RayCurPos.xyz);
            iblUV = iblScreen.xy;
          
            sceneDepth = tex2Dlod(input_depth, float4(iblUV.xy, sceneDepth, 0)).r;

            for (int i = 0; i < refinementSteps; i++)
            {
                iblRayPos.xyz -= RayDirIBL * RayStepIBL;
                RayStepIBL *= g_flRayStepDiv;
            }

            iblRayPos += RayDirIBL * RayStepIBL;
            RayStepIBL = length(iblRayPos.xyz - RayWorldPos) * g_flRayStepDiv;
        }
        
        g_cachedIBL = perform_iblSample(input_sampler, iblUV) * depthMask;

    return g_cachedIBL;
}

float3 perform_ssrTaps(sampler input_sampler, float2 UV)
{
    float3 color = (float3)0;

    float3 color_base = tex2D(input_sampler, UV);
    float3 color_right = tex2D(input_sampler, UV + float2(g_vecFullScreenTexel.x, 0));
    float3 color_left = tex2D(input_sampler, UV + float2(-g_vecFullScreenTexel.x, 0));
    float3 color_up = tex2D(input_sampler, UV + float2(0, g_vecFullScreenTexel.y));
    float3 color_down = tex2D(input_sampler, UV + float2(0, -g_vecFullScreenTexel.y));

    return color = (color_base + color_right + color_left + color_up + color_down) * rcp(10.0f);
}

float3 perform_ssrSample(sampler input_sampler, float2 UV, float roughness)
{
    float3 average_color = (float3)0;

    int x = (float3)0;
    int y = (float3)0;

    float minSSR = 0.05;
    float a = max(1.0f - roughness, minSSR);
    float aOffset = 0.0025f * (a * a * a);

    [unroll]
    for (y = 0; y < 4; y++)
    {
        [unroll]
        for (x = 0; x < 4; x++)
        {
            float2 sampleUV = float2(UV.x + (x - 1.5f) * aOffset,
                UV.y + (y - 1.5f) * aOffset);
            average_color += perform_ssrTaps(input_sampler, sampleUV);

        }

    }
#if DEBUG_FXC
    return float3(UV.x + (x - 1.5f) * 0.0025f,
        UV.y + (y - 1.5f) * 0.0025f, 0.0);
#endif

    average_color *= rcp(16.0f);
    return average_color;
}

float3 ssrComposition_simple(float roughness, float metallic, float2 uv, float3 N, sampler input_sampler, sampler input_depth, float4 worldPos, float3 view)
{

    float2 screenUV = uv.xy;
    float3 WorldNormals = N;

    float4 vWorldPos = worldPos;

    // view
    // reflection UV construced from views against normals.

    float sceneDepth = tex2Dlod(input_depth, float4(screenUV.xy, 0, 0)).r;

    // why are we reconstructing world pos instead of using the perfectly valid one provided?
    float3 viewRay = normalize(g_ViewForward + g_ViewRight * screenUV.x - g_ViewUp * screenUV.y);
    float distance = sceneDepth / dot(viewRay, g_ViewForward);
    float3 WorldPosition = g_ViewOrigin + viewRay * distance;

    float3 V = normalize(-view);
    float3 reflection = normalize(reflect(V, WorldNormals));

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

    float3 RayDir = normalize(reflection);

    float3 RayStart = WorldPosition;

    float3 RayCurPos = RayStart;
    float RayStep = (0.2 + 0.05) * sqrt(sceneDepth) * rcp(1e-3 + saturate(1 - dot(RayDir, -view)));

    //float3 reflectionUV = float3(screenUV.xy, sceneDepth);
    float2 reflectionUV = (float2)0;
    float2 iblUV = (float2)0;

    float3 reflectionWorldPos = (float3)0;
    float2 rayScreen = (float2)0;
    float3 viewPos = (float3)0;

    float depth_current = sceneDepth / 100000.0f;
    float depth_compare = readDepth_Normalized(WorldToView(viewPos.xyz), input_depth) + 0.1f;

    float g_flRayStepDiv = 0.5;
    float g_flDepthDiffMax = 0.01;
    int maxIterations = 8;
    int refinementSteps = 4;
    float depthMask = 1.0f;


    int j = 0;
    while (j++ < maxIterations)
    {

        float3 RayWorldPos = readWorld(WorldToUV(viewPos.xyz), input_depth);

        rayScreen = WorldToUV(viewPos.xyz);
        reflectionUV = rayScreen.xy;

        sceneDepth = tex2Dlod(input_depth, float4(reflectionUV.xy, sceneDepth, 0)).r;

        for (int i = 0; i < refinementSteps; i++)
        {
            RayCurPos.xyz -= RayDir * RayStep;
            RayStep *= g_flRayStepDiv;

            depth_current = sceneDepth / 100000.0f;
            depth_compare = readDepth_Normalized(WorldToView(viewPos.xyz), input_depth);

            float depthDiff = abs(depth_compare - depth_current);
            depthMask = step(depthDiff, g_flDepthDiffMax);
        }

        RayCurPos.xyz += RayDir * RayStep;
        RayStep = length(RayCurPos.xyz - RayWorldPos) * g_flRayStepDiv;
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

    // scale down viewport half - stretch the screen corners to the edge of the screen.

    //float center = 0.5f;
    //float2 vScaled = (V.xy - center) * 0.5f + center;
    //float2 vStretched = (vScaled - 0.25f) / 0.5f;

    //float2 vClip = vStretched * 2.0f - 1.0f; //0.0 - 1 [ -1 to 1] vClip * -1.0 + 0.0
    //float vMask = length(-vClip);
    //vMask *= vMask;
    //vMask = 1.0f - vMask;

    float2 screenClip = uv * 2.0f - 1.0f;
    float vMask = length(screenClip);
    vMask *= vMask;
    vMask = 1.0f - vMask;

    // yo use that reflection that we have made, use it against the current frame !!
    // and the ibl

    //float3 reflectionframe = tex2D(input_sampler, reflectionUV);
    float3 iblframe = performIBL_screespace(uv, WorldNormals, roughness, view, input_sampler, input_depth, depthMask);
    float3 tappedSSRresult = perform_ssrSample(input_sampler, reflectionUV, roughness);

    float ssrWeight = UVmask * mask * depthMask;

    // attemp to cut off IBL when reflection is in bounds.wrks but inverts colors, shows ibl only on some surfaces, fucks with viewports.
    //float3 adjustedIblIrradiance = iblframe * (1.0f - ssrWeight);
    //float3 finalReflector = adjustedIblIrradiance + tappedSSRresult * ssrWeight;

    float3 finalReflector = lerp(iblframe * vMask, tappedSSRresult, ssrWeight);

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

