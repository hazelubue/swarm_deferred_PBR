
float3 ACES_Narkowicz_Tonemap(float3 LinearColor, float exposure /*default 1.0 - multiplies the internal exposure multiplier*/)
{
    // default multiplier from the original snippet (ExposureCompensation=1)
    const float DefaultExposureMultiplier = 1.4;

    // Pre-tonemapping transform (AP1-ish / working space)
    const float3x3 PRE_TONEMAPPING_TRANSFORM =
    {
         0.575961650,  0.344143820,  0.079952030,
         0.070806820,  0.827392350,  0.101774690,
         0.028035252,  0.131523770,  0.840242300
    };

    // Post-tonemapping transform (inverse-ish)
    const float3x3 POST_TONEMAPPING_TRANSFORM =
    {
         1.666954300, -0.601741150, -0.065202855,
        -0.106835220,  1.237778600, -0.130948950,
        -0.004142626, -0.087411870,  1.091555000
    };

    // effective exposure (keeps original behavior if exposure == 1)
    float effExposure = DefaultExposureMultiplier * exposure;

    // transform color into working space and apply exposure
    float3 WorkingColor = mul(effExposure * PRE_TONEMAPPING_TRANSFORM, LinearColor);

    // Narkowicz 2016 rational polynomial fit (per-channel, vectorized)
    // y = (x*(a*x + b)) / (x*(c*x + d) + e)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    WorkingColor = (WorkingColor * (a * WorkingColor + b)) / (WorkingColor * (c * WorkingColor + d) + e);

    // clamp in working space (avoid weird negatives)
    WorkingColor = saturate(WorkingColor);

    // back to display / scene RGB
    float3 OutColor = mul(POST_TONEMAPPING_TRANSFORM, WorkingColor);

    // final clamp and return (still linear)
    return saturate(OutColor);
}

float3 ACES_Tonemap(float3 LinearColor, float exposure)
{
    exposure = 1.4;

    const float3x3 PRE_TONEMAPPING_TRANSFORM =
    {
         0.575961650,  0.344143820,  0.079952030,
         0.070806820,  0.827392350,  0.101774690,
         0.028035252,  0.131523770,  0.840242300
    };
    const float3x3 EXPOSED_PRE_TONEMAPPING_TRANSFORM = exposure * PRE_TONEMAPPING_TRANSFORM;
    const float3x3 POST_TONEMAPPING_TRANSFORM =
    {
         1.666954300, -0.601741150, -0.065202855,
        -0.106835220,  1.237778600, -0.130948950,
        -0.004142626, -0.087411870,  1.091555000
    };

    // Transform color spaces, perform blue correction and pre desaturation
    float3 WorkingColor = mul(EXPOSED_PRE_TONEMAPPING_TRANSFORM, LinearColor);

    // Apply tonemapping curve
    // Narkowicz 2016, "ACES Filmic Tone Mapping Curve"
    // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    WorkingColor = saturate((WorkingColor * (a * WorkingColor + b)) / (WorkingColor * (c * WorkingColor + d) + e));

    // Transform color spaces, apply blue correction and post desaturation
    return mul( POST_TONEMAPPING_TRANSFORM, WorkingColor );
}
