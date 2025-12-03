
#include "tonemap_postprocessing.h"

static const float EPSILON = 0.00001;

sampler sMixedSampler[FREE_LIGHT_SAMPLERS] : register(FIRST_LIGHT_SAMPLER_FXC);
const float4 g_flMixedData[112] : register(FIRST_SHARED_LIGHTDATA_CONSTANT_FXC);

const float3 g_vecViewOrigin : register(c0);
const float4 g_vecShadowMappingTweaks_0 : register(c1);
const float4 g_vecShadowMappingTweaks_1 : register(c2);
float g_MetallicRoughnessFactor : register(c10); // multipler of roughness * metallic.
float g_GrazingFactor : register(c11); // multiplier of grazing effect
float g_GrazingPower : register(c12); //Power of grazing effect
float g_SpecularBoost : register(c13);
float g_SpecSizeScale : register(c14);
float3 g_flGIIntensity : register(c17);
float g_flEnableGI : register(c18);
float g_RoughnessScale : register(c19);
float g_shapeBias : register(c20);
float g_DiffuseScale : register(c25);
float g_FresnelPower : register(c22);
float g_SheenStrength : register(c23);
float g_SpecularBrightness : register(c24);
float g_green : register(c27);
float g_green2 : register(c30);
float g_blue : register(c28);

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0f.xxx - F0, F0) - F0) * pow(1.0f - cosTheta, 111.0f) * roughness;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f.xxx - F0) * pow(1.0f - cosTheta, 111.0f);
}

float DistributionGGX(float3 N, float3 H, float distL, float roughness)
{
    float alphaPrime = saturate(16.0f / (distL * 2.0) + roughness);
    float a = roughness * alphaPrime;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

half Distribution_GGX_Disney(half hdotN, half alphaG)
{
    float a2 = alphaG * alphaG;
    float tmp = (hdotN * hdotN) * (a2 - 1.0) + 1.0;
    //tmp *= tmp;

    return (a2 / (PI * tmp));
}

float ComputeSpotlightAttenuation(int index, float3 N, float3 vecWorldToLight, float dist, bool bDoSpotAttn)
{
    float3 L = vecWorldToLight / dist;
    float distNorm = dist / g_flMixedData[index].w;
    float fade = saturate(1.0f - distNorm);
    fade = pow(fade, g_flMixedData[index + 1].w);

    if (bDoSpotAttn)
    {
        float coneInner = g_flMixedData[index + 2].w;
        float coneOuter = max(g_flMixedData[index + 3].w, coneInner + 0.05f);
        coneOuter = max(coneOuter, 0.1f);
        float spotDot = dot(g_flMixedData[index + 3].xyz, L);
        float spotAtten = smoothstep(coneInner + 0.01f, coneOuter - 0.01f, spotDot);
        fade *= spotAtten;
    }

    float ndotl = saturate(dot(N, L));
    return fade * ndotl;
}

float CharlieDistribution(float NdotH, float alpha)
{
    float sinThetaH = sqrt(1.0 - NdotH * NdotH);
    float alphaInv = 1.0 / max(alpha, 0.001);
    float exponent = alphaInv;
    float numer = (2.0 + exponent) * pow(sinThetaH, exponent);
    return numer / (2.0 * PI);
}

float3 FresnelSheen(float NV, float3 tint, float strength)
{
    float grazing = pow(1.0 - NV, 5.0);
    return tint * strength * grazing;
}

float3 SheenBRDF_DreamWorks(float3 N, float3 V, float3 L, float3 tint, float strength, float roughness)
{
    const float kMinSheenR = 0.05;
    float sheenRough = max(saturate(roughness), kMinSheenR);

    float3 H = normalize(V + L);
    float NdotH = saturate(dot(N, H));
    float NV = saturate(dot(N, V));

    float D = CharlieDistribution(NdotH, sheenRough);

    float grazing = pow(1.0 - NV, 5.0);
    float3 F = tint * strength * grazing;

    return D * F;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float num = NdotV;
    float denom = NdotV * (1.0 - roughness) + roughness;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float r = roughness + 1.0f;
    r = (r * r) / 8.0f;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, r);
    float ggx1 = GeometrySchlickGGX(NdotL, r);

    return ggx1 * ggx2;
}

//float RadicalInverse_VdC(int i)
//{
//    float invBase = 0.5;
//    float result = 0.0;
//    float fraction = 1.0;
//
//    int n = i;
//    while (n > 0)
//    {
//        fraction *= invBase;
//        result += fraction * (n % 2);
//        n = n / 2;
//    }
//    return result;
//}
//
//float2 Hammersley(int i, int N)
//{
//    return float2((float)i / (float)N, RadicalInverse_VdC(i));
//}
//
//
//float3 HemisphereSample(float2 Xi, float3 N)
//{
//    float phi = 2.0f * 3.14159265f * Xi.x;
//    float cosTheta = sqrt(1.0f - Xi.y);
//    float sinTheta = sqrt(Xi.y);
//
//    float3 H;
//    H.x = cos(phi) * sinTheta;
//    H.y = sin(phi) * sinTheta;
//    H.z = cosTheta;
//
//    float3 up = abs(N.z) < 0.999f ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
//    float3 tangent = normalize(cross(up, N));
//    float3 bitangent = cross(N, tangent);
//
//
//    return tangent * H.x + bitangent * H.y + N * H.z;
//}
//
//float3 DoAmbient(int index, float2 UV, float3 vWorldPos, float3 vWorldNormal, float3 vEye, in float roughness, in float3 albedo, in float3 Ambient, in float3 Ground, bool bDoSpotAttn)
//{
//    float3 L = normalize(g_flMixedData[index].xyz - vWorldPos);
//    float3 V = normalize(vEye - vWorldPos);
//    float NV = max(0.0, dot(vWorldNormal, V) * 0.5f + 0.5f);
//    float NU = max(0.0, dot(vWorldNormal, float3(0.0f, 0.0f, 1.0f)) * 0.5f + 0.5f);
//
//    float diffuseTransition = NU;
//    float3 diffuse = lerp(Ground, Ambient, diffuseTransition);
//
//    HALF3 reflectVect = 2.0 * NV * vWorldNormal - V;
//    float3 reflection = 0.0f.xxx;
//
//    const int SAMPLE_COUNT = 2u;
//    for (int i = 0; i < SAMPLE_COUNT; i++)
//    {
//        float2 Xi = Hammersley(i, SAMPLE_COUNT);
//        float3 sampleDir = HemisphereSample(Xi, normalize(reflectVect));
//
//        reflection += SampleAmbientReflection(
//            float4(sampleDir, roughness * 4.0f),
//            Ground,
//            Ambient
//        ).rgb / SAMPLE_COUNT;
//    }
//
//    float3 vecDelta = vWorldPos - g_flMixedData[index].xyz;
//    float lightToWorldDist = length(vecDelta);
//
//    float radius = g_flMixedData[index].w;
//    float fade = 1.0f - saturate(lightToWorldDist / radius);
//
//#if DEFCFG_CHEAP_LIGHTS
//    fade *= fade;
//#else
//    fade = pow(fade, g_flMixedData[index + 1].w);
//#endif
//
//    if (bDoSpotAttn)
//    {
//        float spotDot = dot(g_flMixedData[index + 3].xyz, L);
//        fade *= smoothstep(g_flMixedData[index + 2].w, g_flMixedData[index + 3].w, spotDot);
//    }
//
//    fade = saturate(fade);
//
//    return albedo.rgb * lerp(reflection, diffuse, roughness) * fade;
//}

float ndfGGX(float cosNH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (cosNH * cosNH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

float GaSchlickGGXRemapped(float cosLi, float cosLo, float roughness)
{
    // k is alpha/2, to better fit the Smith model for GGX
    // Roughness is also remapped using (roughness + 1)/2 before squaring
    //
    // Substituting the remapping, you get:
    //  alpha = ((roughness+1)/2)^2 = (roughness+1)*(roughness+1)/4
    //  k = alpha/2 = (roughness+1)*(roughness+1)/8

    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Sebastien Lagarde proposes an empirical approach to derive the specular occlusion term from the diffuse occlusion term in [Lagarde14].
// The result does not have any physical basis but produces visually pleasant results.
// See Sebastien Lagarde and Charles de Rousiers. 2014. Moving Frostbite to PBR.
float ComputeSpecularAO(float vDotN, float ao, float roughness)
{
    return clamp(pow(vDotN + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

// Visibility term G( l, v, h )
// Very similar to Marmoset Toolbag 2 and gives almost the same results as Smith GGX
float Visibility_Schlick(half vdotN, half ldotN, float alpha)
{
    float k = alpha * 0.5;

    float schlickL = (ldotN * (1.0 - k) + k);
    float schlickV = (vdotN * (1.0 - k) + k);

    return (0.25 / (schlickL * schlickV));
    //return ( ( schlickL * schlickV ) / ( 4.0 * vdotN * ldotN ) );
}

// see s2013_pbs_rad_notes.pdf
// Crafting a Next-Gen Material Pipeline for The Order: 1886
// this visibility function also provides some sort of back lighting
float Visibility_SmithGGX(half vdotN, half ldotN, float alpha)
{
    // alpha is already roughness^2

    float V1 = ldotN + sqrt(alpha + (1.0 - alpha) * ldotN * ldotN);
    float V2 = vdotN + sqrt(alpha + (1.0 - alpha) * vdotN * vdotN);

    // RB: avoid too bright spots
    return (1.0 / max(V1 * V2, 0.15));
}

void calculateLight(float3 lightIn, float3 lightIntensity, float3 lightOut, float3 normal, float3 fresnelReflectance, int index, float3 vWorldPos, float3 vEye, float roughness, float metalness, float lightDirectionAngle, float3 albedo, out float3 Diffuse, out float3 Specular)
{
    float3 L = normalize(lightIn);
    float3 V = normalize(lightOut);
    float3 N = normalize(normal);

    float3 HalfAngle = normalize(L + V);
    float3 H = (dot(HalfAngle, HalfAngle) > 0.0f) ? HalfAngle : N;

    float cosLightIn = max(0.0f, dot(N, L));
    float cosHalfAngle = max(0.0f, dot(N, H));

    float cosDirectAngle = max(0.0f, dot(L, H));

    float HV = max(0.0f, dot(H, V));
    float HL = max(0.0f, dot(H, L));
    float NdotV = max(0.0f, dot(normal, V));
    float NV = max(0.0f, dot(N, V));
    float LN = cosLightIn;
    float VoH = max(0.0f, dot(V, H));
    float VdotH = max(0.0f, dot(V, H));
    float NdotL = max(0.0f, dot(N, L));
    float vDotN = max(0.0f, dot(V, N));


    //corrected fresnel with correct values.
    //old implentation caused dark burning spots on any material.
    float3 F = fresnelSchlickRoughness(fresnelReflectance, vDotN, roughness); // was HL //GREAT effects were with cosHalfAngle, caused normal bug. // Specular reflection
    float3 F2 = fresnelSchlickRoughness(fresnelReflectance, NdotV, roughness); // View-dependent term
    float3 F3 = fresnelSchlickRoughness(fresnelReflectance, NdotL, roughness); // was HL //GREAT effects were with cosHalfAngle, caused normal bug. // Light-dependent term

    float alpha = roughness * roughness;

    float D = ndfGGX(cosHalfAngle, roughness);
    // use Sam Pavloc's function.
    float G = Visibility_SmithGGX(NdotV, NdotL, alpha);
    // add specular occlusion for self shadowing.
    //float specAO = ComputeSpecularAO(NdotV, ao, roughness);
    // Calculate geometric attenuation for specular BRDF
    //float G = GaSchlickGGXRemapped(cosLightIn, NdotV, roughness);
    // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium
    // Metals on the other hand either reflect or absorb energ so diffuse contribution is always, zero
    // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness
#if SPECULAR
    // Metalness is not used if F0 map is available
    float3 kd = float3(1, 1, 1) - F;
#else
    //float3 kdF2 = float3(1, 1, 1) - F2;
    float3 kd = (float3(1, 1, 1) - F) * rcp(max(float3(0.1, 0.1, 0.1), float3(1, 1, 1) - F2));
#endif

    // composite all of our fresnel, account for size distortion of lights.
    // important that metalness is used here
    float3 Fc = lerp(F, F2 * F3, saturate(roughness * (1.0 - metalness)));

    //compute ambient here once instead of per loop.
    //float3 ambient = g_flMixedData[index + 2].xyz;

    //float groundIntensity = dot(ambient, float3(0.2126, 0.7152, 0.0722));
    //groundIntensity = saturate(groundIntensity);

    //float3 groundColor = albedo * groundIntensity;

    //F2 is stable allows for non black spots of specular
    float3 diffuseBRDF = Diffuse_OrenNayar(F, roughness, NV, LN, VoH) * g_DiffuseScale;
    float3 sheenBRDF = SheenBRDF_DreamWorks(N, V, L, albedo, g_SheenStrength, roughness);

    float3 specularBRDF = (Fc * D * G) / max(EPSILON, 4.0f);
    //specularBRDF *= specAO;
    //float3 CompositeAmbient = Ambient;/*DoAmbient( UV, vWorldPos, normal, vEye, roughness, albedo, ambient, groundColor);*/

    //composite everything
    //float3 finalColor = (diffuseBRDF + specularBRDF * g_SpecularBoost + sheenBRDF + CompositeAmbient) * lightIntensity * LN;

    Diffuse = (diffuseBRDF + sheenBRDF) * lightIntensity * LN;
    Specular = specularBRDF * g_SpecularBoost * lightIntensity;

#if LIGHTMAPPED && !FLASHLIGHT
    return specularBRDF * lightIntensity * LN;
#else

    //return the computed pbr light with tone mapping and gamma correction
    //return finalColor;

    //old method
    //return (diffuseBRDF + specularBRDF * g_SpecularBoost + sheenBRDF) * lightIntensity * LN;
#endif
}


void DoSpotLightPBR(const int index, float3 normal, float3 lightPos, float3 lightColor, float lightToWorldDist, float3 worldPos, float metalScalar, float roughness, float3 albedo, out float3 Diffuse, out float3 Specular, const bool bDoSpotAttn)
{
    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(g_vecViewOrigin.xyz - worldPos);
    float NdotL = max(0.0f, dot(normal, L));
    float NdotV = max(0.0f, dot(normal, V));

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalScalar);

    calculateLight(L, lightColor, V, normal, F0, index, worldPos, g_vecViewOrigin, roughness, metalScalar, NdotV, albedo, Diffuse, Specular);

    float radius = g_flMixedData[index].w;
    float fade = 1.0f - saturate(lightToWorldDist / radius);

#if DEFCFG_CHEAP_LIGHTS
    fade *= fade;
#else
    fade = pow(fade, g_flMixedData[index + 1].w);
#endif

    float3 attenuation = ComputeSpotlightAttenuation(index, normal, lightPos - worldPos, lightToWorldDist, bDoSpotAttn);

    /*if (bDoSpotAttn)
    {
        float spotDot = dot(g_flMixedData[index + 3].xyz, L);
        fade *= smoothstep(g_flMixedData[index + 2].w, g_flMixedData[index + 3].w, spotDot);
    }*/

    fade = saturate(fade);

    Diffuse *= attenuation * fade;
    Specular *= attenuation * fade;

   /* return directLighting * attenuation * fade;*/
}

//void LightpassParams(int litIdx, out float3 vecDelta, float3 worldPos, out float3 lightColor, out float dist)
//{
//    vecDelta = worldPos - g_flMixedData[litIdx].xyz;
//    lightColor = g_flMixedData[litIdx + 1].xyz;
//    dist = length(vecDelta);
//}

//float3 PerformShadow(float3 depthPos, int samp, int litIdx, float rad, float3 vecDelta, float dist, float3 worldNormal)
//{
//    float atten = ComputeSpotlightAttenuation(litIdx, worldNormal, -vecDelta, dist, true);
//    float shadow = PerformProjectedShadow(sMixedSampler[samp], depthPos, g_vecShadowMappingTweaks_0, g_vecShadowMappingTweaks_1, g_flMixedData[litIdx + 4].x);
//    return atten * shadow;
//}

float3 PerformShadow(float4 depthPos, int samp, int litIdx, float rad, float3 vecDelta, float dist, float3 worldNormal)
{
    //float atten = ComputeSpotlightAttenuation(litIdx, worldNormal, -vecDelta, dist, true);
    float shadow = PerformProjectedShadow(sMixedSampler[samp], depthPos, g_vecShadowMappingTweaks_0, g_vecShadowMappingTweaks_1, g_flMixedData[litIdx + 4].x);
    return shadow;
}

void DoPBRCSM(int litIdx, float3 worldPos, float3 worldNormal, float3 vecDelta, int samp, float3 albedo, float3 lightColor, float dist, float metallic, float roughness, out float3 Diffuse, out float3 Specular)
{
    float4x4 w2t = float4x4(
        g_flMixedData[litIdx + 5], g_flMixedData[litIdx + 6],
        g_flMixedData[litIdx + 7], g_flMixedData[litIdx + 8]
    );

    float viewDot = 1.0f - abs(dot(vecDelta / dist, worldNormal));
    float3 localPos = worldPos + worldNormal * (viewDot + dist * 0.0075f);
    float4 depthPos = mul(float4(localPos, 1.0f), w2t);
    depthPos.xyz /= depthPos.w;

    float3 flShadow = PerformShadow(depthPos, samp, litIdx, g_flMixedData[litIdx + 4].w, vecDelta, dist, worldNormal);

    DoSpotLightPBR(litIdx, worldNormal, g_flMixedData[litIdx].xyz, lightColor,
        dist, worldPos, metallic, roughness, albedo, Diffuse, Specular, true);

    /*return pbr * flShadow;*/

    Diffuse *= flShadow;
    Specular *= flShadow;
}


void DoPBR(int litIdx, float3 worldPos, float3 worldNormal, float3 vecDelta, int samp, float3 albedo, float3 lightColor, float dist, float metallic, float roughness, out float3 Diffuse, out float3 Specular)
{
    float4x4 w2t = float4x4(
        g_flMixedData[litIdx + 5], g_flMixedData[litIdx + 6],
        g_flMixedData[litIdx + 7], g_flMixedData[litIdx + 8]
    );

    float viewDot = 1.0f - abs(dot(vecDelta / dist, worldNormal));
    float3 localPos = worldPos + worldNormal * (viewDot + dist * 0.0075f);
    float4 depthPos = mul(float4(localPos, 1.0f), w2t);
    depthPos.xyz /= depthPos.w;

    //float3 flShadow = PerformShadowTHEXA(depthPos, samp, litIdx, g_flMixedData[litIdx + 4].w, vecDelta, dist, worldNormal);

    DoSpotLightPBR(litIdx, worldNormal, g_flMixedData[litIdx].xyz, lightColor,
        dist, worldPos, metallic, roughness, albedo, Diffuse, Specular, true);

/*    return pbr;*/ //* flShadow;

}

void DoPBRCSMCookied(int litIdx, float3 worldPos, float3 worldNormal, float3 vecDelta, int samp, float3 albedo, float3 lightColor, float dist, float metallic, float roughness, out float3 Diffuse, out float3 Specular)
{

    float4x4 w2t = float4x4(
        g_flMixedData[litIdx + 5], g_flMixedData[litIdx + 6],
        g_flMixedData[litIdx + 7], g_flMixedData[litIdx + 8]
    );

    float viewDot = 1.0f - abs(dot(vecDelta / dist, worldNormal));
    float3 localPos = worldPos + worldNormal * (viewDot + dist * 0.0075f);
    float4 depthPos = mul(float4(localPos, 1.0f), w2t);
    depthPos.xyz /= depthPos.w;

    DoSpotLightPBR(litIdx, worldNormal, g_flMixedData[litIdx].xyz, lightColor,
        dist, worldPos, metallic, roughness, albedo, Diffuse, Specular, true);

    float3 flShadow = PerformShadow(depthPos, samp, litIdx, g_flMixedData[litIdx + 4].w, vecDelta, dist, worldNormal);

    float3 stub = 0.0f;

    float3 cubemap = DoLightFinalCookied(g_flMixedData[litIdx + 1].xyz,
        g_flMixedData[litIdx + 2].xyz,
        stub.x, DoStandardCookie(sMixedSampler[samp + 1], depthPos.xy));

    Diffuse *= flShadow + cubemap;
    Specular *= flShadow + cubemap;
}