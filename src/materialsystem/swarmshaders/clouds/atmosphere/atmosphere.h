#if !defined INCLUDE_SKY_ATMOSPHERE
#define INCLUDE_SKY_ATMOSPHERE

#define half_pi PI / 2
#define half_pi pi / 2

float rcp(float x) { return 1.0 / x; }

float invSqrt_at(float v) {
	float y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float2 invSqrt_at(float2 v) {
	float2 y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float3 invSqrt_at(float3 v) {
	float3 y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

float4 invSqrt_at(float4 v) {
	float4 y = 1.0f / (sqrt(2.0f) * sqrt(v));
	y = (y * y + 1.0f / v) / (2.0f * y);
	y = (y * y + 1.0f / v) / (2.0f * y);
	return y;
}

// These have to be macros so that they can be used by constant expressions
#define cone_angle_to_solid_angle(theta) (tau * (1.0 - cos(theta)))
#define solid_angle_to_cone_angle(theta) acos(1.0 - (theta) / tau)

const float3 sunlight_color = float3(1.051, 0.985, 0.940); // Color of sunlight in space, obtained from AM0 solar irradiance spectrum from https://www.nrel.gov/grid/solar-resource/spectra-astm-e490.html using the CIE (2006) 2-deg LMS cone fundamentals

const float sun_angular_radius = 0.533 * degree;
const float moon_angular_radius = 0.52 * degree;

const float2 transmittance_res = float2(/* mu */ 256, /* r */ 64);
const float3 scattering_res = float3(/* nu */ 16, /* mu */ 64, /* mu_s */ 32);

const float min_mu_s = -0.35;

// Atmosphere boundaries

const float planet_radius = 6371e3; // m

#define atmosphere_inner_radius planet_radius - 1e3; // m
#define atmosphere_outer_radius = planet_radius + 110e3; // m

#define planet_radius_sq planet_radius * planet_radius;
#define atmosphere_thickness atmosphere_outer_radius - atmosphere_inner_radius;
#define atmosphere_inner_radius_sq atmosphere_inner_radius * atmosphere_inner_radius;
#define atmosphere_outer_radius_sq atmosphere_outer_radius * atmosphere_outer_radius;

// Atmosphere coefficients

const float air_mie_albedo = 0.9;
const float air_mie_energy_parameter = 3000.0; // Energy parameter for the Klein-Nishina phase function
const float air_mie_g = 0.77;    // Anisotropy parameter for Henyey-Greenstein phase function

const float2 air_scale_heights = float2(8.4e3, 1.25e3); // m

// Coefficients for Rec. 709 primaries transformed to Rec. 2020
const float3 air_rayleigh_coefficient = float3(8.059375432e-06, 1.671209429e-05, 4.080133294e-05); //* rec709_to_rec2020;
const float3 air_mie_coefficient = float3(1.666442358e-06, 1.812685127e-06, 1.958927896e-06); //* rec709_to_rec2020;
const float3 air_ozone_coefficient = float3(8.304280072e-07, 1.314911970e-06, 5.440679729e-08); //* rec709_to_rec2020;

#define air_mie_albedo_mie_coef air_mie_albedo * air_mie_coefficient

#define AIR_RAYLEIGH_COEFFICIENT 0.005
#define AIR_MIE_ALBEDO 0.95
#define AIR_MIE_COEFFICIENT 0.005
#define AIR_OZONE_COEFFICIENT 0.0005

float2x3 air_scattering_coefficients = float2x3(
	  float3(AIR_RAYLEIGH_COEFFICIENT, 0, 0),
	  float3(AIR_MIE_ALBEDO * AIR_MIE_COEFFICIENT, 0, 0)
);

float3x3 air_extinction_coefficients = float3x3(
	  float3(AIR_RAYLEIGH_COEFFICIENT, 0, 0),
	  float3(AIR_MIE_COEFFICIENT, 0, 0),
	  float3(AIR_OZONE_COEFFICIENT, 0, 0)
);

uniform float atmosphere_saturation_boost_amount;

#if defined ATMOSPHERE_TRANSMITTANCE_LUT
float3 atmosphere_transmittance(float mu, float r) {
	if (intersect_sphere(mu, r, planet_radius).x >= 0.0) return float3(0.0);

	float2 uv = atmosphere_transmittance_uv(mu, r);
	return texture(ATMOSPHERE_TRANSMITTANCE_LUT, uv).rgb;
}
#else
// Source: http://www.thetenthplanet.de/archives/4519
float chapman_function_approx(float x, float cos_theta) {
	float c = sqrt(half_pi * x);

	if (cos_theta >= 0.0) { // => theta <= 90 deg
		return c / ((c - 1.0) * cos_theta + 1.0);
	}
	else {
		float sin_theta = sqrt(clamp01(1.0 - sqr(cos_theta)));
		return c / ((c - 1.0) * cos_theta - 1.0) + 2.0 * c * exp(x - x * sin_theta) * sqrt(sin_theta);
	}
}

float2 intersect_sphere(float3 origin, float3 dir, float radius) {
	float b = dot(origin, dir);
	float c = dot(origin, origin) - radius * radius;
	float discriminant = b * b - c;

	if (discriminant < 0.0) return float2(-1.0, -1.0);

	float sqrtDisc = sqrt(discriminant);
	return float2(-b - sqrtDisc, -b + sqrtDisc);
}

float2 intersect_spherical_shell(float3 origin, float3 dir, float inner_radius, float outer_radius) {
	float2 inner = intersect_sphere(origin, dir, inner_radius);
	float2 outer = intersect_sphere(origin, dir, outer_radius);

	float t_near = (inner.x > 0.0) ? inner.x : outer.x;
	float t_far = outer.y;

	return float2(t_near, t_far);
}

float3 atmosphere_transmittance(float mu, float r) {
	if (intersect_sphere(mu, max(r, planet_radius + 10.0), planet_radius).x >= 0.0) return float3(0, 0, 0);

	// Rayleigh and mie density at r
	const float2 rcp_scale_heights = rcp(air_scale_heights);
	const float2 scaled_planet_radius = planet_radius * rcp_scale_heights;
	float2 density = exp(r * -rcp_scale_heights + scaled_planet_radius);

	// Estimate airmass along ray using chapman function approximation
	float2 airmass = air_scale_heights * density;
	airmass.x *= chapman_function_approx(r * rcp_scale_heights.x, mu);
	airmass.y *= chapman_function_approx(r * rcp_scale_heights.y, mu);

	// Approximate ozone density as rayleigh density
	float3 extinction = float3(
		air_extinction_coefficients[0][0],  // Rayleigh
		air_extinction_coefficients[1][0],  // Mie
		air_extinction_coefficients[2][0]   // Ozone
	);

	float3 v = -extinction * airmass.xyx;
	float3 result = exp(v);

	return clamp01(result);
}
#endif

float3 atmosphere_transmittance(float3 ray_origin, float3 ray_dir) {
	float r_sq = dot(ray_origin, ray_origin);
	float rcp_r = invSqrt_at(r_sq);
	float mu = dot(ray_origin, ray_dir) * rcp_r;
	float r = r_sq * rcp_r;

	return atmosphere_transmittance(mu, r);
}
#endif