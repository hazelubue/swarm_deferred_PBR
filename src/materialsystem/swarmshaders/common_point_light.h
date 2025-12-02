//include our tonemapper
#include "tonemap_postprocessing.h"

//sampler sMRAO : register(s15);

static const float EPSILON = 0.00001;

sampler sMixedSampler[FREE_LIGHT_SAMPLERS] : register(FIRST_LIGHT_SAMPLER_FXC);
const float4 g_flMixedData[112] : register(FIRST_SHARED_LIGHTDATA_CONSTANT_FXC);

const float3 g_vecViewOrigin : register(c0);
const float4 g_vecShadowMappingTweaks_0 : register(c1);
const float4 g_vecShadowMappingTweaks_1 : register(c2);

float g_GrazingFactor : register(c11);
float g_GrazingPower : register(c12);
float g_SpecularBoost : register(c16);
float g_SpecSizeScale : register(c15);
float g_DiffuseScale : register(c21);
float g_FresnelPower : register(c22);
float g_SheenStrength : register(c23);
float g_green : register(c27);
float g_green2 : register(c30);
float g_blue : register(c28);

const int g_nParallaxSamples : register(c29);

float3x3 ComputeTangentFrame(float3 N, float3 P, float2 uv, out float3 T, out float3 B, out float sign_det)
{
    float3 dp1 = ddx(P);
    float3 dp2 = ddy(P);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    sign_det = dot(dp2, cross(N, dp1)) > 0.0 ? -1 : 1;

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    T = normalize(mul(float2(duv1.x, duv2.x), inverseM));
    B = normalize(mul(float2(duv1.y, duv2.y), inverseM));
    return float3x3(T, B, N);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0f.xxx - F0, F0) - F0) * pow(1.0f - cosTheta, g_FresnelPower) * roughness;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f.xxx - F0) * pow(1.0f - cosTheta, g_FresnelPower);
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

// Schlick-GGX approximation of geometric attenuation function using Smith's method
// This version remaps the roughness to reduce "hotness", however this should only be used for analytical lights
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

//float3 Diffuse_OrenNayar(float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH)
//{
//    float a = Roughness * Roughness;
//    float s = a;// / ( 1.29 + 0.5 * a );
//    float s2 = s * s;
//    float VoL = 2 * VoH * VoH - 1;		// double angle identity
//    float Cosri = VoL - NoV * NoL;
//    float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
//    float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * (Cosri >= 0 ? 1 / (max(NoL, NoV)) : 1);
//    return DiffuseColor / PI * (C1 + C2) * (1 + Roughness * 0.5);
//}

//float3 DoAmbient(float2 UV, float3 vWorldPos, float3 vWorldNormal, float3 vEye, in float roughness, in float3 albedo, in float3 Ambient, in float3 Ground)
//{
//	float3 V = normalize( vEye - vWorldPos );
//    float NV = max(0.0, dot(vWorldNormal, V) * 0.5f +0.5f);
//    float NU = max(0.0, dot(vWorldNormal, float3(0.0f, 0.0f, 1.0f)) * 0.5f +0.5f);
//
//	float diffuseTransition = NU;
//	float3 diffuse = lerp(Ground, Ambient, diffuseTransition);
//
//	HALF3 reflectVect = 2.0 * NV * vWorldNormal - V;
//	float3 reflection = 0.0f.xxx;
//    for ( int isample = 0; isample < 32; isample++)
//	{
//		float3 randomvec = float3(random(UV + isample), random(UV + isample + 1), random(UV + isample + 2));
//		randomvec = randomvec * 2.0f - 1.0f;
//		reflection += SampleAmbientReflection(float4((reflectVect + (randomvec * roughness * roughness * 1.75f)), roughness * 4.0), Ground, Ambient).rgb / 32.0f;
//	}
//
//
//	return albedo.rgb * lerp(reflection, diffuse, roughness);
//}

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
//float3 DoAmbient( float2 UV, float3 vWorldPos, float3 vWorldNormal, float3 vEye, in float roughness, in float3 albedo, in float3 Ambient, in float3 Ground)
//{
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
//    return albedo.rgb * lerp(reflection, diffuse, roughness);
//}

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
    float NdotVF = dot(normal, V);
    float NdotV = max(0.0f, dot(normal, V));
    float NV = max(0.0f, dot(N, V));
    float LN = cosLightIn;
    float VoH = max(0.0f, dot(V, H));
    float VdotH = max(0.0f, dot(V, H));
    float NdotL = max(0.0f, dot(N, L));


    //corrected fresnel with correct values.
    //old implentation caused dark burning spots on any material.
    float3 F = fresnelSchlickRoughness(fresnelReflectance, VdotH, roughness); // was HL //GREAT effects were with cosHalfAngle, caused normal bug. // Specular reflection
    float3 F2 = fresnelSchlickRoughness(fresnelReflectance, NdotV, roughness); // View-dependent term
    float3 F3 = fresnelSchlickRoughness(fresnelReflectance, NdotL, roughness); // was HL //GREAT effects were with cosHalfAngle, caused normal bug. // Light-dependent term

    float D = ndfGGX(cosHalfAngle, roughness);
    // Calculate geometric attenuation for specular BRDF
    float G = GaSchlickGGXRemapped(cosLightIn, NdotV, roughness);
    // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium
    // Metals on the other hand either reflect or absorb energ so diffuse contribution is always, zero
    // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness
#if SPECULAR
    // Metalness is not used if F0 map is available
    float3 kd = float3(1, 1, 1) - F;
#else
    //float3 kdF2 = float3(1, 1, 1) - F2;
    float3 kd = (float3(1, 1, 1) - F) * (float3(1, 1, 1) - F2);
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
    float3 diffuseBRDF = Diffuse_OrenNayar(F3, roughness, NV, LN, VoH) * g_DiffuseScale;
    float3 sheenBRDF = SheenBRDF_DreamWorks(N, V, L, albedo, g_SheenStrength, roughness);

    float3 specularBRDF = (Fc * D * G) / max(EPSILON, 4.0f);

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

void DoPointLightPBR(const int index, float3 normal,
    float3 lightPos, float3 lightColor,
    float lightToWorldDist, float3 worldPos,
    float metalScalar, float roughness, float3 albedo, out float3 Diffuse, out float3 Specular)
{
    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(g_vecViewOrigin.xyz - worldPos);
    float NdotL = max(0.0f, dot(normal, L));
    float NdotV = max(0.0f, dot(normal, V));

    // Build F0 from albedo & metalness
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalScalar);


    calculateLight(L, lightColor, V, normal, F0, index, worldPos, g_vecViewOrigin, roughness, metalScalar, NdotV, albedo, Diffuse, Specular);

    float radius = g_flMixedData[index].w;
    float distFade = 1.0f - saturate(lightToWorldDist / radius);
    distFade *= distFade;

    Diffuse *= distFade;
    //dont affect specular radius.
    Specular *= distFade;
}

//void LightpassParams(int idx, out float3 lp, out float3 lightColor, out float rad, out float3 lc, out float powf)
//{
//    lp = g_flMixedData[idx].xyz;
//    lightColor = g_flMixedData[idx + 1].xyz;
//    rad = max(g_flMixedData[idx].w, 1e-6);
//    lc = g_flMixedData[idx + 1].xyz;
//    powf = max(g_flMixedData[idx + 1].w, 0.001);
//}

void RotationMatrix(int idx, float3 lp, float3 worldPos, out float d, out float3x3 rot, out float3 local, out float att, float rad, float powf)
{
    float3 toL = lp - worldPos;
    d = length(toL);
    float dn = saturate(d / rad);
    att = pow(smoothstep(1, 0, dn), powf);

    rot = float3x3(
        g_flMixedData[idx + 3].xyz,
        g_flMixedData[idx + 4].xyz,
        g_flMixedData[idx + 5].xyz);
    local = mul3x3(-toL, rot);
}

float3 PerformShadow(int samp, int idx, float rad, float3 local, float d /*,float3 vWorldNormal*/)
{
    float3 Out;
    Out = PerformDualParaboloidShadow(
        sMixedSampler[samp], local,
        g_vecShadowMappingTweaks_0,
        g_vecShadowMappingTweaks_1,
        d, rad, g_flMixedData[idx + 2].w);

    return Out;
}

float3 PerformShadowGlass(int samp, int idx, float rad, float3 local, float d, float3 albedo, float alpha /*,float3 vWorldNormal*/)
{
    float3 Out;
    Out = PerformDualParaboloidShadow(
        sMixedSampler[samp], local,
        g_vecShadowMappingTweaks_0,
        g_vecShadowMappingTweaks_1,
        d, rad, g_flMixedData[idx + 2].w);

    float3 coloredShadow = Out * albedo;

    return lerp(coloredShadow, coloredShadow, alpha);
}

void DoPBRCSMTHEXAGlass(int samp, int idx, float rad, float3 local, float d, float3 vWorldPos, float3 vWorldNormal, float3 albedo, float3 albedoSample, float alpha, float3 dist, float3 vColor, float metallness, float roughness, out float3 Diffuse, out float3 Specular)
{

    float3 flShadow = PerformShadowGlass(samp, idx, rad, local, d, albedoSample, alpha);

    //float att = pow(smoothstep(1.0, 0.0, saturate(d / rad)), 2.0f);

    DoPointLightPBR(idx, vWorldNormal, g_flMixedData[idx].xyz, vColor,
        dist, vWorldPos, metallness, roughness, albedo, Diffuse, Specular);

    Diffuse *= flShadow;
    Specular *= flShadow;
}

void DoPBRCSMTHEXA(int samp, int idx, float rad, float3 local, float d, float3 vWorldPos, float3 vWorldNormal, float3 albedo, float3 dist, float3 vColor, float metallness, float roughness, out float3 Diffuse, out float3 Specular)
{
    float3 flShadow = PerformShadow(samp, idx, rad, local, d);

    //float att = pow(smoothstep(1.0, 0.0, saturate(d / rad)), 2.0f);

    DoPointLightPBR(idx, vWorldNormal, g_flMixedData[idx].xyz, vColor,
        dist, vWorldPos, metallness, roughness, albedo, Diffuse, Specular);

    Diffuse *= flShadow;
    Specular *= flShadow;
}

void DoPBRCSMCookied(int samp, int cookieSamp, int idx, float rad, float3 local, float d, float3 vWorldPos, float3 vWorldNormal, float3 albedo, float3 dist, float3 vColor, float metallness, float roughness, out float3 Diffuse, out float3 Specular)
{
    float3 cookie = DoCubemapCookie(sMixedSampler[cookieSamp], local);
    float3 flShadow = PerformShadow(samp, idx, rad, local, d);

    DoPointLightPBR(idx, vWorldNormal, g_flMixedData[idx].xyz, vColor,
        dist, vWorldPos, metallness, roughness, albedo, Diffuse, Specular);

    Diffuse *= flShadow * cookie;
    Specular *= flShadow * cookie;
}


//float3 DoPBRTHEXA(int litIdx, float3 vWorldPos, float3 vWorldNormal, float3 vColor, float dist, float metallness, float roughness)
//{
//    //float att = pow(smoothstep(1.0, 0.0, saturate(d / rad)), 2.0f);
//
//    float3 Out = DoPointLightPBR(litIdx, vWorldNormal, g_flMixedData[litIdx].xyz, vColor,
//       dist, vWorldPos, metallness, roughness);
//
//    return Out;
//}

//float3 readMRAO(in float2 coord)
//{
//    return tex2D(sMRAO, coord);
//}
