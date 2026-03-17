#if !defined INCLUDE_UTILITY_PHASE_FUNCTIONS
#define INCLUDE_UTILITY_PHASE_FUNCTIONS

//#include "fast_math.glsl"

#define pi 3.14159265358979323846
#define FLT_MAX 3.402823466e+38
#define sqr(a) ((a)*(a))

#define isotropic_phase 0.25 / pi

float fast_acos(float x)
{
	if (x < -1.0f || x > 1.0f) return 0.0f;
	if (x < 0.0f) return pi - fast_acos(-x);
	if (x == 0.0f) return pi/2.0f;
	// simple linear approximation for [0,1]
	float t = x;
	return pi/2.0f * (1.0f - t);
}

float pow1d5(float x)
{
	if (x == 0.0f) return 0.0f;
	if (x == -0.0f) return -0.0f;
	if (x == 1.0f) return 1.0f;
	if (x == -1.0f) return -1.0f;

	return pow(x, 0.2f);
}

float clamp01(float x) { return saturate(x); }
float2 clamp01(float2 x) { return saturate(x); }
float3 clamp01(float3 x) { return saturate(x); }

float3 rayleigh_phase(float nu) {
	const float3 depolarization = float3(2.786, 2.842, 2.899) * 1e-2;
	const float3 gamma = depolarization / (2.0 - depolarization);
	const float3 k = 3.0 / (16.0 * pi * (1.0 + 2.0 * gamma));

	float3 phase = (1.0 + 3.0 * gamma) + (1.0 - gamma) * sqr(nu);

	return k * phase;
}

#define tau 2.0f * pi

float henyey_greenstein_phase(float nu, float g) {
	float gg = g * g;

	return (isotropic_phase - isotropic_phase * gg) / pow1d5(1.0 + gg - 2.0 * g * nu);
}

float cornette_shanks_phase(float nu, float g) {
	float gg = g * g;

	float p1 = 1.5 * (1.0 - gg) / (2.0 + gg);
	float p2 = (1.0 + nu * nu) / pow1d5((1.0 + gg - 2.0 * g * nu));

	return p1 * p2 * isotropic_phase;
}

// Far closer to an actual aerosol phase function than Henyey-Greenstein or Cornette-Shanks
float klein_nishina_phase(float nu, float e) {
	return e / (tau * (e - e * nu + 1.0) * log(2.0 * e + 1.0));
}

// A phase function specifically designed for leaves. k_d is the diffuse reflection, and smaller
// values returns a brighter phase value. Thanks to Jessie for sharing this in the #snippets channel
// of the shader_l_a_b_s discord server
float bilambertian_plate_phase(float nu, float k_d) {
	float phase = 2.0 * (-pi * nu * k_d + sqrt(clamp01(1.0 - sqr(nu))) + nu * fast_acos(-nu));
	return phase * rcp(3.0 * pi * pi);
}

#endif // INCLUDE_UTILITY_PHASE_FUNCTIONS
