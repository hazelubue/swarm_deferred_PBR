
float g_SpecularBoost = 0.5f;
float g_DiffuseScale = 1.0f;
float g_SheenStrength = 0.5f;

static const int MAX_FORWARD_LIGHTS = 16;
float4 g_ForwardLightData[MAX_FORWARD_LIGHTS * 2] : register(c69);
float4 g_ForwardSpotLightData[MAX_FORWARD_LIGHTS * 2] : register(c37);
float4 g_ForwardLightCount : register(c11);

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
    return F0 + (max(1.0f.xxx - F0, F0) - F0) * pow(1.0f - cosTheta, 1.11f) * roughness;
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

float ComputeSpotlightAttenuation(int lightIndex, float3 worldPos, float3 lightPos, float radius)
{
    float3 toLight = lightPos - worldPos;
    float dist = length(toLight);
    float3 L = toLight / dist;

    float distNorm = dist / radius;
    float fade = saturate(1.0f - distNorm);
    fade = fade * fade;

    int spotDataIndex = lightIndex * 2;

    float3 spotForwardDir = g_ForwardSpotLightData[spotDataIndex].xyz;
    float coneInner = g_ForwardSpotLightData[spotDataIndex].w;
    float coneOuter = g_ForwardSpotLightData[spotDataIndex + 1].w;

    float spotDot = dot(normalize(spotForwardDir), -L);
    float spotAtten = smoothstep(coneOuter, coneInner, spotDot);

    return fade * spotAtten;
}

float3 calculateLight(int index, float NdotV, float NdotL, float VdotH, float NdotH,
    float3 L, float3 normal, float3 vWorldPos, float3 vEye,
    float roughness, float metalness, float3 albedo,
    float3 effectiveLightColor, float attenuation, float lightType)
{
    float3 V = normalize(vEye - vWorldPos);

    int dataIndex = index * 2;

    float3 lightPos = g_ForwardLightData[dataIndex].xyz;
    float lightRadius = g_ForwardLightData[dataIndex].w;

    float3 toLight = lightPos - vWorldPos;
    float lightToWorldDist = length(toLight);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metalness);

    float3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
    float3 F2 = fresnelSchlickRoughness(NdotV, F0, roughness);
    float3 F3 = fresnelSchlickRoughness(NdotL, F0, roughness);
    float3 Fc = lerp(F, F2 * F3, saturate(roughness * (1.0 - metalness)));

    float alpha = roughness * roughness;
    float D = ndfGGX(NdotH, roughness);

    float G = Visibility_SmithGGX(NdotV, NdotL, alpha);

    float3 kd = (1.0 - F) * (1.0 - metalness);

    float3 diffuseBRDF = Diffuse_OrenNayar(kd * albedo, roughness, NdotV, NdotL, VdotH);

    float3 sheenBRDF = SheenBRDF_DreamWorks(normal, V, L, albedo, 0.01, roughness);

    float3 specularBRDF = (Fc * D * G) / max(0.00001, 4.0 * NdotV * NdotL);

    float3 diffuse = (diffuseBRDF + sheenBRDF) * effectiveLightColor * NdotL;
    float3 specular = specularBRDF * effectiveLightColor * NdotL * 0.5;

    if (lightType == 0.0)
    {
        diffuse *= attenuation;
        specular *= attenuation;
    }
    else if (lightType == 1.0)
    {
        float spotAtten = ComputeSpotlightAttenuation(index, vWorldPos, lightPos, lightRadius);
        diffuse *= spotAtten;
        specular *= spotAtten;
    }

    return (diffuse + specular);
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

//float3 DoPointLightPBR(float3 normal,
//    float4 lightPos, float4 lightColor,
//    float lightToWorldDist, float3 worldPos,
//    float metalScalar, float roughness, float3 albedo)
//{
//    float3 L = normalize(lightPos - worldPos);
//    float3 V = normalize(g_vecOrigin.xyz - worldPos);
//    //float3 N = normalize(normal);
//
//    float lightRadius = lightPos.w;
//
//    float3 flLighting = calculateLight(L, lightColor, V, normal, F0, worldPos, g_vecOrigin, roughness, metalScalar, NdotV, albedo);
//    
//    return flLighting;
//}

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

float4 customTex2D(in float Texture, in float2 uv)
{
    //clamp to 0 - 1
    uv = clamp(uv, 0.0, 1.0);

    float2 texSize = Texture;

    //grab coord
    float2 pixelCoord = uv * texSize;

    int2 pixelPos = int2(pixelCoord);

    //clamp pixel pos to texelsize 1 to 1
    pixelPos = clamp(pixelPos, int2(0, 0), int2(texSize) - int2(1, 1));

    //composite
    float4 color = float4(Texture, pixelPos.xy, 0.0);

    return color;
}

float GenerateMetallic(in float3 normal, in float4 albedo)
{
    float maxChannel = max(albedo.r, max(albedo.g, albedo.b));
    float minChannel = min(albedo.r, min(albedo.g, albedo.b));
    float saturation = (maxChannel - minChannel) / (maxChannel + 0.001);
    float metallicFromSat = smoothstep(0.3, 0.7, saturation);

    float normalVariation = length(ddx(normal)) + length(ddy(normal));
    float smoothnessBoost = 1.0 - smoothstep(0.0, 0.3, normalVariation);

    float metallic = metallicFromSat * (0.7 + smoothnessBoost * 0.3);

    return saturate(metallic);
}

float GenerateRoughness(in float3 normal, in float4 albedo)
{


    //float3 defColor = float3(0.37, 1.5, 1.9);
    float normalVariation = length(ddx(normal)) + length(ddy(normal)) * 8.0f;
    float roughness = smoothstep(0.0, 1.0, normalVariation * 0.4);

    float maxChannel = max(max(albedo.r, albedo.g), max(albedo.b, albedo.a));
    float minChannel = min(min(albedo.r, albedo.g), min(albedo.b, albedo.a));
    float variation = (maxChannel - minChannel) / (maxChannel + 0.001);

    float brightnessRoughness = 1.0 - smoothstep(0.2, 1.0, variation);

    float saturation = variation;
    float desaturationBoost = smoothstep(0.3, 0.05, saturation) * 0.3;

    roughness = roughness * 0.4 + brightnessRoughness * 0.6 + desaturationBoost;

    roughness = max(roughness, 0.02);

    return clamp(roughness, 0.15, 0.85);
}