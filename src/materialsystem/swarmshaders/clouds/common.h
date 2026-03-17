#if !defined INCLUDE_SKY_CLOUDS_COMMON
#define INCLUDE_SKY_CLOUDS_COMMON

uniform float day_factor;

float3 mix(float3 a, float3 b, float t)
{
	return a + (b - a) * t;
}

float length_squared(float2 v) { return dot(v, v); } 
float length_squared(float3 v) { return dot(v, v); } 
float length_squared(float4 v) { return dot(v, v); }

float cube(float x) { return pow(x, 1.0 / 3.0); }
float dampen(float x) { return sqrt(x); }
float pow4(float x) { return pow(x, 4.0); }
float pow8(float x) { return pow(x, 8.0); }

float pulse(float t, float duration, float frequency)
{
	return smoothstep(0.0, 1.0, sin(t * frequency) * 0.5 + 0.5) * duration;
}

float lift(float x, float power) {
	return pow(saturate(x), power);
}

float cubic_smooth(float x) {
	return x * x * (3.0 - 2.0 * x);
}

float invSqrt(float v) {
	float y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float2 invSqrt(float2 v) {
	float2 y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float3 invSqrt(float3 v) {
	float3 y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float4 invSqrt(float4 v) {
	float4 y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float rcp_length(float2 v) { return invSqrt(dot(v, v)); }
float rcp_length(float3 v) { return invSqrt(dot(v, v)); }

float2 intersect_cylindrical_shell(float3 ray_origin, float3 ray_dir, float inner_cylinder_radius, float outer_cylinder_radius) {
	float len_o = length(ray_origin.xz);
	float rlen_d = rcp_length(ray_dir.xz);

	float t1 = (inner_cylinder_radius - len_o) * rlen_d;
	float t2 = (outer_cylinder_radius - len_o) * rlen_d;

	return float2(t1, t2);
}

struct CloudsParameters {
	// Cumulus congestus
	float cumulus_congestus_blend; // replaces layer 0
	// Volumetric layer 0
	float2  l0_coverage;
	float2  l0_detail_weights;
	float2  l0_edge_sharpening;
	float l0_altitude_scale;
	float l0_cumulus_stratus_blend;
	float l0_extinction_coeff; // also applies for Cu Con
	float l0_scattering_coeff; // also applies for Cu Con
	float l0_shadow;
	// Volumetric layer 1
	float2  l1_coverage;
	float l1_cumulus_stratus_blend;
	float l1_shadow;
	// Planar clouds
	float cirrus_amount;
	float cirrocumulus_amount;
	float noctilucent_amount;
	// Other
	float crepuscular_rays_amount;
};

struct CloudsResult {
	float4 scattering;
	float transmittance;
	float apparent_distance;
};

static const CloudsResult clouds_not_hit = {
	float4(0.0, 0.0, 0.0, 0.0),
	1.0,
	1e5
};

CloudsResult CreateCloudsResult(float4 scattering, float transmittance, float apparent_distance) {
	CloudsResult result;
	result.scattering = scattering;
	result.transmittance = transmittance;
	result.apparent_distance = apparent_distance;
	return result;
}

////const CloudsResult clouds_not_hit = CloudsResult(
////	float4(0, 0, 0, 0),
////	1.0,
////	1e5
////);

float max0(float x) { return max(0.0, x); }

float3 rotateAroundAxis_whydoesthisalreadyexist(float2 p, float3 a, float angle)
{
	a = normalize(a);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	float3 crossAxisPoint = cross(a, float3(p,0.0));
	float dotAxisPoint = dot(a, p);

	return oc * a * dotAxisPoint + c * float3(p,0.0) + s * crossAxisPoint;
}

float2 hash2(float2 p) {
	p = float2(dot(p, float2(127.1, 311.7)),
		dot(p, float2(269.5, 183.3)));
	return frac(sin(p) * 43758.5453123);
}

float2 hash2(float3 p) {
	return hash2(p.xy + p.z);
}

float linear_step(float edge0, float edge1, float x) {
	return saturate((x - edge0) / (edge1 - edge0));
}

// from https://iquilezles.org/articles/gradientnoise/
float2 perlin_gradient(float2 coord) {
	float2 i = floor(coord);
	float2 f = frac(coord);

	float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	float2 du = 30.0 * f * f * (f * (f - 2.0) + 1.0);

	float2 g0 = hash2(i + float2(0.0, 0.0));
	float2 g1 = hash2(i + float2(1.0, 0.0));
	float2 g2 = hash2(i + float2(0.0, 1.0));
	float2 g3 = hash2(i + float2(1.0, 1.0));

	float v0 = dot(g0, f - float2(0.0, 0.0));
	float v1 = dot(g1, f - float2(1.0, 0.0));
	float v2 = dot(g2, f - float2(0.0, 1.0));
	float v3 = dot(g3, f - float2(1.0, 1.0));

	return float2(
		g0 + u.x * (g1 - g0) + u.y * (g2 - g0) + u.x * u.y * (g0 - g1 - g2 + g3) + // d/dx
		du * (u.yx * (v0 - v1 - v2 + v3) + float2(v1, v2) - v0)                      // d/dy
	);
}

float2 curl2D(float2 coord) {
	float2 gradient = perlin_gradient(coord);
	return float2(gradient.y, -gradient.x);
}

float clouds_phase_single(float cos_theta) { // Single scattering phase function
	float forwards_a = klein_nishina_phase(cos_theta, 2600.0); // this gives a nice glow very close to the sun
	float forwards_b = henyey_greenstein_phase(cos_theta, 0.8);

	return 0.8 * max(forwards_a, forwards_b)               // forwards lobe (max'ing them is completely nonsensical but it looks nice)
		+ 0.2 * henyey_greenstein_phase(cos_theta, -0.2); // backwards lobe
}

float clouds_phase_multi(float cos_theta, float3 g) { // Multiple scattering phase function
	return 0.65 * henyey_greenstein_phase(cos_theta, g.x)  // forwards lobe
		+ 0.10 * henyey_greenstein_phase(cos_theta, g.y)  // forwards peak
		+ 0.25 * henyey_greenstein_phase(cos_theta, -g.z); // backwards lobe
}

float clouds_powder_effect(float density, float cos_theta) {
	float powder = pi * density / (density + 0.15);
	powder = mix(powder, 1.0, 0.8 * sqr(cos_theta * 0.5 + 0.5));

	return powder;
}

float3 clouds_aerial_perspective(
	float3 clouds_scattering,
	float clouds_transmittance,
	float3 ray_origin,
	float3 ray_end,
	float3 ray_dir,
	float3 clear_sky
) {
	float3 air_transmittance;

#if CLOUDS_AERIAL_PERSPECTIVE_BOOST != 0
	ray_end = mix(ray_origin, ray_end, float(1 << CLOUDS_AERIAL_PERSPECTIVE_BOOST));
#endif

	if (length_squared(ray_origin) < length_squared(ray_end)) {
		float3 trans_0 = atmosphere_transmittance(ray_origin, ray_dir);
		float3 trans_1 = atmosphere_transmittance(ray_end, ray_dir);

		air_transmittance = clamp01(trans_0 / trans_1);
	}
	else {
		float3 trans_0 =	(ray_origin, -ray_dir);
		float3 trans_1 = atmosphere_transmittance(ray_end, -ray_dir);

		air_transmittance = clamp01(trans_1 / trans_0);
	}

	// Blend to rain color during rain
	//clear_sky = mix(clear_sky, (float3)0.0 * rcp(tau), 0.5f * mix(1.0, 0.9, time_sunrise + time_sunset));
	air_transmittance = mix(air_transmittance, float3(air_transmittance.x, 0.0, 0.0), 0.8);

	return mix((1.0 - clouds_transmittance) * clear_sky, clouds_scattering, air_transmittance);
}

CloudsResult blend_layers(CloudsResult old, CloudsResult new1) {
	bool new_in_front = new1.apparent_distance < old.apparent_distance;

	float4 scattering_behind = new_in_front ? old.scattering : new1.scattering;
	float4 scattering_in_front = new_in_front ? new1.scattering : old.scattering;
	float transmittance_in_front = new_in_front ? new1.transmittance : old.transmittance;

	return CreateCloudsResult(
		scattering_in_front + transmittance_in_front * scattering_behind,
		old.transmittance * new1.transmittance,
		min(old.apparent_distance, new1.apparent_distance)
	);
}


#endif
