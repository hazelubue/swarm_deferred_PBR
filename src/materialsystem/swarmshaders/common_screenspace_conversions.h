#if !defined INCLUDE_UTILITY_SPACE_CONVERSION
#define INCLUDE_UTILITY_SPACE_CONVERSION

// https://wiki.shaderlabs.org/wiki/Shader_tricks#Linearizing_depth
float linearize_depth(float near, float far, float depth) {
	return (near * far) / (depth * (near - far) + far);
}
float linearize_depth(float depth) {
	return linearize_depth(near, far, depth);
}

float reverse_linear_depth(float near, float far, float linear_z) {
	return (far + near) / (far - near) + (2.0 * far * near) / (linear_z * (far - near));
}
float reverse_linear_depth(float linear_z) {
	return reverse_linear_depth(near, far, linear_z);
}

// Approximate linear depth function by DrDesten
float linearize_depth_fast(float near, float depth) {
	return near / (1.0 - depth);
}
float linearize_depth_fast(float depth) {
	return linearize_depth_fast(near, depth);
}

float4 project_and_divide(float4 position, float4x4 projectionMatrix)
{
	// Step 1: Project the position using the matrix
	float4 projected = mul(position, projectionMatrix);

	// Step 2: Divide by the w-component (perspective division)
	// This converts clip space coordinates to normalized device coordinates (NDC)
	// [0, 1] for x, y; [0, 1] for depth (w can be negative)
	return projected / projected.w;
}

float3 screen_to_view_space(float4x4 projection_matrix_inverse, float3 screen_pos, bool handle_jitter) {
	float3 ndc_pos = 2.0 * screen_pos - 1.0;

#ifdef TAA
#ifdef TAAU
	float2 jitter_offset = taa_offset * rcp(taau_render_scale);
#else
	float2 jitter_offset = taa_offset * 0.66;
#endif

	if (handle_jitter) ndc_pos.xy -= jitter_offset;
#endif

	return project_and_divide(float4(ndc_pos, 0.0), projection_matrix_inverse);
}

float3 view_to_screen_space(float4x4 projection_matrix, float3 view_pos, bool handle_jitter) {
	float3 ndc_pos = project_and_divide(projection_matrix, view_pos);

#ifdef TAA
#ifdef TAAU
	float2 jitter_offset = taa_offset * rcp(taau_render_scale);
#else
	float2 jitter_offset = taa_offset * 0.66;
#endif

	if (handle_jitter) ndc_pos.xy += jitter_offset;
#endif

	return ndc_pos * 0.5 + 0.5;
}

float3 screen_to_view_space(float3 screen_pos, bool handle_jitter) {
	return screen_to_view_space(gbufferProjectionInverse, screen_pos, handle_jitter);
}

float3 view_to_screen_space(float3 view_pos, bool handle_jitter) {
	return view_to_screen_space(gbufferProjection, view_pos, handle_jitter);
}

float3 screen_to_view_space(float3 screen_pos, bool handle_jitter, bool is_dh_terrain) {
#ifdef DISTANT_HORIZONS
	float4x4 projection_matrix_inverse = is_dh_terrain
		? dhProjectionInverse
		: gbufferProjectionInverse;

	return screen_to_view_space(projection_matrix_inverse, screen_pos, handle_jitter);
#else
	return screen_to_view_space(gbufferProjectionInverse, screen_pos, handle_jitter);
#endif
}

float3 view_to_screen_space(float3 view_pos, bool handle_jitter, bool is_dh_terrain) {
#ifdef DISTANT_HORIZONS
	float4x4 projection_matrix = is_dh_terrain
		? dhProjection
		: gbufferProjection;

	return view_to_screen_space(projection_matrix, view_pos, handle_jitter);
#else
	return view_to_screen_space(gbufferProjection, view_pos, handle_jitter);
#endif
}

float screen_to_view_space_depth(float4x4 projection_matrix_inverse, float depth) {
	depth = depth * 2.0 - 1.0;
	float2 zw = depth * projection_matrix_inverse[2].zw + projection_matrix_inverse[3].zw;
	return -zw.x / zw.y;
}

float view_to_screen_space_depth(float4x4 projection_matrix, float depth) {
	float2 zw = -depth * projection_matrix[2].zw + projection_matrix[3].zw;
	return (zw.x / zw.y) * 0.5 + 0.5;
}

float3 view_to_scene_space(float3 view_pos) {
	return transform(gbufferModelViewInverse, view_pos);
}

float3 scene_to_view_space(float3 scene_pos) {
	return transform(gbufferModelView, scene_pos);
}

mat3 get_tbn_matrix(float3 normal) {
	float3 tangent = normal.y == 1.0 ? float3(1.0, 0.0, 0.0) : normalize(cross(float3(0.0, 1.0, 0.0), normal));
	float3 bitangent = normalize(cross(tangent, normal));
	return mat3(tangent, bitangent, normal);
}

#if defined TEMPORAL_REPROJECTION
float3 reproject_scene_space(float3 scene_pos, bool hand, bool is_dh_terrain) {
#ifdef DISTANT_HORIZONS
	float4x4 previous_projection_matrix = is_dh_terrain
		? dhPreviousProjection
		: gbufferPreviousProjection;
#else
	float4x4 previous_projection_matrix = gbufferPreviousProjection;
#endif

	float3 camera_offset = hand
		? float3(0.0)
		: cameraPosition - previousCameraPosition;

	float3 previous_pos = transform(gbufferPreviousModelView, scene_pos + camera_offset);
	previous_pos = project_and_divide(previous_projection_matrix, previous_pos);

	return previous_pos * 0.5 + 0.5;
}

float3 reproject(float3 screen_pos, bool is_dh_terrain) {
	float3 pos = screen_to_view_space(screen_pos, false, is_dh_terrain);
	pos = view_to_scene_space(pos);

	bool hand = screen_pos.z < hand_depth;

	return reproject_scene_space(pos, hand, is_dh_terrain);
}
float3 reproject(float3 screen_pos) {
	return reproject(screen_pos, false);
}
float3 reproject(float3 screen_pos, sampler2D velocity_sampler) {
	float3 velocity = texelFetch(velocity_sampler, ifloat2(screen_pos.xy * view_res), 0).xyz;

	if (max_of(abs(velocity)) < eps) {
		return reproject(screen_pos);
	}
	else {
		float3 pos = screen_to_view_space(screen_pos, false);
		pos = pos - velocity;
		pos = view_to_screen_space(pos, false);

		return pos;
	}
}
#endif

#endif // INCLUDE_UTILITY_SPACE_CONVERSION
