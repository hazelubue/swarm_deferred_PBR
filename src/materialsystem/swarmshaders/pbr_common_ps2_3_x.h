#define PI			3.141592653589793
#define EPSILON		0.00001

sampler sMixedSampler[FREE_LIGHT_SAMPLERS] : register(FIRST_LIGHT_SAMPLER_FXC);
const float4 g_flMixedData[112] : register(FIRST_SHARED_LIGHTDATA_CONSTANT_FXC);

float g_DiffuseScale : register(c4);
float g_SheenStrength : register(c5);
float g_SpecularBoost : register(c6);

// PBR BRDF functions
// Based on https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0f.xxx - F0, F0) - F0) * pow(1.0f - cosTheta, 10.0f) * roughness;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f.xxx - F0) * pow(1.0f - cosTheta, 10.0f);
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

float3 Diffuse_OrenNayar(float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH)
{
    float a = Roughness * Roughness;
    float s = a;// / ( 1.29 + 0.5 * a );
    float s2 = s * s;
    float VoL = 2 * VoH * VoH - 1;		// double angle identity
    float Cosri = VoL - NoV * NoL;
    float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
    float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * (Cosri >= 0 ? 1 / (max(NoL, NoV)) : 1);
    return DiffuseColor / PI * (C1 + C2) * (1 + Roughness * 0.5);
}


// Monte Carlo integration, approximate analytic version based on Dimitar Lazarov's work
// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
float3 EnvBRDFApprox(float3 SpecularColor, float Roughness, float NoV)
{
    const float4 c0 = { -1, -0.0275, -0.572, 0.022 };
    const float4 c1 = { 1, 0.0425, 1.04, -0.04 };
    float4 r = Roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
    return SpecularColor * AB.x + AB.y;
}

// Compute the matrix used to transform tangent space normals to world space
// This expects DirectX normal maps in Mikk Tangent Space http://www.mikktspace.com
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

float GetAttenForLight(float4 lightAtten, int lightNum)
{
	return lightAtten[lightNum];
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


void calculateLight(float3 lightIn, float3 lightIntensity, float3 lightOut, float3 normal, float3 fresnelReflectance, int index, float3 vWorldPos, float3 vEye, float roughness, float metalness, float lightDirectionAngle, float3 albedo, out float3 Diffuse, out float3 Specular)
{
    float3 L = normalize(lightIn);
    float3 V = normalize(lightOut);
    float3 N = normalize(normal);

    float3 HalfAngle = normalize(L + V);
    float3 H = (dot(HalfAngle, HalfAngle) > 0.0f) ? HalfAngle : N;

    float cosLightIn = max(0.0f, dot(N, L));
    float cosHalfAngle = max(0.0f, dot(N, H));

    float HV = max(0.0f, dot(H, V));
    float HL = max(0.0f, dot(H, L));
    float NdotV = max(0.0f, dot(normal, V));
    float NV = max(0.0f, dot(N, V));
    float LN = cosLightIn;
    float VoH = max(0.0f, dot(V, H));


    //corrected fresnel with correct values.
    //old implentation caused dark burning spots on any material.
    float3 F = fresnelSchlick(fresnelReflectance, HL);
    float3 F2 = fresnelSchlick(fresnelReflectance, HV);
    float3 F3 = fresnelSchlick(fresnelReflectance, HL);

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
    float3 kd = lerp((float3(1, 1, 1) - F) * (float3(1, 1, 1) - F2), float3(1, 1, 1) - F3, 0.0);
#endif

    // composite all of our fresnel, account for size distortion of lights.
    // important that metalness is used here
    float3 Fc = lerp((float3(1, 1, 1) - F2) * (float3(1, 1, 1) - F3), F, roughness);

    //compute ambient here once instead of per loop.
    //float3 ambient = g_flMixedData[index + 2].xyz;

    //float groundIntensity = dot(ambient, float3(0.2126, 0.7152, 0.0722));
    //groundIntensity = saturate(groundIntensity);

    //float3 groundColor = albedo * groundIntensity;

    float3 diffuseBRDF = Diffuse_OrenNayar(kd, roughness, NV, LN, VoH) * g_DiffuseScale;
    float3 sheenBRDF = SheenBRDF_DreamWorks(N, V, L, albedo, g_SheenStrength, roughness);

    float3 specularBRDF = (Fc * D * G) / max(EPSILON, 4.0f);

    //float3 CompositeAmbient = Ambient;/*DoAmbient( UV, vWorldPos, normal, vEye, roughness, albedo, ambient, groundColor);*/

    //composite everything
    //float3 finalColor = (diffuseBRDF + specularBRDF * g_SpecularBoost + sheenBRDF + CompositeAmbient) * lightIntensity * LN;

    Diffuse = (diffuseBRDF + sheenBRDF) * lightIntensity * LN;
    Specular = specularBRDF * g_SpecularBoost * lightIntensity;

//#if LIGHTMAPPED && !FLASHLIGHT
//    return specularBRDF * lightIntensity * LN;
//#else

    //return the computed pbr light with tone mapping and gamma correction
    //return finalColor;

    //old method
    //return (diffuseBRDF + specularBRDF * g_SpecularBoost + sheenBRDF) * lightIntensity * LN;
//#endif
}

// Get diffuse ambient light
float3 AmbientLookupLightmap(float3 worldPos,
	float3 normal,
	float3 textureNormal,
	float4 lightmapTexCoord1And2,
	float4 lightmapTexCoord3,
	sampler LightmapSampler,
	float4 modulation)
{
	float2 bumpCoord1;
	float2 bumpCoord2;
	float2 bumpCoord3;

	ComputeBumpedLightmapCoordinates(
		lightmapTexCoord1And2, lightmapTexCoord3.xy,
		bumpCoord1, bumpCoord2, bumpCoord3);

    float3 lightmapColor1 = tex2D(LightmapSampler, bumpCoord1).rgb;
    float3 lightmapColor2 = tex2D(LightmapSampler, bumpCoord2).rgb;
    float3 lightmapColor3 = tex2D(LightmapSampler, bumpCoord3).rgb;

	float3 dp;
	dp.x = saturate(dot(textureNormal, bumpBasis[0]));
	dp.y = saturate(dot(textureNormal, bumpBasis[1]));
	dp.z = saturate(dot(textureNormal, bumpBasis[2]));
	dp *= dp;

    float3 diffuseLighting = dp.x * lightmapColor1.rgb +
        dp.y * lightmapColor2.rgb +
        dp.z * lightmapColor3.rgb;

	float sum = dot(dp, float3(1, 1, 1));
	if (sum != 0)
		diffuseLighting *= modulation.xyz / sum;
	return diffuseLighting;
}



//float3 AmbientLookup(float3 worldPos, float3 normal, float3 ambientCube[6], float3 textureNormal, float4 lightmapTexCoord1And2, float4 lightmapTexCoord3, sampler LightmapSampler, float4 modulation)
//{
//#if ( LIGHTMAPPED )
//	{
//		return AmbientLookupLightmap(worldPos,
//			normal,
//			textureNormal,
//			lightmapTexCoord1And2,
//			lightmapTexCoord3,
//			LightmapSampler,
//			modulation);
//	}
//#else
//	{
//		return PixelShaderAmbientLight(normal, ambientCube);
//	}
//#endif
//}
//
//float3 AmbientCubeLookup(float3 normal, float3 ambientCube[6])
//{
//    return PixelShaderAmbientLight(normal, ambientCube);
//}

float ScreenSpaceBayerDither(float2 vScreenPos)
{
	int x = vScreenPos.x % 8;
	int y = vScreenPos.y % 8;

	const int dither[8][8] = {
	{ 0, 32, 8, 40, 2, 34, 10, 42}, /* 8x8 Bayer ordered dithering */
	{48, 16, 56, 24, 50, 18, 58, 26}, /* pattern. Each input pixel */
	{12, 44, 4, 36, 14, 46, 6, 38}, /* is scaled to the 0..63 range */
	{60, 28, 52, 20, 62, 30, 54, 22}, /* before looking in this table */
	{ 3, 35, 11, 43, 1, 33, 9, 41}, /* to determine the action. */
	{51, 19, 59, 27, 49, 17, 57, 25},
	{15, 47, 7, 39, 13, 45, 5, 37},
	{63, 31, 55, 23, 61, 29, 53, 21} };

	float limit = 0.0;

	{
		limit = (dither[x][y] + 1) / 64.0;
	}

	return limit;
}

float3 worldToRelative(float3 worldVector, float3 surfTangent, float3 surfBasis, float3 surfNormal)
{
	return float3(
		dot(worldVector, surfTangent),
		dot(worldVector, surfBasis),
		dot(worldVector, surfNormal)
	);
}

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float VanDerCorput(int n, int base)
{
    float invBase = 1.0 / float(base);
    float denom = 1.0;
    float result = 0.0;

    for (int i = 0; i < 32; ++i)
    {
        if (n > 0)
        {
            denom = mod(float(n), 2.0);
            result += denom * invBase;
            invBase = invBase / 2.0;
            n = int(float(n) / 2.0);
        }
    }

    return result;
}
// ----------------------------------------------------------------------------
float2 Hammersley(int i, int N)
{
    return float2(float(i) / float(N), VanDerCorput(i, 2u));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space floattor to world-space sample floattor
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 samplefloat = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(samplefloat);
}