
// Constants and Definitions
// ====================================

#define eps 0.0001
#define degree 0.0174533
#define rcp_pi 0.31831
#define isotropic_phase 0.07957747
#define PI 3.1415926


// ====================================
// Cloud System Constants
// ====================================

static const float clouds_cumulus_radius = 6371000.0 + 1500.0;
static const float clouds_cumulus_top_radius = 6371000.0 + 3500.0;
static const float clouds_cumulus_thickness = 2000.0;
static const float clouds_cumulus_congestus_distance = 50000.0;
static const float clouds_altocumulus_radius = 6371000.0 + 4000.0;
static const float planet_radius = 6371000.0;
static float world_age = 0.0;
static float3 cameraPosition = float3(0, 0, 0);
static const float CLOUDS_SCALE = 1.0;
static const float CLOUDS_CUMULUS_WIND_ANGLE = 45.0;
static const float CLOUDS_CUMULUS_WIND_SPEED = 10.0;
static const int CLOUDS_CUMULUS_PRIMARY_STEPS_H = 16;
static const int CLOUDS_CUMULUS_PRIMARY_STEPS_Z = 24;
static const int CLOUDS_CUMULUS_LIGHTING_STEPS = 3;
static const int CLOUDS_CUMULUS_AMBIENT_STEPS = 2;

static const float CLOUDS_CUMULUS_SIZE = 1.0;

// Global cloud parameters
static CloudParams clouds_params;
static float3 sun_dir;
static float3 moon_dir;
static float3 sun_color;
static float3 moon_color;
static float3 sky_color;
static float3 sunlight_color;

// ====================================
// Utility Functions
// ====================================

float clamp01(float x) { return saturate(x); }
float2 clamp01(float2 x) { return saturate(x); }
float3 clamp01(float3 x) { return saturate(x); }
float max0(float x) { return max(0.0, x); }
float sqr(float x) { return x * x; }
float rcp(float x) { return 1.0 / x; }
float dampen(float x) { return sqrt(x); }

float linear_step(float edge0, float edge1, float x) {
	return saturate((x - edge0) / (edge1 - edge0));
}

float lift(float x, float power) {
	return pow(saturate(x), power);
}

float cubic_smooth(float x) {
	return x * x * (3.0 - 2.0 * x);
}

// ====================================
// Cloud Result Structure
// ====================================

struct CloudsResult {
	float4 scattering; // w = ambient scattering, for lightning flashes
	float transmittance;
	float apparent_distance;
};

static const CloudsResult clouds_not_hit = {
	float4(0.0, 0.0, 0.0, 0.0),
	1.0,
	1e5
};

CloudsResult blend_layers(CloudsResult back_layer, CloudsResult front_layer) {
	bool front_in_front = front_layer.apparent_distance < back_layer.apparent_distance;
	float4 scattering_behind = front_in_front ? back_layer.scattering : front_layer.scattering;
	float4 scattering_in_front = front_in_front ? front_layer.scattering : back_layer.scattering;
	float transmittance_in_front = front_in_front ? front_layer.transmittance : back_layer.transmittance;

	CloudsResult result;
	result.scattering = scattering_in_front + transmittance_in_front * scattering_behind;
	result.transmittance = back_layer.transmittance * front_layer.transmittance;
	result.apparent_distance = min(back_layer.apparent_distance, front_layer.apparent_distance);
	return result;
}


// Distance covered by the cumulus coverage map on each axis (m^2)
const float clouds_cumulus_coverage_map_scale = 1.5e5;

const float clouds_coverage_map_distortion = 0.8;

float clouds_cumulus_local_coverage(float2 pos) {
	const float wind_angle = CLOUDS_CUMULUS_WIND_ANGLE * degree;
	const float2 wind_velocity = CLOUDS_CUMULUS_WIND_SPEED * float2(cos(wind_angle), sin(wind_angle));

	pos += cameraPosition.xz * CLOUDS_SCALE;
	pos += wind_velocity * world_age;

	// Sample noise
	float2 p1 = (0.000002 / CLOUDS_CUMULUS_SIZE) * pos;
	float2 p2 = (0.000027 / CLOUDS_CUMULUS_SIZE) * pos;
	float2 noise = float2(
		tex2D(noisetex, p1).x, // cloud coverage
		tex2D(noisetex, p2).w  // cloud shape
	);

	// Compute cumulus coverage
	float coverage_cu = 0.0, coverage_st = 0.0;

	if (clouds_params.l0_cumulus_stratus_blend < 1.0 - eps) {
		coverage_cu = lerp(clouds_params.l0_coverage.x, clouds_params.l0_coverage.y, noise.x);
		coverage_cu = linear_step(1.0 - coverage_cu, 1.0, noise.y);
	}

	// Compute stratus coverage
	if (clouds_params.l0_cumulus_stratus_blend > eps) {
		coverage_st = cubic_smooth(
			linear_step(
				0.9 - clouds_params.l0_coverage.x,
				1.0,
				2.0 * noise.x * clouds_params.l0_coverage.y
			)
		);
		coverage_st = 0.5 * coverage_st + 1.0 * coverage_st * linear_step(0.3, 0.6, noise.y);
		coverage_st = coverage_st / (coverage_st + 1.0);
	}

	return lerp(coverage_cu, coverage_st, clouds_params.l0_cumulus_stratus_blend);
}

float2 project_clouds_cumulus_coverage_map(float3 pos) {
	// Scale position
	float2 coverage_map_uv = pos.xz * rcp(0.5 * clouds_cumulus_coverage_map_scale);

	// Distort so that clouds closer to the player are higher resolution
	coverage_map_uv /= clouds_coverage_map_distortion * length(coverage_map_uv) + (1.0 - clouds_coverage_map_distortion);

	// Scale to [0, 1]
	coverage_map_uv = coverage_map_uv * 0.5 + 0.5;

	return coverage_map_uv;
}

float render_clouds_cumulus_coverage_map(float2 uv) {
	// Get clouds position 
	float2 pos = uv * 2.0 - 1.0;
	pos *= (1.0 - clouds_coverage_map_distortion) / (1.0 - length(pos) * clouds_coverage_map_distortion);
	pos *= 0.5 * clouds_cumulus_coverage_map_scale;

	return clouds_cumulus_local_coverage(pos);
}


bool clouds_early_exit(
	CloudsResult result,
	float r,
	float layer_radius
) {
	bool has_congestus = clouds_params.cumulus_congestus_blend > eps;

	return result.transmittance < 1e-3 && r < layer_radius
		&& (result.apparent_distance < clouds_cumulus_congestus_distance || !has_congestus);
}

// 1st layer: volumetric cumulus/stratocumulus/stratus clouds

// altitude_fraction := 0 at the bottom of the cloud layer and 1 at the top
float clouds_cumulus_altitude_shaping(float density, float altitude_fraction) {
	// Stratus shapes
	if (clouds_params.l0_cumulus_stratus_blend > eps) {
		density = lerp(
			density,
			clamp01(
				density * dampen(clamp01(2.0 * altitude_fraction)
					* linear_step(0.0, 0.1, altitude_fraction)
					* linear_step(0.0, 0.6, 1.0 - altitude_fraction))
			),
			clouds_params.l0_cumulus_stratus_blend
		);
	}

	// Carve egg shape
	density -= smoothstep(0.2, 1.0, altitude_fraction)
		* (0.6 - 0.3 * clouds_params.l0_cumulus_stratus_blend);

	// Reduce density at the bottom of the cloud
	density *= smoothstep(0.0, 0.2, altitude_fraction);

	return density;
}

float clouds_cumulus_density(float3 pos) {
	float r = length(pos);

#if defined CLOUDS_USE_LOCAL_COVERAGE_MAP
	// Get local coverage from precomputed coverage map - saves 1 sample and some maths
	float2 coverage_map_uv = project_clouds_cumulus_coverage_map(pos);

	if (
		r < clouds_cumulus_radius || r > clouds_cumulus_top_radius
		|| clamp01(coverage_map_uv) != coverage_map_uv
		) {
		return 0.0;
	}

	float density = texture(colortex8, coverage_map_uv).z;
#else 
	if (r < clouds_cumulus_radius || r > clouds_cumulus_top_radius) {
		return 0.0;
	}

	float density = clouds_cumulus_local_coverage(pos.xz);
#endif

	// Altitude shaping 

	float altitude_fraction = (r - clouds_cumulus_radius) * clouds_params.l0_altitude_scale;

	density = clouds_cumulus_altitude_shaping(
		density,
		altitude_fraction
	);

	if (density < eps) return 0.0;

	const float wind_angle = CLOUDS_CUMULUS_WIND_ANGLE * degree;
	const float2 wind_velocity = CLOUDS_CUMULUS_WIND_SPEED * float2(cos(wind_angle), sin(wind_angle));

	float3 wind = float3(wind_velocity * world_age, 0.0).xzy;

	pos.xz += cameraPosition.xz * CLOUDS_SCALE + wind.xz;

	// 3D worley noise for detail
	float worley_0 = tex3Dlod(SAMPLER_WORLEY_BUBBLY, float4((pos + 0.2 * wind) * 0.0009, 0)).x;
	float worley_1 = tex3Dlod(SAMPLER_WORLEY_SWIRLEY, float4((pos + 0.4 * wind) * 0.005, 0)).x;
	//float worley_0 = worley_noise_3d((pos + 0.2 * wind), 0.0009);
	//float worley_1 = worley_noise_3d((pos + 0.4 * wind), 0.005);

	float detail_fade = 0.20 * smoothstep(0.85, 1.0, 1.0 - altitude_fraction)
		- 0.35 * smoothstep(0.05, 0.5, altitude_fraction) + 0.6;

	density -= clouds_params.l0_detail_weights.x * sqr(worley_0) * dampen(clamp01(1.0 - density));
	density -= clouds_params.l0_detail_weights.y * sqr(worley_1) * dampen(clamp01(1.0 - density)) * detail_fade;

	// Adjust density so that the clouds are wispy at the bottom and hard at the top
	density = max0(density);
	density = lift(
		density,
		lerp(
			clouds_params.l0_edge_sharpening.x,
			clouds_params.l0_edge_sharpening.y,
			altitude_fraction
		)
	);
	density *= 0.1 + 0.9 * smoothstep(0.2, 0.7, altitude_fraction);

	return density;
}

float clouds_cumulus_optical_depth(
	float3 ray_origin,
	float3 ray_dir,
	float dither,
	const int step_count
) {
	const float step_growth = 2.0;

	float step_length = 0.1 * clouds_cumulus_thickness / float(step_count); // m

	float3 ray_pos = ray_origin;
	float4 ray_step = float4(ray_dir, 1.0) * step_length;

	float optical_depth = 0.0;

	[unroll(8)]
	for (int i = 0; i < step_count; ++i, ray_pos += ray_step.xyz) {
		ray_step *= step_growth;
		optical_depth += clouds_cumulus_density(ray_pos + ray_step.xyz * dither) * ray_step.w;
	}

	return optical_depth;
}

float clouds_phase_single(float cos_theta) {
	float g = 0.6;
	float g2 = g * g;
	return (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cos_theta, 1.5) / (4.0 * PI);
}

float clouds_phase_multi(float cos_theta, float3 phase_g) {
	float phase = 0.0;
	phase += (1.0 - phase_g.x * phase_g.x) / pow(1.0 + phase_g.x * phase_g.x - 2.0 * phase_g.x * cos_theta, 1.5);
	phase += (1.0 - phase_g.y * phase_g.y) / pow(1.0 + phase_g.y * phase_g.y - 2.0 * phase_g.y * cos_theta, 1.5);
	phase += (1.0 - phase_g.z * phase_g.z) / pow(1.0 + phase_g.z * phase_g.z - 2.0 * phase_g.z * cos_theta, 1.5);
	return phase / (12.0 * PI);
}

float clouds_powder_effect(float density, float cos_theta) {
	float powder = 1.0 - exp(-density * 2.0);
	return lerp(powder, 1.0, 0.5 + 0.5 * cos_theta);
}

float2 clouds_cumulus_scattering(
	float density,
	float light_optical_depth,
	float sky_optical_depth,
	float ground_optical_depth,
	float step_transmittance,
	float cos_theta,
	float bounced_light
) {
	float2 scattering = float2(0.0, 0.0);

	float scatter_amount = clouds_params.l0_scattering_coeff;
	float extinct_amount = clouds_params.l0_extinction_coeff;

	float scattering_integral_times_density = (1.0 - step_transmittance) / clouds_params.l0_extinction_coeff;

	float powder_effect = clouds_powder_effect(density + density * clouds_params.l0_cumulus_stratus_blend, cos_theta);
	float scattering_falloff = 0.55 * lerp(lift(clamp01(clouds_params.l0_scattering_coeff / 0.1), 0.33), 1.0, cos_theta * 0.5 + 0.5);

	float phase = clouds_phase_single(cos_theta);
	float3 phase_g = pow(float3(0.6, 0.9, 0.3), float3(1.0 + light_optical_depth, 0.0, 0.0));

	[unroll(8)]
	for (int i = 0; i < 8; ++i) {
		scattering.x += scatter_amount * exp(-extinct_amount * light_optical_depth) * phase * (1.0 - 0.5 * clouds_params.l0_shadow);
		scattering.x += scatter_amount * exp(-extinct_amount * ground_optical_depth) * isotropic_phase * bounced_light;
		scattering.x += scatter_amount * exp(-extinct_amount * sky_optical_depth) * isotropic_phase * clouds_params.l0_shadow * 0.5; // fake bounced lighting from the layer above
		scattering.y += scatter_amount * exp(-extinct_amount * sky_optical_depth) * isotropic_phase;

		scatter_amount *= scattering_falloff * powder_effect;
		extinct_amount *= 0.4;
		phase_g *= 0.8;

		powder_effect = lerp(powder_effect, sqrt(powder_effect), 0.5);

		phase = clouds_phase_multi(cos_theta, phase_g);
	}

	return scattering * scattering_integral_times_density;
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

float2 hash2(float2 p) {
	p = float2(dot(p, float2(127.1, 311.7)),
		dot(p, float2(269.5, 183.3)));
	return frac(sin(p) * 43758.5453123);
}

float2 hash2(float3 p) {
	return hash2(p.xy + p.z);
}

float3 atmosphere_transmittance(float3 pos, float3 dir) {
	return float3(1.0, 1.0, 1.0);
}

float3 atmosphere_post_processing(float3 color) {
	return color;
}

float3 clouds_aerial_perspective(float3 scattering, float transmittance, float3 viewer_pos, float3 ray_origin, float3 ray_dir, float3 clear_sky) {
	return scattering;
}

CloudsResult draw_cumulus_clouds(
	float3 air_viewer_pos,
	float3 ray_dir,
	float3 clear_sky,
	float distance_to_terrain,
	float dither
) {
	// ---------------------
	//   Raymarching Setup
	// ---------------------

#if defined PROGRAM_DEFERRED0
	const int  primary_steps_horizon = CLOUDS_CUMULUS_PRIMARY_STEPS_H / 2;
	const int  primary_steps_zenith = CLOUDS_CUMULUS_PRIMARY_STEPS_Z / 2;
#else
	const int  primary_steps_horizon = CLOUDS_CUMULUS_PRIMARY_STEPS_H;
	const int  primary_steps_zenith = CLOUDS_CUMULUS_PRIMARY_STEPS_Z;
#endif
	const int  lighting_steps = CLOUDS_CUMULUS_LIGHTING_STEPS;
	const int  ambient_steps = CLOUDS_CUMULUS_AMBIENT_STEPS;
	const float max_ray_length = 2e4;
	const float min_transmittance = 0.075;
	const float planet_albedo = 0.4;
	const float3  sky_dir = float3(0.0, 1.0, 0.0);

	// Early exit if coverage is 0
	if (clouds_params.l0_coverage.y < eps) { return clouds_not_hit; }

	int primary_steps = int(lerp(
		primary_steps_horizon,
		primary_steps_zenith,
		abs(ray_dir.y)
	));

	float r = length(air_viewer_pos);

	float2 dists = intersect_spherical_shell(
		air_viewer_pos,
		ray_dir,
		clouds_cumulus_radius - planet_radius,  // Make relative to viewer
		clouds_cumulus_top_radius - planet_radius
	);
	bool planet_intersected = intersect_sphere(air_viewer_pos, ray_dir, min(r - 10.0, planet_radius)).y >= 0.0;
	bool terrain_intersected = distance_to_terrain >= 0.0 && r < clouds_cumulus_radius && distance_to_terrain < dists.x;

	if (dists.y < 0.0                                   // volume not intersected
		|| planet_intersected && r < clouds_cumulus_radius // planet blocking clouds
		|| terrain_intersected                             // terrain blocking clouds
		) {
		return clouds_not_hit;
	}

	float ray_length = (distance_to_terrain >= 0.0) ? distance_to_terrain : dists.y;
	ray_length = clamp(ray_length - dists.x, 0.0, max_ray_length);
	float step_length = ray_length * rcp(float(primary_steps));

	float3 ray_step = ray_dir * step_length;
	float3 ray_origin = air_viewer_pos + ray_dir * (dists.x + step_length * dither);

	float transmittance = 1.0;

	float distance_sum = 0.0;
	float distance_weight_sum = 0.0;

	// ------------------
	//   Lighting Setup
	// ------------------

	bool  moonlit = sun_dir.y < -0.04;
	float3  light_dir = moonlit ? moon_dir : sun_dir;
	float cos_theta = dot(ray_dir, light_dir);
	float bounced_light = planet_albedo * light_dir.y * rcp_pi;

	float2 scattering = float2(0.0, 0.0);

	// --------------------
	//   Raymarching Loop
	// --------------------
	[unroll(8)]
	for (int i = 0; i < primary_steps; ++i) {
		if (transmittance < min_transmittance) break;

		float3 ray_pos = ray_origin + ray_step * i;

		float altitude_fraction = (length(ray_pos) - (clouds_cumulus_radius - planet_radius)) * rcp(clouds_cumulus_thickness);

		float density = clouds_cumulus_density(ray_pos);

		if (density < eps) continue;

		// Fade away in the distance to hide the max ray length cutoff
		float distance_to_sample = distance(ray_origin, ray_pos);
		density *= smoothstep(1.0, 0.95, distance_to_sample * rcp(max_ray_length));

#if defined CLOUDS_USE_LOCAL_COVERAGE_MAP
		// Fade away at the edges of the local coverage map
		density *= smoothstep(1.0, 0.9, length(ray_pos.xz) * rcp(0.5 * clouds_cumulus_coverage_map_scale));
#endif

		float step_optical_depth = density * clouds_params.l0_extinction_coeff * step_length;
		float step_transmittance = exp(-step_optical_depth);

#if defined PROGRAM_DEFERRED0
		float2 hash = float2(0.0);
#else
		float2 hash = hash2(frac(ray_pos)); // used to dither the light rays
#endif

		float light_optical_depth = clouds_cumulus_optical_depth(ray_pos, light_dir, hash.x, lighting_steps);
		float sky_optical_depth = clouds_cumulus_optical_depth(ray_pos, sky_dir, hash.y, ambient_steps);
		// guess optical depth to the ground using altitude fraction and density from this sample
		float ground_optical_depth = lerp(
			density,
			1.0,
			clamp01(altitude_fraction * 2.0 - 1.0)
		) * altitude_fraction * clouds_cumulus_thickness;

		scattering += clouds_cumulus_scattering(
			density,
			light_optical_depth,
			sky_optical_depth,
			ground_optical_depth,
			step_transmittance,
			cos_theta,
			bounced_light
		) * transmittance;

		transmittance *= step_transmittance;

		// Update distance to cloud
		distance_sum += distance_to_sample * density;
		distance_weight_sum += density;
	}

	// Get main light color for this layer
	float3 light_color = sunlight_color * atmosphere_transmittance(ray_origin, light_dir);
	light_color = atmosphere_post_processing(light_color);
	light_color *= moonlit ? moon_color : sun_color;

	// Remap the transmittance so that min_transmittance is 0
	float clouds_transmittance = linear_step(min_transmittance, 1.0, transmittance);

	float3 clouds_scattering = scattering.x * light_color + scattering.y * sky_color;
	clouds_scattering = clouds_aerial_perspective(clouds_scattering, clouds_transmittance, air_viewer_pos, ray_origin, ray_dir, clear_sky);

	float apparent_distance = (distance_weight_sum == 0.0)
		? 1e6
		: (distance_sum / distance_weight_sum) + distance(air_viewer_pos, ray_origin);

	float4 global_scattering = float4(clouds_scattering, scattering.y);

	CloudsResult result;
	result.scattering = global_scattering;
	result.transmittance = clouds_transmittance;
	result.apparent_distance = apparent_distance;

	return result;
}

CloudsResult draw_clouds(
	float3 air_viewer_pos,
	float3 ray_dir,
	float3 clear_sky,
	float distance_to_terrain,
	float dither
) {
	CloudsResult result = clouds_not_hit;
	float r = length(air_viewer_pos);


	result = draw_cumulus_clouds(air_viewer_pos, ray_dir, clear_sky, distance_to_terrain, dither);
	if (clouds_early_exit(result, r, clouds_cumulus_radius)) {
		return result;
	}

#ifdef CLOUDS_ALTOCUMULUS
	CloudsResult result_ac = draw_altocumulus_clouds(air_viewer_pos, ray_dir, clear_sky, distance_to_terrain, dither);
	result = blend_layers(result, result_ac);
	if (clouds_early_exit(result, r, clouds_altocumulus_radius)) {
		return result;
	}
#endif

#ifdef CLOUDS_CUMULUS_CONGESTUS
	if (clouds_params.cumulus_congestus_blend > eps) {
		CloudsResult result_cu_con = draw_cumulus_congestus_clouds(air_viewer_pos, ray_dir, clear_sky, distance_to_terrain, dither);

		// fade existing clouds into congestus
		float distance_fade = lerp(
			1.0,
			result_cu_con.transmittance,
			linear_step(
				0.75,
				1.0,
				result.apparent_distance * rcp(clouds_cumulus_congestus_distance)
			)
		);
		result.scattering *= distance_fade;
		result.transmittance += (1.0 - result.transmittance) * (1.0 - distance_fade);
		result.apparent_distance = lerp(result_cu_con.apparent_distance, result.apparent_distance, distance_fade);

		result = blend_layers(result, result_cu_con);
		if (result.transmittance < 1e-3) return result;
	}
#endif

#ifdef CLOUDS_CIRRUS
	CloudsResult result_ci = draw_cirrus_clouds(air_viewer_pos, ray_dir, clear_sky, distance_to_terrain, dither);
	result = blend_layers(result, result_ci);
	if (result.transmittance < 1e-3) return result;
#endif

#ifdef CLOUDS_NOCTILUCENT
	float4 result_nlc = draw_noctilucent_clouds(air_viewer_pos, ray_dir, clear_sky);
	result.scattering.rgb += result_nlc.xyz * result.transmittance;
	result.transmittance *= result_nlc.w;
#endif

	return result;
}

