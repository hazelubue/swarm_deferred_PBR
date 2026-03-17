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

float3 calculateLight(float3 lightIn, float3 lightIntensity, float3 lightOut, float3 normal, float3 fresnelReflectance, float3 vWorldPos, float3 vEye, float roughness, float metalness, float lightDirectionAngle, float3 albedo, float3 vecWorld, float3 vecWorldRay)
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

    float3 Diffuse, Specular;

    Diffuse = (diffuseBRDF + sheenBRDF) * lightIntensity * g_light_diffuse * LN;
    Specular = specularBRDF * g_SpecularBoost * g_light_diffuse * lightIntensity;

    float3 finalColor = Diffuse + Specular;

#if LIGHTMAPPED && !FLASHLIGHT
    return specularBRDF * lightIntensity * LN;
#else

    //return the computed pbr light with tone mapping and gamma correction
    return finalColor;

    //old method
    //return (diffuseBRDF + specularBRDF * g_SpecularBoost + sheenBRDF) * lightIntensity * LN;
#endif
}

float LinearizeDepth(float d, float f, float n)
{
    return n * f / (f + d * (n - f));
}

float3 readWorld(in float2 UV, sampler input_depth)
{
    float depth = LinearizeDepth(tex2D(input_depth, UV).a, g_zFar, g_zNear);
    float2 screenUV = UV * 2.0f - 1.0f;
    float3 viewRay = normalize(g_ViewForward + g_ViewRight * screenUV.x - g_ViewUp * screenUV.y);
    float distance = depth / dot(viewRay, g_ViewForward);
    return g_ViewOrigin + viewRay * distance;
}

// from saruna also correct math

float2 ClipToScreenXY(float2 UV)
{
    return float2(UV.x * 0.5f + 0.5f, -UV.y * 0.5f + 0.5f);
}

float2 WorldToUV(in float3 position)
{
    float4 viewPos = mul(float4(position, 1.0f), GetViewProj());
    viewPos.xyz /= viewPos.w;
    return ClipToScreenXY(viewPos.xy);
}

float3 traceRays(float3 worldPos, sampler input_depth, float3 vecWorld, float3 vecWorldRay, float3 lightPos)
{
    float3 RayPos = lightPos;
    float3 RayDir = normalize(vecWorldRay);

    float RayStep = 0.1;

    float g_flRayStepDiv = 0.5;
    float g_flDepthDiffMax = 0.01;
    int maxIterations = 4;
    int refinementSteps = 1;

    float3 vecMarchingRay = worldPos - vecWorld;
    float3 vecMarchingNormal = normalize(vecMarchingRay);
    float flProjectedDot = step(0.0001f, dot(vecMarchingNormal, RayDir));

    for (int j = 0; j < maxIterations; j++)
    {
        RayPos += RayDir * RayStep;

        float2 rayUV = WorldToUV(RayPos);


        if (any(rayUV < 0.0) || any(rayUV > 1.0))
            break;

        float sceneDepth = tex2Dlod(input_depth, float4(rayUV, 0, 0)).r;

        float3 sceneWorldPos = readWorld(rayUV, input_depth);

        float rayDepth = length(RayPos - worldPos);
        float sceneDistance = length(sceneWorldPos - worldPos);
        float depthDiff = rayDepth - sceneDistance;

        if (depthDiff > 0 && depthDiff < g_flDepthDiffMax)
        {
            float3 refinedPos = RayPos;
            float refinedStep = RayStep;

            for (int i = 0; i < refinementSteps; i++)
            {
                refinedStep *= g_flRayStepDiv;
                refinedPos -= RayDir * refinedStep;

                float2 refinedUV = WorldToUV(refinedPos);
                float3 refinedScenePos = readWorld(refinedUV, input_depth);

                float refinedRayDist = length(refinedPos - worldPos);
                float refinedSceneDist = length(refinedScenePos - worldPos);

                if (refinedRayDist < refinedSceneDist)
                    refinedPos += RayDir * refinedStep;
            }

            return refinedPos;
        }
    }

    return RayPos;
}

float3 SampleCosineHemisphere(float2 random, float3 normal)
{
    float r = sqrt(random.x);
    float theta = 2.0 * PI * random.y;
    float2 disk = float2(r * cos(theta), r * sin(theta));

    float3 direction;
    direction.xy = disk;
    direction.z = sqrt(max(0.0, 1.0 - dot(disk, disk)));

    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    return tangent * direction.x + bitangent * direction.y + normal * direction.z;
}

float2 GetRand(float2 uv, int sampleIndex)
{
    float2 seed = uv + float2(sampleIndex * 0.1, sampleIndex * 0.2);
    return frac(sin(dot(seed, float2(12.9898, 78.233))) * float2(43758.5453, 28001.1378));
}

float2 traceRaysAroundHemisphere(float3 worldPos, float3 N, sampler input_depth, sampler input_normal, float3 albedo, float3 vecWorld, float3 lightPos, int numSamples)
{
    float3 accumLighting = 0;
    float2 screenUV = WorldToUV(worldPos);

    float2 hitUV = (float2)0;

    for (int i = 0; i < numSamples; i++)
    {
        float2 random = GetRand(screenUV, i);
        float3 rayDir = SampleCosineHemisphere(random, N);

        float3 hitPosition = traceRays(worldPos, input_depth, vecWorld, rayDir, lightPos);

        hitUV = WorldToUV(hitPosition);
        
    }

    return hitUV;
}


float3 rayTraceVolume(float3 worldPos, float3 N, sampler input_depth, float3 vecWorld, float3 lightPosition, float3 lightIntensity, int numSamples)
{

    // Calculate screen-space coordinates for random sampling
    float2 screenUV = (worldPos.xy / g_vecFullScreenTexel.xy) * 2 - 1;

    // Initialize hit color
    float3 hitColor = 0;

    float2 rand = GetRand(screenUV, numSamples);

    // Sample the hemisphere multiple times
    for (int i = 0; i < numSamples; i++)
    {
        // Get a random point in the hemisphere
        float3 rayDir = SampleCosineHemisphere(rand, N);

        // Trace the ray and get the hit position
        float3 hitPos = traceRays(worldPos, input_depth, vecWorld, rayDir, lightPosition);

        // Calculate the distance from the start to the hit
        float dist = length(hitPos - worldPos);

        // Simple linear fog calculation
        float fogFactor = saturate(dist / 10.0); // Adjust 10.0 to control fog range

        // Mix between fog color and light color
        float3 color = lerp(float3(0.5, 0.7, 1.0), lightIntensity, fogFactor);

        // Accumulate the color
        hitColor += color;
    }

    // Average over all samples
    hitColor /= numSamples;

    return hitColor;
}


//float3 rayTraceDiffuse(float3 worldPos, float3 N, float3 viewDir, sampler input_color, sampler input_depth, sampler input_normal, float3 albedo, float3 vecWorld, float3 fresnelReflectance, float3 lightPosition, float3 lightIntensity, float roughness, float metalness, int numSamples)
//{
//    float2 rayUV = traceRaysAroundHemisphere(worldPos, N, viewDir, input_depth, input_normal, albedo, vecWorld, fresnelReflectance, lightPosition, lightIntensity, roughness, metalness, numSamples);
//
//    if (any(rayUV < 0.0) || any(rayUV > 1.0))
//        return float3(0, 0, 0); 
//
//    float3 hitColor = tex2Dlod(input_color, float4(rayUV, 0, 0)).rgb;
//    float3 hitNormal = tex2Dlod(input_normal, float4(rayUV, 0, 0)).rgb * 2.0 - 1.0;
//    float hitDepth = tex2Dlod(input_depth, float4(rayUV, 0, 0)).r;
//
//    float4 clipPos = float4(rayUV.x * 2.0 - 1.0,
//                          -(rayUV.y * 2.0 - 1.0),
//                            hitDepth, 1.0);
//    float4 worldPosFromDepth = mul(clipPos, GetViewProjInv());
//    float3 hitWorldPos = worldPosFromDepth.xyz / worldPosFromDepth.w;
//
//    float3 L = normalize(lightPosition - hitWorldPos);
//    float NdotL = max(0.0, dot(normalize(hitNormal), L));
//
//    float distance = length(lightPosition - hitWorldPos);
//    float attenuation = 1.0 / (1.0 + distance * distance);
//
//    return hitColor * lightIntensity * NdotL * attenuation * albedo;
//}

float3 EvaluateFastSSS(float3 normal, float3 viewDir, float3 lightDir, float3 lightColor, float3 sssColor, float thickness,
    float distortion, float power, float scale, float ambient, float attenuation)
{
    float3 H = normalize(lightDir + normal * distortion);
    float VdotH = saturate(dot(viewDir, -H));
    float back = pow(VdotH, power) * scale;
    float intensity = attenuation * (back + ambient) * thickness;
    //float shadowFactor = lerp(shadow, 1.0f, ignoreShadows);
    float ndotl = saturate(dot(normal, lightDir));
    float shadowApplied = lerp(1.0f, 0.0, ndotl);
    //shadowFactor = lerp(shadowApplied, 0.0, backfaceShadow);
    return lightColor * sssColor * intensity;
}