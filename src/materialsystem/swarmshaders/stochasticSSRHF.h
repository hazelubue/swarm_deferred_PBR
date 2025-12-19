// Minimal SSR-specific definitions from globals.hlsli
#define PI 3.14159265358979323846
#define FLT_MAX 3.402823466e+38
#define sqr(a) ((a)*(a))

#define GGX_SAMPLE_VISIBLE

// Bias used on GGX importance sample when denoising, to remove part of the tale that create a lot more noise.
#define GGX_IMPORTANCE_SAMPLE_BIAS 0.1
static const int SSR_TILESIZE = 32;
static const int POSTPROCESS_BLOCKSIZE = 8;

inline int2 unflatten2D(int idx, int width)
{
	return int2(idx % width, idx / width);
}

int2 GetReflectionIndirectDispatchCoord(int3 Gid, int3 GTid, int2 tiles, int downsample)
{
	int tile_replicate = sqr(SSR_TILESIZE / downsample / POSTPROCESS_BLOCKSIZE);
	int tile_idx = Gid.x / tile_replicate;
	int tile_packed = tiles[tile_idx];
	// DX9 compatible unpacking without bitwise operations
	int2 tile = int2(tile_packed % 65536, tile_packed / 65536);
	int subtile_idx = Gid.x % tile_replicate;
	int2 subtile = unflatten2D(subtile_idx, SSR_TILESIZE / downsample / POSTPROCESS_BLOCKSIZE);
	int2 subtile_upperleft = tile * SSR_TILESIZE / downsample + subtile * POSTPROCESS_BLOCKSIZE;
	return subtile_upperleft + unflatten2D(GTid.x, POSTPROCESS_BLOCKSIZE);
}

//inline float3 reconstruct_position(in float2 uv, in float z, in float4x4 inverse_view_projection)
//{
//	float x = uv.x * 2 - 1;
//	float y = (1 - uv.y) * 2 - 1;
//	float4 position_s = float4(x, y, z, 1);
//	float4 position_v = mul(inverse_view_projection, position_s);
//	return position_v.xyz / position_v.w;
//}
//inline float3 reconstruct_position(in float2 uv, in float z)
//{
//	return reconstruct_position(uv, z, GetCamera().inverse_view_projection);
//}

float3x3 get_tangentspace(in float3 normal)
{
	float3 helper = abs(normal.x) > 0.99 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

float4 blue_noise(int2 pixel)
{
	// Simplified - replace with actual blue noise texture if needed
	return float4(frac(sin(dot(pixel, float2(12.9898, 78.233))) * 43758.5453), 0, 0, 0);
}

float4 blue_noise(int2 pixel, int frame)
{
	return blue_noise(pixel);
}

inline float2 hammersley2d(int idx, int num)
{
	int bits = idx;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float radicalInverse_VdC = float(bits) * 2.3283064365386963e-10;
	return float2(float(idx) / float(num), radicalInverse_VdC);
}

float InterleavedGradientNoise(float2 uv, int frameCount)
{
	const float2 magicFrameScale = float2(47, 17) * 0.695;
	uv += frameCount * magicFrameScale;
	const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
	return frac(magic.z * frac(dot(uv, magic.xy)));
}

// --- stochasticSSRHF.hlsli ---

#define min_roughness 0.045

float4 ImportanceSampleGGX(float2 Xi, float Roughness)
{
	Roughness = clamp(Roughness, min_roughness, 1);
	float m = Roughness * Roughness;
	float m2 = m * m;

	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
	float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));

	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
	float D = m2 / (PI * d * d);
	float pdf = D * CosTheta;

	return float4(H, pdf);
}

float3x3 GetTangentBasis(float3 TangentZ)
{
	const float Sign = TangentZ.z >= 0 ? 1 : -1;
	const float a = -rcp(Sign + TangentZ.z);
	const float b = TangentZ.x * TangentZ.y * a;

	float3 TangentX = { 1 + Sign * a * pow(TangentZ.x, 2), Sign * b, -Sign * TangentZ.x };
	float3 TangentY = { b, Sign + a * pow(TangentZ.y, 2), -TangentZ.y };

	return float3x3(TangentX, TangentY, TangentZ);
}

float2 SampleDisk(float2 Xi)
{
	float theta = 2 * PI * Xi.x;
	float radius = sqrt(Xi.y);
	return radius * float2(cos(theta), sin(theta));
}

float4 ImportanceSampleVisibleGGX(float2 diskXi, float roughness, float3 V)
{
	roughness = clamp(roughness, min_roughness, 1);
	float alphaRoughness = roughness * roughness;
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;

	float3 Vh = normalize(float3(alphaRoughness * V.xy, V.z));

	float3 tangent0 = (Vh.z < 0.9999) ? normalize(cross(float3(0, 0, 1), Vh)) : float3(1, 0, 0);
	float3 tangent1 = cross(Vh, tangent0);

	float2 p = diskXi;
	float s = 0.5 + 0.5 * Vh.z;
	p.y = (1 - s) * sqrt(1 - p.x * p.x) + s * p.y;

	float3 H;
	H = p.x * tangent0;
	H += p.y * tangent1;
	H += sqrt(saturate(1 - dot(p, p))) * Vh;

	H = normalize(float3(alphaRoughness * H.xy, max(0.0, H.z)));

	float NdotV = V.z;
	float NdotH = H.z;
	float VdotH = dot(V, H);

	float f = (NdotH * alphaRoughnessSq - NdotH) * NdotH + 1;
	float D = alphaRoughnessSq / (PI * f * f);

	float SmithGGXMasking = 2.0 * NdotV / (sqrt(NdotV * (NdotV - NdotV * alphaRoughnessSq) + alphaRoughnessSq) + NdotV);

	float PDF = SmithGGXMasking * VdotH * D / NdotV;

	return float4(H, PDF);
}

bool NeedReflection(float roughness, float depth, float roughness_cutoff)
{
	return (roughness <= roughness_cutoff) && (depth > 0.0);
}

half3 decode_oct(half2 e)
{
	half3 v = half3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * float2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
	return normalize(v);
}

float Luminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}