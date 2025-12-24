
const float cloudNarrowness = 0.57;
const float cloudStretch = 4.2;
const float cloudTallness = 12.4;

float2 GetRoundedCloudCoord(float2 pos, float cloudRoundness) {
    float2 coord = pos.xy + 0.5;
    float2 signCoord = sign(coord);
    coord = abs(coord) + 1.0;
    float2 i = floor(coord);
    float2 f = frac(coord);
    f = smoothstep(0.5 - cloudRoundness, 0.5 + cloudRoundness, f);
    coord = i + f;
    return (coord - 0.5) * signCoord / 256.0;
}

float2 GetSmoothCloudCoord(float2 pos) {
    return pos / 256.0;
}


float3 ModifyTracePos(float3 tracePos, float cloudAltitude) {
    float wind = g_Time; // Or use g_parameters.w if that's your time
    tracePos.x += wind;
    tracePos.z += cloudAltitude * 64.0;
    tracePos.xz *= cloudNarrowness;
    return tracePos;
}

bool GetCloudNoise(float3 tracePos, inout float3 tracePosM, float cloudAltitude, sampler2D noiseTex, int layerID) {
    tracePosM = ModifyTracePos(tracePos, cloudAltitude);

    // Sample at different vertical offsets to create 3D effect
    //float2 coord1 = GetRoundedCloudCoord(tracePosM.xz, 1.225);
    //float2 coord2 = GetRoundedCloudCoord(tracePosM.xz + tracePosM.y * 0.5, 0.125); 
    //float2 coord3 = GetRoundedCloudCoord(tracePosM.xz + tracePosM.y * 0.6, 0.125);
    ////float2 coord1 = float3(tracePosM.xz, 0.125);
    ////float2 coord2 = float3(tracePosM.xz + tracePosM.y * 0.1, 0.125);
    ////float2 coord3 = float3(tracePosM.xz + tracePosM.y * 0.2, 0.125);

    //float noise1 = tex2D(noiseTex, coord1).r;
    //float noise2 = tex2D(noiseTex, coord2).g;
    //float noise3 = tex2D(noiseTex, coord3).b;

    float4 noiseWeights = (layerID == 0) ? g_CloudCoverage2 : g_CloudCoverage;

    float2 baseCoord = tracePosM.xz / 256.0;

    float2 coord1 = baseCoord;
    float2 coord2 = baseCoord * 2.0 + tracePosM.y * 0.09;
    float2 coord3 = baseCoord * 4.0 + tracePosM.y * 0.08;
    float2 coord4 = baseCoord * 0.5 - tracePosM.y * 0.13;

    // Sample noise at different scales
    float noise1 = tex2D(noiseTex, coord1).r; // Base shape
    float noise2 = tex2D(noiseTex, coord2).g; // Medium detail
    float noise3 = tex2D(noiseTex, coord3).b; // Fine detail
    float noise4 = tex2D(noiseTex, coord4).r; // Large puffy shapes

    float adjustedNoise1 = noise1 * noiseWeights.x; // Was 0.7
    float adjustedNoise2 = noise2 * noiseWeights.y; // Was 0.3
    float adjustedNoise3 = noise3 * noiseWeights.z; // Was 0.1
    float adjustedNoise4 = noise4 * noiseWeights.w; // Was 0.666

    float density = adjustedNoise1 + adjustedNoise2 + adjustedNoise3 + adjustedNoise4;

    float threshold = clamp(abs(cloudAltitude - tracePos.y) / cloudStretch, 0.001, 0.999);
    threshold = pow(threshold, 8.0);

    float coverageThreshold = threshold * 0.7 + 0.8;
    coverageThreshold = lerp(coverageThreshold + 0.3, coverageThreshold - 0.3, g_CloudParams.x);

    return density > coverageThreshold;
}
float4 GetVolumetricClouds(
    float cloudAltitude,
    float distanceThreshold,
    inout float cloudLinearDepth,
    float skyFade,
    float skyMult0,
    float3 cameraPos,
    float3 nPlayerPos,
    float lViewPosM,
    float VdotS,
    float VdotU,
    float dither,
    float3 skyColor,
    sampler2D noiseTex,
    int layerID
) {
    float4 volumetricClouds = float4(0.0, 0.0, 0.0, 0.0);

    float higherPlaneAltitude = cloudAltitude + cloudStretch;
    float lowerPlaneAltitude = cloudAltitude - cloudStretch;

    float lowerPlaneDistance = (lowerPlaneAltitude - cameraPos.y) / nPlayerPos.y;
    float higherPlaneDistance = (higherPlaneAltitude - cameraPos.y) / nPlayerPos.y;

    float minPlaneDistance = min(lowerPlaneDistance, higherPlaneDistance);
    minPlaneDistance = max(minPlaneDistance, 0.0);
    float maxPlaneDistance = max(lowerPlaneDistance, higherPlaneDistance);

    if (maxPlaneDistance < 0.0) return float4(0.0, 0.0, 0.0, 0.0);

    float planeDistanceDif = maxPlaneDistance - minPlaneDistance;

    int sampleCount = max(int(planeDistanceDif) / 16, 6);
    sampleCount = min(sampleCount, 30);

    float distanceToEntry = length((cameraPos + minPlaneDistance * nPlayerPos).xz - cameraPos.xz);
    float distanceFactor = distanceToEntry / distanceThreshold;
    sampleCount = (int)(sampleCount * (1.0 - distanceFactor * 0.6));

    sampleCount = (int)(sampleCount * g_CloudQualityParams.x);
    sampleCount = max(sampleCount, 3); // Minimum samples
    sampleCount = min(sampleCount, 60); // Maximum samples (increased for high quality)

    float stepMult = planeDistanceDif / float(sampleCount);
    float3 traceAdd = nPlayerPos * stepMult;
    float3 tracePos = cameraPos + minPlaneDistance * nPlayerPos;
    tracePos += traceAdd * dither;
    //tracePos.y -= traceAdd.y;

    [unroll(8)]
    for (int i = 0; i < sampleCount; i++) {
        tracePos += traceAdd;

        float3 cloudPlayerPos = tracePos - cameraPos;
        float lTracePos = length(cloudPlayerPos);
        float lTracePosXZ = length(cloudPlayerPos.xz);

        if (lTracePosXZ > distanceThreshold) break;

        float3 tracePosM;
        // Pass the noise texture to the function
        if (GetCloudNoise(tracePos, tracePosM, cloudAltitude, noiseTex, layerID)) {
            // Calculate shading
            float cloudTallness = cloudStretch * 2.0;
            float cloudShading = 1.0 - (higherPlaneAltitude - tracePos.y) / cloudTallness;
            cloudShading = pow(max(cloudShading, 0.0), 0.8);

            // Sun influence
            float VdotSM1 = max(VdotS, 0.0);

            // Lighting
            float3 ambientColor = skyColor * 0.5;
            float3 directColor = g_SunColor.rgb;
            float3 colorSample = ambientColor * (1.0 - 0.35 * cloudShading)
                + directColor * (0.1 + cloudShading);

            // Distance fog
            float distanceRatio = (distanceThreshold - lTracePosXZ) / distanceThreshold;
            float cloudFogFactor = pow(saturate(distanceRatio), 2.0) * 0.75;
            colorSample = lerp(skyColor, colorSample, cloudFogFactor);

            // Alpha
            float cloudDistanceFactor = clamp(distanceRatio, 0.0, 0.75);
            volumetricClouds.a = sqrt(cloudDistanceFactor * 1.33333);
            volumetricClouds.rgb = colorSample;

            cloudLinearDepth = lTracePos;
            break;
        }
    }

    return volumetricClouds;
}

//float4 GetVolumetricClouds(
//        float cloudAltitude,
//        float distanceThreshold,
//        inout float cloudLinearDepth,
//        float skyFade,
//        float skyMult0,
//        float3 cameraPos,
//        float3 nPlayerPos,
//        float lViewPosM,
//        float VdotS,
//        float VdotU,
//        float dither,
//        float3 skyColor,
//        sampler2D noiseTex,
//        int layerID
//    )
//    {
//    float4 volumetricClouds = (float4) 0;
//
//    float higherPlaneAltitude = cloudAltitude + cloudStretch;
//    float lowerPlaneAltitude = cloudAltitude - cloudStretch;
//
//    float lowerPlaneDistance = (lowerPlaneAltitude - cameraPos.y) / nPlayerPos.y;
//    float higherPlaneDistance = (higherPlaneAltitude - cameraPos.y) / nPlayerPos.y;
//    float minPlaneDistance = min(lowerPlaneDistance, higherPlaneDistance);
//    minPlaneDistance = max(minPlaneDistance, 0.0);
//    float maxPlaneDistance = max(lowerPlaneDistance, higherPlaneDistance);
//    if (maxPlaneDistance < 0.0) return (float4) 0;
//    float planeDistanceDif = maxPlaneDistance - minPlaneDistance;
//
//#if CLOUD_QUALITY == 1 || !defined DEFERRED1
//    int sampleCount = max(int(planeDistanceDif) / 16, 6);
//#elif CLOUD_QUALITY == 2
//    int sampleCount = max(int(planeDistanceDif) / 8, 12);
//#elif CLOUD_QUALITY == 3
//    int sampleCount = max(int(planeDistanceDif), 12);
//#endif
//
//    float stepMult = planeDistanceDif / sampleCount;
//    float3 traceAdd = nPlayerPos * stepMult;
//    float3 tracePos = cameraPos + minPlaneDistance * nPlayerPos;
//    tracePos += traceAdd * dither;
//    tracePos.y -= traceAdd.y;
//
//#ifdef FIX_AMD_REFLECTION_CRASH
//    sampleCount = min(sampleCount, 30); //BFARC
//#endif
//    [unroll(8)]
//    for (int i = 0; i < sampleCount; i++) {
//        tracePos += traceAdd;
//
//        float3 cloudPlayerPos = tracePos - cameraPos;
//        float lTracePos = length(cloudPlayerPos);
//        float lTracePosXZ = length(cloudPlayerPos.xz);
//        float cloudMult = 1.0;
//        if (lTracePosXZ > distanceThreshold) break;
//        if (lTracePos > lViewPosM) {
//            if (skyFade < 0.7) continue;
//            else cloudMult = skyMult0;
//        }
//
//        float3 tracePosM;
//        if (GetCloudNoise(tracePos, tracePosM, cloudAltitude, noiseTex, layerID)) {
//            float lightMult = 1.0;
//
//#if SHADOW_QUALITY > -1
//            float shadowLength = shadowDistance * 0.9166667; //consistent08JJ622
//            if (shadowLength > lTracePos)
//                if (GetShadowOnCloud(tracePos, cameraPos, cloudAltitude, lowerPlaneAltitude, higherPlaneAltitude)) {
//#ifdef CLOUD_CLOSED_AREA_CHECK
//                    if (eyeBrightness.y != 240) continue;
//                    else
//#endif
//                        lightMult = 0.25;
//                }
//#endif
//
//            float cloudShading = 1.0 - (higherPlaneAltitude - tracePos.y) / cloudTallness;
//            cloudShading = pow(max(cloudShading, 0.0), 0.8);
//            //float VdotSM1 = max(sunVisibility > 0.5 ? VdotS : -VdotS, 0.0);
//
//#if CLOUD_QUALITY >= 2
//#ifdef DEFERRED1
//            float cloudShadingM = 1.0 - pow2(cloudShading);
//#else
//            float cloudShadingM = 1.0 - cloudShading;
//#endif
//
//            float gradientNoise = InterleavedGradientNoiseForClouds();
//
//            float3 cLightPos = tracePosM;
//            float3 cLightPosAdd = normalize(ViewToPlayer(lightfloat * 1000000000.0)) * float3(0.08);
//            cLightPosAdd *= shadowTime;
//
//            float light = 2.0;
//            cLightPos += (1.0 + gradientNoise) * cLightPosAdd;
//#ifdef DEFERRED1
//            light -= texture2D(colortex3, GetRoundedCloudCoord(cLightPos.xz, 0.125)).b * cloudShadingM;
//#else
//            light -= texture2D(gaux4, GetRoundedCloudCoord(cLightPos.xz, 0.125)).b * cloudShadingM;
//#endif
//            cLightPos += gradientNoise * cLightPosAdd;
//#ifdef DEFERRED1
//            light -= texture2D(colortex3, GetRoundedCloudCoord(cLightPos.xz, 0.125)).b * cloudShadingM;
//#else
//            light -= texture2D(gaux4, GetRoundedCloudCoord(cLightPos.xz, 0.125)).b * cloudShadingM;
//#endif
//
//            float VdotSM2 = VdotSM1 * shadowTime * 0.25;
//            VdotSM2 += 0.5 * cloudShading + 0.08;
//            cloudShading = VdotSM2 * light * lightMult;
//#endif
//
//            float3 colorSample = skyColor * 0.95 * (1.0 - 0.35 * cloudShading) + g_SunColor * (0.1 + cloudShading);
//            float3 cloudSkyColor = skyColor.rgb;
//#ifdef ATM_COLOR_MULTS
//            cloudSkyColor *= sqrtAtmColorMult; // C72380KD - Reduced atmColorMult impact on some things
//#endif
//            float distanceRatio = (distanceThreshold - lTracePosXZ) / distanceThreshold;
//            float cloudFogFactor = pow(saturate(distanceRatio), 2.0) * 0.75;
//            //float cloudFogFactor = pow2(clamp(distanceRatio, 0.0, 1.0)) * 0.75;
//            float skyMult1 = 1.0 - 0.2 * (1.0 - skyFade);
//            float skyMult2 = 1.0 - 0.33333 * skyFade;
//            colorSample = lerp(cloudSkyColor, colorSample * skyMult1, cloudFogFactor * skyMult2);
//            //colorSample *= pow(1.0 - maxBlindnessDarkness);
//
//            float cloudDistanceFactor = clamp(distanceRatio, 0.0, 0.75);
//            //float distanceRatioNew = (2000 - lTracePosXZ) / 2000;
//            //float cloudDistanceFactorNew = clamp(distanceRatioNew, 0.5, 0.75);
//
//            //volumetricClouds.a = pow(cloudDistanceFactor * 1.33333, 0.5 + 10.0 * pow(abs(VdotSM1), 90.0)) * cloudMult;
//            volumetricClouds.a = sqrt(cloudDistanceFactor * 1.33333) * cloudMult;
//            volumetricClouds.rgb = colorSample;
//
//            cloudLinearDepth = lTracePos;
//            break;
//        }
//    }
//
//    return volumetricClouds;
//}