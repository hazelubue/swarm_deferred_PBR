#include "stochasticSSRHF.h"
#include "ShaderInterop_Postprocess.h"

static const float traceThickness = 1.5;
static const float blendScreenEdgeFade = 5.0f;

static const float HiZTraceMostDetailedLevel = 0.0;
static const float HiZTraceIterationsMax = 64;

static const int downsampleFactor = 2;

float2 GetMipResolution(float2 screenDimensions, int mipLevel)
{
    return screenDimensions * pow(0.5, mipLevel);
}

void InitialAdvanceRay(float3 origin, float3 direction, float2 currentMipResolution, float2 currentMipResolution_rcp, float2 floorOffset, float2 uvOffset, out float3 position, out float tCurrent)
{
    float2 currentMipPosition = currentMipResolution * origin.xy;

    // Intersect ray with the half box that is pointing away from the ray origin.
    float2 xyPlane = floor(currentMipPosition) + floorOffset;
    xyPlane = xyPlane * currentMipResolution_rcp + uvOffset;

    // o + d * t = p' => t = (p' - o) / d
    float2 t = (xyPlane - origin.xy) / (direction.xy + 0.0001);
    tCurrent = min(t.x, t.y);
    position = origin + tCurrent * direction;
}

bool AdvanceRay(float3 origin, float3 direction, float2 currentMipPosition, float2 currentMipResolution_rcp, float2 floorOffset, float2 uvOffset, float surfaceZ, inout float3 position, inout float tCurrent)
{
    // Create boundary planes
    float2 xyPlane = floor(currentMipPosition) + floorOffset;
    xyPlane = xyPlane * currentMipResolution_rcp + uvOffset;
    float3 boundaryPlanes = float3(xyPlane, surfaceZ);

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    float3 t = (boundaryPlanes - origin) / (direction + 0.0001);

    // Prevent using z plane when shooting out of the depth buffer.
    t.z = direction.z < 0 ? t.z : FLT_MAX;

    // Choose nearest intersection with a boundary.
    float tMin = min(min(t.x, t.y), t.z);

    // Larger z means closer to the camera.
    bool aboveSurface = surfaceZ > position.z;

    // Decide whether we are able to advance the ray until we hit the xy boundaries or if we had to clamp it at the surface.
    // We use the asint comparison to avoid NaN / Inf logic, also we actually care about bitwise equality here to see if t_min is the t.z we fed into the min3 above.
    bool skippedTile = abs(tMin - t.z) > 0.00001 && aboveSurface;

    // Make sure to only advance the ray if we're still above the surface.
    tCurrent = aboveSurface ? tMin : tCurrent;

    // Advance ray
    position = origin + tCurrent * direction;

    return skippedTile;
}

// Based on: https://github.com/GPUOpen-Effects/FidelityFX-SSSR/tree/master
// Requires origin and direction of the ray to be in screen space [0, 1] x [0, 1]
float3 HierarchicalRaymarch(float3 origin, float3 direction, float2 screenSize, sampler depthSampler, out bool validHit)
{
    int currentMip = HiZTraceMostDetailedLevel;
    float2 currentMipResolution = GetMipResolution(screenSize, currentMip);
    float2 currentMipResolution_rcp = rcp(currentMipResolution);
    float2 uvOffset = 0.005 * exp2(HiZTraceMostDetailedLevel) / screenSize;
    uvOffset = (direction.xy < 0) ? -uvOffset : uvOffset;
    float2 floorOffset = (direction.xy < 0) ? 0 : 1;

    float tCurrent;
    float3 position;
    InitialAdvanceRay(origin, direction, currentMipResolution, currentMipResolution_rcp, floorOffset, uvOffset, position, tCurrent);

    int i = 0;
    while (i < HiZTraceIterationsMax && currentMip >= HiZTraceMostDetailedLevel)
    {
        if (any(position.xy < 0.0) || any(position.xy > 1.0))
        {
            validHit = false;
            return position;
        }

        float2 currentMipPosition = currentMipResolution * position.xy;
        float surfaceZ = tex2Dlod(depthSampler, float4(position.xy, 0, currentMip)).r;

        bool skippedTile = AdvanceRay(origin, direction, currentMipPosition, currentMipResolution_rcp, floorOffset, uvOffset, surfaceZ, position, tCurrent);

        currentMip += skippedTile ? 1 : -1;
        currentMipResolution *= skippedTile ? 0.5 : 2;
        currentMipResolution_rcp *= skippedTile ? 2 : 0.5;

        i++;
    }

    validHit = (i <= HiZTraceIterationsMax);
    return position;
}


static const int rayMarchIterationsMax = 60; // primary ray march step count (higher will find more in distance, but slower)
static const float rayMarchStepIncrease = 1.05f; // primary ray march step increase (higher will travel more distance, but can miss details)
static const int rayMarchFineIterationsMax = 2; // binary step count (higher is nicer but slower)
static const float rayMarchTolerance = 0.000002; // early exit factor for binary search (smaller is nicer but slower)
static const float rayMarchLevelIncrement = 0.3; // level increment based on ray travel distance and roughness (higher values improves performance, but traces at lower resolution)

// samplePos where ray march left of

float3 BinarySearch(float3 samplePos, float3 V, float level, sampler depthSampler)
{
    for (int i = 0; i < rayMarchFineIterationsMax; i++)
    {
        float sampleDepth = tex2Dlod(depthSampler, float4(samplePos.xy, 0, level)).r;

        if (abs(samplePos.z - sampleDepth) < rayMarchTolerance)
        {
            return samplePos;
        }

        if (samplePos.z >= sampleDepth)
        {
            samplePos += V;
        }

        V *= 0.5f;
        samplePos -= V;
    }

    return samplePos;
}

float3 RayMarch(float3 P, float3 V, float roughness, float jitter, sampler depthSampler, out bool validHit)
{
    float3 samplePos = P + V * jitter;
    float sampleDepth = 0;
    float level = 1;

    int iterations = 0;
    while (iterations <= rayMarchIterationsMax)
    {
        if (any(samplePos.xy < 0.0) || any(samplePos.xy > 1.0))
        {
            validHit = false;
            return samplePos;
        }

        samplePos += V;
        float sampleDepth = tex2Dlod(depthSampler, float4(samplePos.xy, 0, level)).r;

        if (sampleDepth > samplePos.z)
        {
            samplePos = BinarySearch(samplePos, V, level, depthSampler);
            break;
        }

        V *= rayMarchStepIncrease;
        level += rayMarchLevelIncrement * roughness;

        iterations++;
    }

    validHit = (iterations <= rayMarchIterationsMax);
    return float3(samplePos.xy, sampleDepth);
}

float CalculateEdgeVignette(float2 hitPixel)
{
    float2 hitPixelNDC = hitPixel * 2.0 - 1.0;

    //float maxDimension = min(1.0, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
    //float attenuation = 1.0 - max(0.0, maxDimension - blendScreenEdgeFade) / (1.0 - blendScreenEdgeFade);

    float2 vignette = saturate(abs(hitPixelNDC) * blendScreenEdgeFade - (blendScreenEdgeFade - 1.0f));
    float attenuation = saturate(1.0 - dot(vignette, vignette));

    return attenuation;
}

float3 reconstruct_world_position(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

#ifdef SOURCE_ENGINE_FLIPPED
    clipPos.y = -clipPos.y;
#endif
    float4 worldPos = mul(clipPos, GetViewProjInv());
    return worldPos.xyz / worldPos.w;
}

float ValidateHit(float2 hit, float hitDepth, float rayDepth, float2 prevHitUV)
{
    float vignetteHit = CalculateEdgeVignette(hit.xy);
    float vignetteHitPrev = CalculateEdgeVignette(prevHitUV);
    float vignette = min(vignetteHit, vignetteHitPrev);

    float3 surfaceViewPosition = reconstruct_world_position(hit.xy, hitDepth);
    float3 hitViewPosition = reconstruct_world_position(hit.xy, rayDepth);

    float distance = length(surfaceViewPosition - hitViewPosition);
    float confidence = 1.0 - smoothstep(0.0, traceThickness, distance);

    return vignette * confidence;
}

//float3 ssrComposition(float depth, float roughness, float2 uv, float3 N, sampler texture_depth_sampler, sampler input_sampler, float4 worldPos)
//{
//    float3 output = (float3)0;
//
//    float3 P_world = reconstruct_world_position(uv, depth);
//    float3 camera_world = g_vecOrigin;
//    float3 V_world = normalize(camera_world - P_world);
//
//    float4 H;
//    float3 L_world;
//
//    if (roughness > 0.05f)
//    {
//        float3x3 tangentBasis = GetTangentBasis(N);
//        float3 tangentV = mul(tangentBasis, V_world);
//
//        const float2 bluenoise = blue_noise(uv).xy;
//        float2 Xi = bluenoise.xy;
//        Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);
//
//        H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
//        H.xyz = mul(H.xyz, tangentBasis);
//        L_world = reflect(-V_world, H.xyz);
//    }
//    else
//    {
//        H = float4(N.xyz, 1.0f);
//        L_world = reflect(-V_world, H.xyz);
//    }
//
//    float3 RayStart = P_world;
//    float3 RayDir = L_world;
//    float3 RayCurPos = RayStart;
//
//    // Ray step parameters
//    float RayStep = 0.1;
//    float g_flRayStepDiv = 0.5;
//    float g_flDepthDiffMax = 0.01;
//
//    float2 rayUV = uv;
//    bool validHit = false;
//    float3 rayStep = float3(uv, depth);
//
//    int maxIterations = 16;
//    [loop]
//        for (int i = 0; i < maxIterations; i++)
//        {
//            RayCurPos += RayDir * RayStep;
//
//            float4 clipPos = mul(float4(RayCurPos, 1.0), GetViewProj());
//            float3 ndcPos = clipPos.xyz / clipPos.w;
//            rayUV = ndcPos.xy * 0.5 + 0.5;
//
//#ifdef SOURCE_ENGINE_FLIPPED
//            rayUV.y = 1.0 - rayUV.y;
//#endif
//
//            float depth_current = ndcPos.z * 0.5 + 0.5;
//
//            // Check bounds
//            if (rayUV.x < 0.0 || rayUV.x > 1.0 || rayUV.y < 0.0 || rayUV.y > 1.0)
//                break;
//
//            float depth_compare = tex2Dlod(texture_depth_sampler, float4(rayUV, 0, 0)).r;
//
//            float depthDiff = abs(depth_compare - depth_current);
//            if (depthDiff < g_flDepthDiffMax && depth_current > depth_compare)
//            {
//                validHit = true;
//                rayStep = float3(rayUV, depth_compare);
//                break;
//            }
//
//            RayStep = length(RayCurPos - RayStart) * g_flRayStepDiv;
//        }
//
//    float3 rayStepScreen = rayStep;
//
//    float4 clipPos = mul(float4(RayCurPos, 1.0), GetViewProj());
//    float3 ndcPos = clipPos.xyz / clipPos.w;
//    float2 rayCurUV = ndcPos.xy * 0.5 + 0.5;
//
//    rayCurUV.y = 1.0 - rayCurUV.y;
//
//    float rayCurDepth = ndcPos.z * 0.5 + 0.5;
//
//    float3 rayCurScreen = float3(rayCurUV, rayCurDepth);
//    float3 rayDirectionScreen = rayCurScreen - rayStepScreen;
//
//    float2 screenSize = rcp(g_vecFullScreenTexel);
//    float2 hit = HierarchicalRaymarch(rayStepScreen, rayDirectionScreen, screenSize, texture_depth_sampler, validHit);
//
//    float2 prevHitUV = hit.xy;
//    float hitDepth = tex2D(texture_depth_sampler, hit.xy).r;
//    float hitRayDepth = rayStepScreen.z + length(hit.xy - rayStepScreen.xy) * rayDirectionScreen.z;
//    float confidence = validHit ? ValidateHit(hit.xy, hitDepth, hitRayDepth, prevHitUV) : 0;
//
//    float4 indirectSpecular;
//    indirectSpecular.rgb = confidence > 0 ? tex2D(input_sampler, prevHitUV).rgb : 0;
//    indirectSpecular.a = confidence;
//
//    output = float4(indirectSpecular.rgb, indirectSpecular.a);
//    return output;
//}

float3 ssrComposition(float depth, float roughness, float2 uv, float3 N, sampler texture_depth_sampler, sampler input_sampler, float4 worldPos)
{
    float3 output = (float3)0;

    float3 P_world = reconstruct_world_position(uv, depth);

    float3 camera_world = g_vecOrigin;
    float3 V_world = normalize(camera_world - P_world);

    float4 H;
    float3 L_world;

    if (roughness > 0.05f)
    {
        float3x3 tangentBasis = GetTangentBasis(N);
        float3 tangentV = mul(tangentBasis, V_world);

        const float2 bluenoise = blue_noise(uv).xy;
        float2 Xi = bluenoise.xy;
        Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

        H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
        H.xyz = mul(H.xyz, tangentBasis);
        L_world = reflect(-V_world, H.xyz);
    }
    else
    {
        H = float4(N.xyz, 1.0f);
        L_world = reflect(-V_world, H.xyz);
    }

    float4 rayStartClip = mul(float4(P_world, 1.0), GetViewProj());
    float4 rayEndClip = mul(float4(P_world + L_world, 1.0), GetViewProj());

    float3 rayStartNDC = rayStartClip.xyz * rcp(rayStartClip.w);
    float3 rayEndNDC = rayEndClip.xyz * rcp(rayEndClip.w);

    rayStartNDC.z = rayStartNDC.z * 0.5 + 0.5;
    rayEndNDC.z = rayEndNDC.z * 0.5 + 0.5;

    float3 rayStartScreen = float3(rayStartNDC.xy * 0.5 + 0.5, rayStartNDC.z);

    rayStartScreen.y = 1.0 - rayStartScreen.y;

    float3 rayEndScreen = float3(rayEndNDC.xy * 0.5 + 0.5, rayEndNDC.z);

    rayEndScreen.z = rayStartScreen.z;

    float3 rayDirectionScreen = rayEndScreen - rayStartScreen;

    float rayLength = length(rayDirectionScreen);
    rayDirectionScreen = normalize(rayDirectionScreen) * 0.01;

    float2 screenSize = rcp(g_vecFullScreenTexel);

    bool validHit = true;
    float3 hit = HierarchicalRaymarch(rayStartScreen, rayDirectionScreen, screenSize, texture_depth_sampler, validHit);

    float2 prevHitUV = hit.xy;
    float hitDepth = tex2D(texture_depth_sampler, hit.xy).r;
    float hitRayDepth = rayStartScreen.z + length(hit.xy - rayStartScreen.xy) * rayDirectionScreen.z;
    float confidence = validHit ? ValidateHit(hit.xy, hitDepth, hitRayDepth, prevHitUV) : 0;

    float4 indirectSpecular;
    indirectSpecular.rgb = confidence > 0 ? tex2D(input_sampler, prevHitUV).rgb : 0;
    indirectSpecular.a = confidence;

    output = float4(indirectSpecular.rgb, indirectSpecular.a);
    return output;
}

float GetDepthFromNormal(float3 worldNormal, float2 texCoord, float roughness)
{
    float2 gradient = 0;
    if (abs(worldNormal.z) > 0.001)
    {
        gradient = -worldNormal.xy / worldNormal.z;
    }

    float gradientMagnitude = length(gradient);
    float baseDepth = 1.0 - saturate(dot(worldNormal, float3(0, 0, 1)));
    float depth = baseDepth + gradientMagnitude * 0.1;

    return depth;
}

float3 WorldToView(in float3 position)
{
    float4 viewPos = mul(float4(position, 1.0f), g_View);
    return viewPos.xyz / viewPos.w;  // Add the perspective divide
}

float3 ClipToScreen(float3 UV)
{
    /* return float3(UV.x * 0.5f + 0.5f, 0.5f - UV.y * 0.5f, UV.z * 0.5f + 0.5f);*/
    float x = float(UV.x * 0.5f + 0.5f);
    float y = float(0.5f - UV.y * 0.5f);
    float z = float(UV.z * 0.5f + 0.5f);

    return float3(x, y, z);
}

float3 WorldToUV(in float3 position)
{
    float4 viewProj = mul(float4(position, 1.0f), GetViewProj());
    viewProj.xyz /= viewProj.w;
    float3 clipView = ClipToScreen(viewProj.xyz);
    //clipView.x = 1.0 -clipView.x;
    return clipView.xyz;
}



float3 readPos(in float3 coord)
{
    float4 ViewPos = mul(float4(coord.xyz, 1.0f), GetViewProjInv());
    ViewPos.xyz /= ViewPos.w;
    ViewPos.xyz = float3(ViewPos.xy, ViewPos.z);
    return float3(ViewPos.xyz);
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

float3 ScreenToClip(float3 screenUV)
{
    return float3(screenUV.x * 2.0f - 1.0f, 1.0f - screenUV.y * 2.0f, screenUV.z * 2.0f - 1.0f);
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

float3 ssrComposition_simple(float roughness, float metallic, float2 uv, float3 N, sampler input_sampler, sampler input_depth, float4 worldPos, float3 view)
{
    //float3 uvtoviewADD = float3(-tan(radians(FIELD_OF_VIEW * 0.5)).xx, 1.0) * ASPECT_RATIO;
    //float3 uvtoviewMUL = float3(-2.0 * uvtoviewADD.xy, 0.0);

    float2 screenUV = uv.xy;
    float3 WorldNormals = N;
    float4 vWorldPos = worldPos;

    float3 reflection = normalize(reflect(-view, WorldNormals));

    float sceneDepth = tex2Dlod(input_depth, float4(screenUV.xy, 0, 0)).r;

    float3 rayStartWorld = ScreenToWorld(float3(screenUV, sceneDepth));
    float3 rayStartView = WorldToView(rayStartWorld);

    float3 RayDir = normalize(reflection);
    float3 RayCurPos = rayStartView.xyz;

    //float2 currentUV = get_uv_from_position(RayCurPos, uvtoviewMUL, uvtoviewADD);

    float RayStep = (0.2 + 0.05) * sqrt(sceneDepth) * rcp(1e-3 + saturate(1 - dot(RayDir, -view)));

    float3 reflectionUV = float3(screenUV, sceneDepth);
    float3 reflectionWorldPos = (float3)0;
    float2 rayScreen = (float2)0;

    float depth_current = sceneDepth / 4000.0f;
    float depth_compare = readDepth_Normalized(WorldToUV(RayCurPos.xyz), input_depth) + 0.1f; // to ensure the first loop always starts

    float g_flRayStepDiv = 0.5;
    float g_flDepthDiffMax = 0.01;
    int maxIterations = 8;
    int refinementSteps = 4;
    float depthMask = 1.0f;

    int j = 0;
    while (j++ < maxIterations)
    {
        rayScreen = WorldToUV(RayCurPos.xyz);
        reflectionUV = float3(rayScreen.xy, sceneDepth);
        //reflectionUV.x = 1.0 - reflectionUV.x * float2(2.0, -2.0) + float2(-1.0, 1.0);
        //reflectionUV = get_uv_from_position(RayCurPos.xyz, uvtoviewMUL, uvtoviewADD);

        sceneDepth = tex2Dlod(input_depth, float4(reflectionUV.xyz, 0)).r;
        reflectionWorldPos = readPos(ScreenToClip(float3(reflectionUV)));

        for (int i = 0; i < refinementSteps; i++)
        {
            RayCurPos.xyz -= RayDir * RayStep;
            RayStep *= g_flRayStepDiv;

            depth_current = sceneDepth / 4000.0f;
            depth_compare = readDepth_Normalized(WorldToUV(RayCurPos.xyz), input_depth);

            float depthDiff = abs(depth_compare - depth_current);
            depthMask = step(depthDiff, g_flDepthDiffMax);

            /*if (RayDir.z >= depthDiff)
            {
                RayCurPos.xyz += RayDir * RayStep;
                RayStep *= g_flRayStepDiv;
            }*/
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

    float3 frame = tex2D(input_sampler, reflectionUV);

    return float3(frame * fresnelMask * UVmask * mask * depthMask);
}


//float3 ssrComposition_simple(float roughness, float metallic, float4 uv, float3 N, sampler input_sampler, sampler input_depth, float4 worldPos, float3 view)
//{
//    float2 screenUV = uv.xy;
//    float currentDepth = tex2Dlod(input_depth, float4(screenUV, 0, 0)).r;
//    float3 rayStart = float3(screenUV, currentDepth);
//
//    // 2. Calculate reflection vector
//    float3 reflection = normalize(reflect(-view, N));
//
//    // 3. Transform to clip space correctly
//    float4 clipPos = mul(float4(worldPos.xyz + reflection * 0.1, 1.0), GetViewProj());
//    clipPos.xyz /= clipPos.w;
//    float2 reflectUV = clipPos.xy * 0.5 + 0.5;
//
//#ifdef SOURCE_ENGINE_FLIPPED
//    reflectUV.y = 1.0 - reflectUV.y;
//    screenUV.y = 1.0 - screenUV.y;
//#endif
//
//    // 4. Ray direction in UV space
//    float2 rayDirUV = reflectUV - screenUV;
//    float rayLength = length(rayDirUV);
//
//    // 5. Better step size calculation (constant pixel stride)
//    float2 screenSize = 1.0 / g_vecFullScreenTexel;
//    float pixelStride = 4.0;
//    float2 stepUV = normalize(rayDirUV) * pixelStride / screenSize;
//
//    // 6. Ray march with linear depth testing
//    float3 rayPos = float3(screenUV, currentDepth);
//    bool hitFound = false;
//    float2 hitUV = screenUV;
//
//    int maxIterations = 64;
//    for (int i = 0; i < maxIterations; i++)
//    {
//        rayPos.xy += stepUV;
//
//
//        if (rayPos.x < 0 || rayPos.x > 1 || rayPos.y < 0 || rayPos.y > 1)
//            break;
//
//        float sceneDepth = tex2Dlod(input_depth, float4(rayPos.xy, 0, 0)).r;
//        if (rayPos.z >= sceneDepth)
//        {
//            hitFound = true;
//            hitUV = rayPos.xy;
//            break;
//        }
//    }
//
//    float UVmask = hitFound ? 1.0 : 0.0;
//    UVmask *= (hitUV.x < 0 || hitUV.x > 1 ? 0 : 1)
//        * (hitUV.y < 0 || hitUV.y > 1 ? 0 : 1);
//
//    hitUV = saturate(hitUV);
//    float2 reflectionClip = hitUV * 2.0f - 1.0f;
//    float mask = length(reflectionClip);
//    mask *= mask;
//    mask = 1.0f - mask;
//
//    float fresnelMask = dot(-view, N);
//    fresnelMask = saturate(fresnelMask);
//    fresnelMask = 1.0f - fresnelMask;
//    fresnelMask = lerp(fresnelMask, pow(fresnelMask, 5.0f), 1.0f - metallic);
//
//    float3 frame = tex2D(input_sampler, hitUV);
//
//    return float3(frame * fresnelMask * UVmask * mask);
//}
