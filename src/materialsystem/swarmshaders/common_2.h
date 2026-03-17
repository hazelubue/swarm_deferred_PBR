
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