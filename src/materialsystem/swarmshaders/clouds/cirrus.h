#if !defined INCLUDE_SKY_CLOUDS_CIRRUS
#define INCLUDE_SKY_CLOUDS_CIRRUS

// 3rd layer: planar cirrus/cirrocumulus clouds

float clouds_cirrus_density(
	float2 coord,
	float altitude_fraction
) {

	coord.y = 1.0 - coord.y;

	//const float wind_angle = clouds_cirrus_wind_angle * degree;
	//const float2 wind_velocity = clouds_cirrus_wind_speed * float2(cos(wind_angle), sin(wind_angle));


	coord = coord + cameraPosition.xz * CLOUDS_SCALE;
	//coord = coord + wind_velocity * world_age;

	//fx12!!

	float2 curl = curl2D(0.00002 * coord) * 0.5
	          + curl2D(0.00004 * coord) * 0.25
			  + curl2D(0.00008 * coord) * 0.125;

	float height_shaping = 1.0 - abs(1.0 - 2.0 * altitude_fraction);

	// Cirrus 

	//fx12!!

	float density_cirrus = 0.7 * tex2D(noisetex, (0.000001 / clouds_cirrus_size) * coord + (0.004 * clouds_cirrus_curl_strength) * curl).x
	                     + 0.3 * tex2D(noisetex, (0.000008 / clouds_cirrus_size) * coord + (0.008 * clouds_cirrus_curl_strength) * curl).x;

	//float adjusted_amount = CloudsParameters.cirrus_amount;

	density_cirrus = linear_step(
		0.7/* - adjusted_amount*/,
		1.0, 
		density_cirrus
	);

	float2 detail_coord = coord;

	float detail_amplitude = 0.2;
	float detail_frequency = 0.00002;
	float curl_strength    = 0.1 * clouds_cirrus_curl_strength;

	for (int i = 0; i < 4; ++i) {
		//fx12!!
		float detail = tex2D(noisetex, detail_coord * detail_frequency + curl * curl_strength).x;

		density_cirrus -= detail * detail_amplitude;

		detail_amplitude *= 0.6;
		detail_frequency *= 2.0;
		curl_strength *= 4.0;

		detail_coord += 0.3/* * wind_velocity*/ * world_age;
	}
	//fx12!!
	density_cirrus = mix(1.0, 0.75, day_factor) * cube(max0(density_cirrus)) * sqr(height_shaping) * clouds_cirrus_density_static;

	// Cirrocumulus 
	//fx12!!
	float coverage = tex2D(noisetex, (0.0000026 / clouds_cirrocumulus_size) * coord + 0.25).w;
	coverage = 5.0 * linear_step(
		0.25, 
		0.9, 
		0.7887/*CloudsParameters.cirrocumulus_amount*/ * coverage
	);
	//fx12!!
	float density_cirrocumulus = dampen(tex2D(noisetex, (0.000025 * rcp(clouds_cirrocumulus_size)) * coord + (0.033 * clouds_cirrocumulus_curl_strength) * curl).w);
	density_cirrocumulus = linear_step(1.0 - coverage, 1.0, density_cirrocumulus);
	//fx12!!
	float2 curl_cc = curl2D(0.001 * coord);
	//fx12!!
	density_cirrocumulus -= sqr(tex2D(noisetex, coord * 0.00005 + (0.003 * clouds_cirrocumulus_curl_strength) * curl_cc).y) * (clouds_cirrocumulus_detail_strength * 1.0);
	density_cirrocumulus -= sqr(tex2D(noisetex, coord * 0.0002 + (0.007 * clouds_cirrocumulus_curl_strength) * curl_cc).y) * (clouds_cirrocumulus_detail_strength * 0.5);
	density_cirrocumulus -= sqr(tex2D(noisetex, coord * 0.0008 + (0.03 * clouds_cirrocumulus_curl_strength) * curl_cc).y) * (clouds_cirrocumulus_detail_strength * 0.1);

	density_cirrocumulus  = max0(density_cirrocumulus);

	density_cirrocumulus = 0.25 * pow4(max0(density_cirrocumulus)) * height_shaping * clouds_cirrocumulus_density;

	return density_cirrus + density_cirrocumulus;
}

float clouds_cirrus_optical_depth(
	float3 ray_origin,
	float3 ray_dir,	//sun dir
	float dither
) {
	const uint step_count      = clouds_cirrus_lighting_steps;
	const float max_ray_length = 1e3;
	const float step_growth    = 1.5;

	// Assuming ray_origin is between inner and outer boundary, find distance to closest layer
	// boundary
	float2 inner_sphere = intersect_sphere(ray_origin, ray_dir, clouds_cirrus_radius - 0.5 * clouds_cirrus_thickness);
	float2 outer_sphere = intersect_sphere(ray_origin, ray_dir, clouds_cirrus_radius + 0.5 * clouds_cirrus_thickness);

	float ray_length = (inner_sphere.y >= 0.0) ? inner_sphere.x : outer_sphere.y;
	      ray_length = min(ray_length, max_ray_length);

	// Find initial step length a so that Σ(ar^i) = rayLength
	float step_coeff = (step_growth - 1.0) / (pow(step_growth, float(step_count)) - 1.0) / step_growth;
	float step_length = ray_length * step_coeff;

	float3 ray_pos  = ray_origin;
	float4 ray_step = float4(ray_dir, 1.0) * step_length;

	float optical_depth = 0.0;

	for (uint i = 0u; i < step_count; ++i, ray_pos += ray_step.xyz) {
		ray_step *= step_growth;

		float3 dithered_pos = ray_pos + ray_step.xyz * dither;

		float r = length(dithered_pos);
		float altitude_fraction = (r - clouds_cirrus_radius) * rcp(clouds_cirrus_thickness) + 0.5;
		if (clamp01(altitude_fraction) != altitude_fraction) continue;

		float3 sphere_pos = dithered_pos * (clouds_cirrus_radius / r);

		optical_depth += clouds_cirrus_density(sphere_pos.xz, altitude_fraction) * ray_step.w;
	}

	return optical_depth;
}

float2 clouds_cirrus_scattering(
	float density,
	float3 sun_dir,
	float view_transmittance,
	float light_optical_depth,
	float cos_theta
) {
	float2 scattering = (float2)0.0;

	float phase = clouds_phase_single(cos_theta);
	float3 phase_g = float3(0.6, 0.9, 0.3);

	float powder_effect = 8.0 * (1.0 - exp(-40.0 * density));
	      powder_effect = mix(powder_effect, 1.0, pow1d5(cos_theta * 0.5 + 0.5));

	float scatter_amount = clouds_cirrus_scattering_coeff;
	float extinct_amount = clouds_cirrus_extinction_coeff * (1.0 + 0.5 * max0(smoothstep(0.0, 0.15, abs(sun_dir.y)) - smoothstep(0.5, 0.7, 0.7/*CloudsParameters.cirrus_amount*/)));

	for (uint i = 0u; i < 4u; ++i) {
		scattering.x += scatter_amount * exp(-extinct_amount * light_optical_depth) * phase * powder_effect; // direct light
		scattering.y += scatter_amount * exp(-0.33 * clouds_cirrus_thickness * extinct_amount * density) * isotropic_phase; // sky light

		scatter_amount *= 0.5;
		extinct_amount *= 0.25;
		phase_g *= 0.5;

		powder_effect = mix(powder_effect, sqrt(powder_effect), 0.5);

		phase = clouds_phase_multi(cos_theta, phase_g);
	}

	float scattering_integral = (1.0 - view_transmittance) / clouds_cirrus_extinction_coeff;
	return scattering * scattering_integral;
}

CloudsResult draw_cirrus_clouds(
	float3 air_viewer_pos,
	float3 ray_dir,
	float3 sun_dir,
	float3 sun_color,
	float3 clear_sky,
	float distance_to_terrain,
	float dither
) {
	// Early exit if coverage is 0
	//if (0.7/*CloudsParameters.cirrus_amount*/ < eps && 0.3454/*CloudsParameters.cirrocumulus_amount*/ < eps) { 
	//	return clouds_not_hit; 
	//}

	// ---------------
	//   Ray Casting
	// ---------------

	float r = length(air_viewer_pos);

	float2 dists = intersect_sphere(air_viewer_pos, ray_dir, clouds_cirrus_radius);
	bool planet_intersected = intersect_sphere(air_viewer_pos, ray_dir, min(r - 10.0, planet_radius)).y >= 0.0;
	bool terrain_intersected = distance_to_terrain >= 0.0 && r < clouds_cirrus_radius && distance_to_terrain < dists.y;

	//if (dists.y < 0.0                                  // sphere not intersected
	// || planet_intersected && r < clouds_cirrus_radius // planet blocking clouds
	// || terrain_intersected
	//) { return clouds_not_hit; }

	float distance_to_sphere = (r < clouds_cirrus_radius) ? dists.y : dists.x;
	float3 sphere_pos = air_viewer_pos + ray_dir * distance_to_sphere;

	// ------------------
	//   Cloud Lighting
	// ------------------

	//when implementing a real time day cycle return to this logic otherwise full sun.
	/*bool moonlit = sun_dir.y < -0.049;
	float3 light_dir = moonlit ? moon_dir : sun_dir;*/

	float3 light_dir = sun_dir;
	float cos_theta = dot(ray_dir, light_dir);

	float density = clouds_cirrus_density(sphere_pos.xz, 0.5);
	//if (density < eps) return clouds_not_hit;
	//#whatdoyoudo?														//sundir
	float light_optical_depth = clouds_cirrus_optical_depth(sphere_pos, ray_dir, dither);
	float view_optical_depth  = 0.5 * density * clouds_cirrus_extinction_coeff * clouds_cirrus_thickness * rcp(abs(ray_dir.y) + eps);
	float view_transmittance  = exp(-view_optical_depth);

	float2 scattering = clouds_cirrus_scattering(density, sun_dir, view_transmittance, light_optical_depth, cos_theta);

	// Get main light color for this layer
	float r_sq = dot(sphere_pos, sphere_pos);
	float rcp_r = invSqrt(r_sq);
	float mu = dot(sphere_pos, light_dir) * rcp_r;

	float3 light_color  = sunlight_color * atmosphere_transmittance(sphere_pos, light_dir);
	//light_color = atmosphere_post_processing(light_color);
	//moon integration // MOON
	//light_color *= moonlit ? moon_color : sun_color;
	light_color *= sun_color;

	// Remap the transmittance so that min_transmittance is 0
	float3 clouds_scattering = scattering.x * light_color + scattering.y * 1.41;
	     clouds_scattering = clouds_aerial_perspective(clouds_scattering, view_transmittance, air_viewer_pos, sphere_pos, ray_dir, clear_sky);

	return CreateCloudsResult(
		 float4(clouds_scattering, scattering.y),
		 view_transmittance,
		 distance_to_sphere
	);
}

#endif
