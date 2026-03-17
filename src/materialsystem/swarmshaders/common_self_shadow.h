// ============================================================
// Percentage-Closer Soft Shadows (PCSS) Pixel Shader
// Converted from Microsoft HLSL Shader Compiler 10.1 assembly
// ps_3_0
// ============================================================

static const float4 k_misc = float4(1.0, 0.0, 0.5, 0.75);          // c2
static const float4 k_shadow = float4(0.25, 4.0, 0.00390625, 0.001953125); // c3
static const float4 k_depth = float4(0.999984741, 255.996094, 65535.0, 0.0); // c4
static const int    k_loopCount = 4;                                      // i0.x

float3 perform_self_shadowing(float3 L, float att, in float2 uv, float3 worldPos, float lightColor)

{
    float4 Constants0 = float4(L, att);
    float4 Constants1 = float4(5, 15, 2000, 1);

    float2 shadowMapUV = k_misc.z * worldPos.xy;   // r0.xy = c2.z * vPos
    float2 shadowFrac = frac(shadowMapUV.yx);            // r0.zw = frac(r0.yx) swizzled -> r0.xy used below

    float2 shadowMapInt = shadowMapUV - shadowFrac.xy;

    float  depthScaledY = shadowMapInt.y * k_misc.w;

    float2 r1_xz = k_misc.zz;   // both 0.5

    float depthCoarse = dot(shadowMapInt, float2(r1_xz.x, depthScaledY));

    float3 worldPosFrac = frac(float3(worldPos.x, worldPos.x, worldPos.y));

    float2 worldPosInt_xw = float2(worldPos.x, worldPos.y) - worldPosFrac.yz;
    float  depthScaledX = worldPosInt_xw.x * k_misc.w;

    float depthFine = dot(float2(worldPosInt_xw.x, worldPosInt_xw.y),
        float2(r1_xz.x, depthScaledX));
    float2 depthUV = frac(float2(depthCoarse, depthFine));

    float  receiverDepth = depthUV.x * k_shadow.x + depthUV.y;

    float4 lightSpaceUV = k_misc.xxyy * float4(uv.xyxx);

    float4 depthSample = tex2Dlod(depthsampler, float4(lightSpaceUV.xy, 0, 0));

    float4 depthRecip = rcp(depthSample);

    float loopCountF = Constants1.x;

    float3 depthAccum = Constants0.xyz * loopCountF + depthRecip.xyz;

    float3 depthDiff = depthRecip.xyz - depthAccum;
    float  depthDiffSqLen = dot(depthDiff, depthDiff);

    float  depthInvSqrt = rcp(sqrt(depthDiffSqLen));
    float  depthFrac2 = frac(depthInvSqrt);
    float  depthFiltered = depthInvSqrt - depthFrac2;

    float  penumbraSize = min(depthFiltered, k_shadow.y);
    float4 worldPosH = float4(depthAccum, 1.0);

    float4x4 ViewProj = GetViewProj();

    float  lightSpaceW = dot(worldPosH, ViewProj[3]);

    float  lightSpaceWInv = rcp(lightSpaceW);
    float  lightSpaceX = dot(worldPosH, ViewProj[0]);
    float  lightSpaceY = dot(worldPosH, ViewProj[1]);

    float2 shadowProjUV = float2(lightSpaceX, lightSpaceY) * lightSpaceWInv;
    shadowProjUV = shadowProjUV * k_misc.z + k_misc.z;
    shadowProjUV = shadowProjUV * k_misc.z + k_misc.z;

    float  penumbraRcp = rcp(penumbraSize);
    float2 shadowOffset = float2(0.0, 0.0);
    float  shadowAccum = 0.0;
    float  shadowCount = 0.0;

    [loop]
    for (int sampleIdx = 0; sampleIdx < k_loopCount; sampleIdx++)
    {
        if (penumbraSize < shadowCount)
        {
            break;  // break_ne c2.x, -c2.x  (break if 1 != -1, always true)
        }

        shadowCount += k_misc.x;

        float sampleScale = receiverDepth * shadowCount;

        sampleScale *= penumbraRcp;
        float2 sampleUV = lerp(shadowProjUV, uv, sampleScale);
        float2 sampleUVSat = saturate(sampleUV);

        float2 uvOutOfBounds = sampleUV - sampleUVSat;
        float  oobCheck = dot(uvOutOfBounds, k_misc.xx);
        if (oobCheck != -oobCheck)
        {
            break;  // break_ne c2.x, -c2.x
        }

        float4 shadowTap = tex2Dlod(depthsampler, float4(sampleUV, 0, 0));
        float  tapDepth = lerp(depthFine, depthInvSqrt, sampleScale);

        float  depthDelta = shadowTap.w - tapDepth;
        float  cmpA = (depthDelta >= 0.0) ? shadowCount : (shadowCount - k_misc.x);

        float  depthDelta2 = shadowTap.w - depthInvSqrt;

        float  shadowBias = Constants1.z - abs(depthDelta2);
        float  cmpB = (shadowBias >= 0.0) ? 0.0 : -k_misc.x;
        float  depthDelta3 = depthDelta - Constants1.y;

        cmpB = (depthDelta3 >= 0.0) ? 0.0 : cmpB;

        float  shadowCountPP = shadowAccum + k_misc.x;
        float  cmpC = (cmpB >= 0.0) ? shadowAccum : shadowCountPP;

        shadowAccum = (depthDelta <= 0.0) ? shadowAccum : cmpC;
        shadowCount = cmpA + k_misc.x;
    }
    float shadowFactor = saturate(penumbraRcp * shadowAccum);

    float shadowTerm = shadowFactor * (-Constants0.w) + 1.0;

    shadowTerm = log2(shadowTerm);
    shadowTerm *= Constants1.w;

    float finalShadow = exp2(shadowTerm);

    float3 fogColor = depthInvSqrt * k_depth.xyz;
    fogColor = frac(fogColor);

    fogColor.xy = fogColor.xy - fogColor.yz * k_shadow.z;
    float2 fogUV = fogColor.xy + k_shadow.w;
    float3 color = lerp(finalShadow, lightColor, k_misc.x);

    return color;
}
