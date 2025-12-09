
float g_SpecularBoost = 0.5f;
float g_DiffuseScale = 1.0f;
float g_SheenStrength = 0.5f;

const float2 g_vecFullScreenTexel : register(c1);
const float4 g_vecFogParams : register(c2);
const float3 g_vecOrigin : register(c3);

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
    denom = 3.1456 * denom * denom;

    return num / denom;
}
float CharlieDistribution(float NdotH, float alpha)
{
    float sinThetaH = sqrt(1.0 - NdotH * NdotH);
    float alphaInv = 1.0 / max(alpha, 0.001);
    float exponent = alphaInv;
    float numer = (2.0 + exponent) * pow(sinThetaH, exponent);
    return numer / (2.0 * 3.1456);
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
    return a2 / (3.1456 * denom * denom);
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

float3 Diffuse_OrenNayar(float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH)
{
    float a = Roughness * Roughness;
    float s = a;// / ( 1.29 + 0.5 * a );
    float s2 = s * s;
    float VoL = 2 * VoH * VoH - 1;		// double angle identity
    float Cosri = VoL - NoV * NoL;
    float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
    float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * (Cosri >= 0 ? 1 / (max(NoL, NoV)) : 1);
    return DiffuseColor / 3.1456 * (C1 + C2) * (1 + Roughness * 0.5);
}

void calculateLight(float3 lightIn, float4 lightIntensity, float3 lightOut, float3 normal, float3 fresnelReflectance, float3 vWorldPos, float3 vEye, float roughness, float metalness, float lightDirectionAngle, float3 albedo, out float3 Diffuse, out float3 Specular)
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
    float vDotN = max(0.0f, dot(V, N));


    //corrected fresnel with correct values.
    //old implentation caused dark burning spots on any material.
    float3 F = fresnelSchlickRoughness(vDotN, fresnelReflectance, roughness);
    float3 F2 = fresnelSchlickRoughness(NdotV, fresnelReflectance, roughness);
    float3 F3 = fresnelSchlickRoughness(NdotL, fresnelReflectance, roughness);

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

    float3 specularBRDF = (Fc * D * G) / max(0.00001, 4.0f * cosLightIn * lightDirectionAngle);
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

//void DoPointLightPBR(const int index, float3 normal,
//    float3 lightPos, float3 lightColor,
//    float lightToWorldDist, float3 worldPos,
//    float metalScalar, float roughness, float3 albedo, out float3 Diffuse, out float3 Specular)
//{
//    float3 L = normalize(lightPos - worldPos);
//    float3 V = normalize(g_vecOrigin.xyz - worldPos);
//    float NdotL = max(0.0f, dot(normal, L));
//    float NdotV = max(0.0f, dot(normal, V));
//
//    // Build F0 from albedo & metalness
//    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalScalar);
//
//
//    calculateLight(L, lightColor, V, normal, F0, index, worldPos, g_vecOrigin, roughness, metalScalar, NdotV, albedo, Diffuse, Specular);
//
//    float radius = g_flMixedData[index].w;
//    float distFade = 1.0f - saturate(lightToWorldDist / radius);
//    distFade *= distFade;
//
//    Diffuse *= distFade;
//    //dont affect specular radius.
//    Specular *= distFade;
//}

void DoPointLightPBR(float3 normal,
    float4 lightPos, float4 lightColor,
    float lightToWorldDist, float3 worldPos,
    float metalScalar, float roughness, float3 albedo, out float3 Diffuse, out float3 Specular)
{
    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(g_vecOrigin.xyz - worldPos);
    //float3 N = normalize(normal);

    float lightRadius = lightPos.w;

    float NdotL = max(0.0f, dot(normal, L));
    float NdotV = max(0.0f, dot(normal, V));

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalScalar);

    calculateLight(L, lightColor, V, normal, F0, worldPos, g_vecOrigin, roughness, metalScalar, NdotV, albedo, Diffuse, Specular);

    float distFade = 1.0f - saturate(lightToWorldDist / max(lightRadius, 0.001));
    distFade *= distFade;

    Diffuse *= distFade; 
    Specular *= distFade;
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

    float phi = 2.0 * 3.1456 * Xi.x;
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