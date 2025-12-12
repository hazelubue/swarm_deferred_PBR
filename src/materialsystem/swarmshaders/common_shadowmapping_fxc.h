
#ifndef COMMON_SHADOWMAPPING_H
#define COMMON_SHADOWMAPPING_H


static float gauss3[3] =
{
	0.196842,
	0.606316,
	0.196842,
};

static float gauss4[4] =
{
	0.095773,
	0.404227,
	0.404227,
	0.095773,
};

static float gauss2D3[3] =
{
	0.038747,
	0.119348,
	0.367619,
};

float ShadowDepth_Raw_Nvidia( sampler depthMap, float3 uvw )
{
	return tex2Dproj( depthMap, float4( uvw, 1 ) ).x;
}

float ShadowDepth_3x3Gauss_Nvidia( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
	float2 uv = uvw.xy;
	float objDepth = uvw.z;

	float2 fraction = abs( frac( uv * offsets_1.xy ) - 0.5f );

	// 'Invalid src mod for second source param' ... GODDAMNIT
	fraction = 1.0f - fraction * fraction * 0.800001f;

	offsets_0 *= fraction.xyxy;

	float lightGauss = tex2Dproj( depthMap, float4( uvw, 1 ) ).x * gauss2D3[ 2 ];

	lightGauss += ( tex2Dproj( depthMap, float4( uv + float2( offsets_0.x, 0 ), objDepth, 1 ) ).x +
					tex2Dproj( depthMap, float4( uv - float2( offsets_0.x, 0 ), objDepth, 1 ) ).x +
					tex2Dproj( depthMap, float4( uv + float2( 0, offsets_0.y ), objDepth, 1 ) ).x +
					tex2Dproj( depthMap, float4( uv - float2( 0, offsets_0.y ), objDepth, 1 ) ).x ) * gauss2D3[ 1 ];

	lightGauss += ( tex2Dproj( depthMap, float4( uv + offsets_0.xy, objDepth, 1 ) ).x +
					tex2Dproj( depthMap, float4( uv + float2( -offsets_0.x, offsets_0.y ), objDepth, 1 ) ).x +
					tex2Dproj( depthMap, float4( uv + float2( offsets_0.x, -offsets_0.y ), objDepth, 1 ) ).x +
					tex2Dproj( depthMap, float4( uv - offsets_0.xy, objDepth, 1 ) ).x ) * gauss2D3[ 0 ];

	return lightGauss;
}

float ShadowDepth_5x5Gauss_Nvidia( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
	float2 uv = uvw.xy;
	float objDepth = uvw.z;

	float2 fraction = abs( frac( uv * offsets_1.xy ) - 0.5f );
	fraction = 1.0f - fraction * fraction * 0.33333f;

	offsets_0 *= fraction.xyxy;

	float lightGauss = tex2Dproj( depthMap, float4( uvw, 1 ) ).x * 0.162103f;

	lightGauss += ( tex2Dproj( depthMap, float4( uv + offsets_0.zw, objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( -offsets_0.z, offsets_0.w ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( offsets_0.z, -offsets_0.w ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv - offsets_0.zw, objDepth, 1 ) ).x ) * 0.002969f;

	lightGauss += ( tex2Dproj( depthMap, float4( uv + offsets_0.zy, objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + offsets_0.xw, objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( -offsets_0.x, offsets_0.w ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( -offsets_0.z, offsets_0.y ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv - offsets_0.zy, objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv - offsets_0.xw, objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( offsets_0.x, -offsets_0.w ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( offsets_0.z, -offsets_0.y ), objDepth, 1 ) ).x ) * 0.013306f;

	lightGauss += ( tex2Dproj( depthMap, float4( uv + float2( offsets_0.z, 0 ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( -offsets_0.z, 0 ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( 0, offsets_0.w ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( 0, -offsets_0.w ), objDepth, 1 ) ).x ) * 0.021938f;

	lightGauss += ( tex2Dproj( depthMap, float4( uv + offsets_0.xy, objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( -offsets_0.x, offsets_0.y ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( offsets_0.x, -offsets_0.y ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv - offsets_0.xy, objDepth, 1 ) ).x ) * 0.059634f;

	lightGauss += ( tex2Dproj( depthMap, float4( uv + float2( offsets_0.x, 0 ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv - float2( offsets_0.x, 0 ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv + float2( 0, offsets_0.y ), objDepth, 1 ) ).x +
		tex2Dproj( depthMap, float4( uv - float2( 0, offsets_0.y ), objDepth, 1 ) ).x ) * 0.098320f;

	return lightGauss;
}

float ShadowColor_Raw( sampler depthMap, float3 uvw )
{
	float shadowmapDepth = tex2D( depthMap, uvw.xy ).x;

	return saturate( ceil( shadowmapDepth - uvw.z ) );
}


float ShadowColor_3x3SoftwareBilinear_Box( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
	uvw.xy *= offsets_1.xy;
	float2 texel_min = floor( uvw.xy ) / offsets_1.xy + (offsets_0.xy * 0.5f);
	float2 frac_uv = frac( uvw.xy );

#define TWEAK_SUBTRACT_SELF_3x3 8333.3f

	float3x3 pcf_samples = saturate(
							float3x3(	float3(		uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, -offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( 0, -offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, -offsets_0.y ) ).r )
										* TWEAK_SUBTRACT_SELF_3x3,

										float3(		uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, 0 ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( 0, 0 ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, 0 ) ).r )
										* TWEAK_SUBTRACT_SELF_3x3,

										float3(		uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( 0, offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, offsets_0.y ) ).r )
										* TWEAK_SUBTRACT_SELF_3x3
										)
									);

	// not optimizing this because it sucks anyway..
	float flLight = lerp(
							lerp( pcf_samples[0][0], pcf_samples[0][1], frac_uv.x ),
							lerp( pcf_samples[1][0], pcf_samples[1][1], frac_uv.x ),
							frac_uv.y
						)
						+ lerp(
							lerp( pcf_samples[0][1], pcf_samples[0][2], frac_uv.x ),
							lerp( pcf_samples[1][1], pcf_samples[1][2], frac_uv.x ),
							frac_uv.y
						)

						+ lerp(
							lerp( pcf_samples[1][0], pcf_samples[1][1], frac_uv.x ),
							lerp( pcf_samples[2][0], pcf_samples[2][1], frac_uv.x ),
							frac_uv.y
						)
						+ lerp(
							lerp( pcf_samples[1][1], pcf_samples[1][2], frac_uv.x ),
							lerp( pcf_samples[2][1], pcf_samples[2][2], frac_uv.x ),
							frac_uv.y
						);

	flLight *= 1.0f / 4.0f;

	return 1.0f - flLight;
}


float ShadowColor_4x4SoftwareBilinear_Box( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
	uvw.xy *= offsets_1.xy;
	float2 texel_min = floor( uvw.xy ) / offsets_1.xy + (offsets_0.xy * 0.5f);
	float2 frac_uv = frac( uvw.xy );

#define TWEAK_SUBTRACT_SELF 16666.6f
#define TWEAK_SUBTRACT_SELF_FAR 8333.3f
#define TWEAK_SUBTRACT_SELF_FAR_MAX 4166.6f

	float4x4 pcf_samples = saturate(
							float4x4(	float4(		uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, -offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.z, -offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, offsets_0.w ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.z, offsets_0.w ) ).r )
										* TWEAK_SUBTRACT_SELF_FAR_MAX,

										float4(		uvw.z - tex2D( depthMap, texel_min + float2( 0, -offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, -offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, 0 ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.z, 0 ) ).r )
										* TWEAK_SUBTRACT_SELF_FAR,

										float4(		uvw.z - tex2D( depthMap, texel_min + float2( -offsets_0.x, offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.z, offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( 0, offsets_0.w ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, offsets_0.w ) ).r )
										* TWEAK_SUBTRACT_SELF_FAR,

										float4(		uvw.z - tex2D( depthMap, texel_min ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, 0 ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( 0, offsets_0.y ) ).r,
													uvw.z - tex2D( depthMap, texel_min + float2( offsets_0.x, offsets_0.y ) ).r )
										* TWEAK_SUBTRACT_SELF
										)
									);

	float2 frac_uv_inv = 1.0f - frac_uv;

	float4 weights =
		float4( frac_uv.x * frac_uv.y,
				frac_uv_inv.x * frac_uv.y,
				frac_uv.x * frac_uv_inv.y,
				frac_uv_inv.x * frac_uv_inv.y );

	float flLight = dot( weights, float4(
			pcf_samples[2][2], pcf_samples[0][2], pcf_samples[1][0], pcf_samples[0][0] ) )
		+			dot( weights, float4(
			pcf_samples[2][3], pcf_samples[2][2], pcf_samples[1][1], pcf_samples[1][0] ) )
		+			dot( weights, float4(
			pcf_samples[0][3], pcf_samples[2][3], pcf_samples[0][1], pcf_samples[1][1] ) )

		+			dot( weights, pcf_samples[3][0] )
		+			dot( weights, pcf_samples[3][1] )
		+			dot( weights, float4(
			pcf_samples[1][3], pcf_samples[1][2], pcf_samples[1][3], pcf_samples[1][2] ) )

		+			dot( weights, pcf_samples[3][2] )
		+			dot( weights, pcf_samples[3][3] )
		+			dot( weights, float4(
			pcf_samples[2][1], pcf_samples[2][0], pcf_samples[2][1], pcf_samples[2][0] ) );

	flLight *= 1.0f / 9.0f;

	return 1.0f - flLight;
}

float ShadowColor_SoftwareBilinear_SingleRow_4Tap( sampler depthMap, float objDepth, float2 uv_start, float texelsize, float frac_x )
{
	float flLast = ceil( tex2D( depthMap, uv_start ).r - objDepth );
	float flLight = 0.0f;
	for ( int x = 0; x < 3; x++ )
	{
		uv_start.x += texelsize;

		float flNext = ceil( tex2D( depthMap, uv_start ).r - objDepth );
		flLight += lerp( flLast, flNext, frac_x ) * gauss3[x];

		flLast = flNext;
	}
	return flLight;
}

float ShadowColor_SoftwareBilinear_SingleRow_5Tap( sampler depthMap, float objDepth, float2 uv_start, float texelsize, float frac_x )
{
	float flLast = ceil( tex2D( depthMap, uv_start ).r - objDepth );
	float flLight = 0.0f;
	for ( int x = 0; x < 4; x++ )
	{
		uv_start.x += texelsize;

		float flNext = ceil( tex2D( depthMap, uv_start ).r - objDepth );
		flLight += lerp( flLast, flNext, frac_x ) * gauss4[x];

		flLast = flNext;
	}
	return flLight;
}

float ShadowColor_4x4SoftwareBilinear_Gauss( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
	float2 frac_uv = frac( uvw.xy * offsets_1.xy );
	float2 texel_min = uvw.xy - frac_uv.xy / offsets_1.xy - offsets_0.xy * 0.5f;

	float flLight = 0.0f;
	float flRowLast = ShadowColor_SoftwareBilinear_SingleRow_4Tap( depthMap, uvw.z, texel_min, offsets_0.x, frac_uv.x );

	for ( int y = 0; y < 3; y++ )
	{
		texel_min.y += offsets_0.y;

		float flRowCur = ShadowColor_SoftwareBilinear_SingleRow_4Tap( depthMap, uvw.z, texel_min, offsets_0.x, frac_uv.x );
		flLight += lerp( flRowLast, flRowCur, frac_uv.y ) * gauss3[y];

		flRowLast = flRowCur;
	}

	return flLight;
}

float ShadowColor_5x5SoftwareBilinear_Gauss( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
	float2 frac_uv = frac( uvw.xy * offsets_1.xy );
	float2 texel_min = uvw.xy - frac_uv.xy / offsets_1.xy - offsets_0.xy * 0.5f;

	float flLight = 0.0f;
	float flRowLast = ShadowColor_SoftwareBilinear_SingleRow_5Tap( depthMap, uvw.z, texel_min, offsets_0.x, frac_uv.x );

	for ( int y = 0; y < 4; y++ )
	{
		texel_min.y += offsets_0.y;

		float flRowCur = ShadowColor_SoftwareBilinear_SingleRow_5Tap( depthMap, uvw.z, texel_min, offsets_0.x, frac_uv.x );
		flLight += lerp( flRowLast, flRowCur, frac_uv.y ) * gauss4[y];

		flRowLast = flRowCur;
	}

	return flLight;
}

float EstimatePenumbra(sampler depthMap, float3 uvw, float2 searchRadius)
{
	float blockerSum = 0.0f;
	float numBlockers = 0.0f;

	static const float2 searchPattern[8] = {
		float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
		float2(-0.09418410, -0.92938870), float2(0.34495938, 0.29387760),
		float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
		float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),

	};

	for (int i = 0; i < 8; i++)
	{
		float2 sampleUV = uvw.xy + searchPattern[i] * searchRadius;
		float blockerDepth = tex2D(depthMap, sampleUV).r;

		if (blockerDepth < uvw.z - 0.001f)
		{
			blockerSum += blockerDepth;
			numBlockers += 1.0f;
		}
	}

	if (numBlockers < 1.0f) return 0.0f;

	float avgBlockerDepth = blockerSum / numBlockers;
	float penumbra = (uvw.z - avgBlockerDepth) / avgBlockerDepth;
	return saturate(penumbra * 50.0f);
}
float2 Hash22(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * float3(443.897, 441.423, 437.195));
	p3 += dot(p3, p3.yzx + 19.19);
	return frac((p3.xx + p3.yz) * p3.zy);
}

float Hash(float2 p)
{
	return frac(sin(dot(p, float2(500.9898, 78.233))) * 43758.5453);
}

float BlueNoise(float2 uv)
{
	float n1 = Hash(uv);
	float n2 = Hash(uv * 2.0 + 10.37);
	float n3 = Hash(uv * 4.0 - 12.12);

	return frac((n1 + n2 + n3) / 40.0);
}

float WhiteNoise(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float InterleavedGradientNoise(float2 uv)
{
	float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
	return frac(magic.z * frac(dot(uv, magic.xy)));
}


float GetDistanceBlurFactor(float depth)
{
	float normalizedDepth = saturate(depth);
	float blurFactor = 1.0f - exp(-normalizedDepth * 1.5f);

	blurFactor = smoothstep(0.0f, 1.0f, blurFactor);

	return 1.0f + blurFactor * 1.0f;
}

float2 Rand(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float rand_white(in float2 uv)
{
	float2 noise = (frac(sin(dot(uv, float2(6.9898, 32.233) * 1.0)) * 21324.5453));
	return abs(noise.x + noise.y) * 0.5;
}


float ShadowColor_4x4SoftwareBilinear_PenumbraNoise(sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1)
{

	float distanceScale = GetDistanceBlurFactor(uvw.z);
	uvw.xy *= offsets_1.xy;
	// avoid floor() snapping — use continuous UVs so hardware bilinear/PCF can smooth properly
	float2 rand = Rand(uvw.xy);
	float2 rotatedOffset = offsets_0.xy * (rand.x - 0.5f) + offsets_0.yz * (rand.y - 0.5f);

	float2 texel_min = uvw.xy / offsets_1.xy + rotatedOffset * distanceScale;
	float2 frac_uv = frac(uvw.xy);

#define TWEAK_SUBTRACT_SELF 8333.3f
#define TWEAK_SUBTRACT_SELF_FAR 8333.3f
#define TWEAK_SUBTRACT_SELF_FAR_MAX 4166.6f
	const float DEPTH_BIAS = 0.0015f; // tune slightly if you get shadow acne / detachment

	float4x4 pcf_samples = saturate(
		float4x4(float4(uvw.z - tex2D(depthMap, texel_min + float2(-offsets_0.x, -offsets_0.y)).r,
			uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.z, -offsets_0.y)).r,
			uvw.z - tex2D(depthMap, texel_min + float2(-offsets_0.x, offsets_0.w)).r,
			uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.z, offsets_0.w)).r)
			* TWEAK_SUBTRACT_SELF_FAR_MAX,

			float4(uvw.z - tex2D(depthMap, texel_min + float2(0, -offsets_0.y)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.x, -offsets_0.y)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(-offsets_0.x, 0)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.z, 0)).r)
			* TWEAK_SUBTRACT_SELF_FAR,

			float4(uvw.z - tex2D(depthMap, texel_min + float2(-offsets_0.x, offsets_0.y)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.z, offsets_0.y)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(0, offsets_0.w)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.x, offsets_0.w)).r)
			* TWEAK_SUBTRACT_SELF_FAR,

			float4(uvw.z - tex2D(depthMap, texel_min).r,
				uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.x, 0)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(0, offsets_0.y)).r,
				uvw.z - tex2D(depthMap, texel_min + float2(offsets_0.x, offsets_0.y)).r)
			* TWEAK_SUBTRACT_SELF
		)
	);

	float2 frac_uv_inv = 1.0f - frac_uv;

	float4 weights =
		float4(frac_uv.x * frac_uv.y,
			frac_uv_inv.x * frac_uv.y,
			frac_uv.x * frac_uv_inv.y,
			frac_uv_inv.x * frac_uv_inv.y);

	float flLight = dot(weights, float4(
		pcf_samples[2][2], pcf_samples[0][2], pcf_samples[1][0], pcf_samples[0][0]))
		+ dot(weights, float4(
			pcf_samples[2][3], pcf_samples[2][2], pcf_samples[1][1], pcf_samples[1][0]))
		+ dot(weights, float4(
			pcf_samples[0][3], pcf_samples[2][3], pcf_samples[0][1], pcf_samples[1][1]))

		+ dot(weights, pcf_samples[3][0])
		+ dot(weights, pcf_samples[3][1])
		+ dot(weights, float4(
			pcf_samples[1][3], pcf_samples[1][2], pcf_samples[1][3], pcf_samples[1][2]))

		+ dot(weights, pcf_samples[3][2])
		+ dot(weights, pcf_samples[3][3])
		+ dot(weights, float4(
			pcf_samples[2][1], pcf_samples[2][0], pcf_samples[2][1], pcf_samples[2][0]));

	flLight *= 1.0f / 9.0f;

	static const float2 poissonDisk[32] = {
		float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
		float2(-0.09418410, -0.92938870), float2(0.34495938, 0.29387760),
		float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
		float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
		float2(-0.15300845, 0.58071113), float2(0.68180464, -0.19486278),
		float2(0.26575993, 0.74764085), float2(-0.53742909, -0.47373420),
		float2(0.89132898, 0.35857890), float2(-0.71915442, 0.23895057),
		float2(0.45525315, -0.54746658), float2(-0.25234789, 0.89764521),
		float2(-0.63680227, 0.14549147), float2(0.14807814, -0.28745039),
		float2(0.59766323, 0.11853144), float2(-0.84075239, -0.08738724),
		float2(0.30645928, -0.83930749), float2(-0.71698458, 0.66778630),
		float2(0.87613220, -0.40059934), float2(-0.40208414, -0.71104700),
		float2(0.18090531, 0.94938154), float2(0.71494067, 0.61670959),
		float2(-0.25788117, 0.36127573), float2(-0.99039023, 0.12448689),
		float2(0.42770064, 0.51747394), float2(-0.05075629, -0.13149844),
		float2(0.63387454, -0.27338242), float2(-0.18652373, -0.51248652)
	};

	float penumbraSize = EstimatePenumbra(depthMap, uvw, offsets_0.xy * 4.0f);

	float blurOffsetScale = lerp(1.0f, 4.0f, saturate(penumbraSize)); // tuneable
	float2 blurOffsetUV = offsets_0.xy * blurOffsetScale * distanceScale;

	//float2 blurOffsetUV = offsets_0.xy * (2.0f + penumbraSize * 3.0f) * distanceScale;

	float blurredShadow = 0.0f;

	for (int i = 0; i < 8; i++) {
		float2 sampleUV = texel_min + poissonDisk[i] * blurOffsetUV;
		float depthSample = tex2D(depthMap, sampleUV).r;

		float shadowFactor = saturate((uvw.z - depthSample) / 0.02f);
		blurredShadow += shadowFactor * 0.125f; // 1/8 average
	}

	float Guass = tex2Dproj(depthMap, float4(uvw, 1)).x * 0.162103f;

	float noise = 0.0f;
	float2 baseBlurRadius = float2(0.5f, 0.5f);
	float2 blurRadius = baseBlurRadius * (1.0f + penumbraSize * 3.0f) * distanceScale; // penumbra increases radius

	for (int i = 0; i < 8; ++i)
	{
		noise += rand_white(float3(uvw.xy, 0.0)) * 1.0;
		//noise += BlueNoise(uvw.xy - 5.0f * Guass + poissonDisk[i] * blurRadius);
	}
	noise *= (1.0f / 8.0f); // average

	flLight = saturate(blurredShadow);
	float edgeDistance = abs(flLight - 0.5f);
	float isEdge = 1.0f - smoothstep(0.0f, 0.3f, edgeDistance); // adjust 0.3f for edge width

	float distanceFromEdge = abs(flLight - 0.5f) * 2.0f; // 0 at edges, 1 at extremes

	const float PENUMBRA_EPS = 1e-4f;
	float safePenumbra = max(penumbraSize, PENUMBRA_EPS);
	float penumbraNoise = noise / safePenumbra;

	flLight += (penumbraNoise - 0.5f) * 0.008f * distanceFromEdge * isEdge;
	flLight = saturate(flLight);

	float noisyEdge = smoothstep(0.3f, 0.7f, penumbraNoise);
	flLight = lerp(flLight, noisyEdge, isEdge * 0.05f); // Much smaller blend

	return 1.0f - flLight;
}


float2 poissonDisk[16] = 
{
	float2( -0.94201624, -0.39906216 ),
	float2( 0.94558609, -0.76890725 ),
	float2( -0.094184101, -0.92938870 ),
	float2( 0.34495938, 0.29387760 ),
	float2( -0.91588581, 0.45771432 ),
	float2( -0.81544232, -0.87912464 ),
	float2( -0.38277543, 0.27676845 ),
	float2( 0.97484398, 0.75648379 ),
	float2( 0.44323325, -0.97511554 ),
	float2( 0.53742981, -0.47373420 ),
	float2( -0.26496911, -0.41893023 ),
	float2( 0.79197514, 0.19090188 ),
	float2( -0.24188840, 0.99706507 ),
	float2( -0.81409955, 0.91437590 ),
	float2( 0.19984126, 0.78641367 ),
	float2( 0.14383161, -0.14100790 )
};

void FindBlocker4x4
(
	out float avgBlockerDepth,
	out float numBlockers,
	sampler depthMap,
	float2 uv,
	float zReceiver,
	float zNear,
	float lightSizeUV
)
{
	//This uses similar triangles to compute what //area of the shadow map we should search
	//float searchWidth = lightSizeUV * (zReceiver - zNear) / zReceiver;
	float searchWidth = 1;

	float blockerSum = 0;
	numBlockers = 0;

	float shadowMapDepth = tex2D( depthMap, uv + poissonDisk[0] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[1] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[2] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[3] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[4] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[5] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[6] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[7] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[8] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[9] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[10] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[11] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[12] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[13] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[14] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}
	shadowMapDepth = tex2D( depthMap, uv + poissonDisk[15] * searchWidth ).r;
	if ( shadowMapDepth < zReceiver ) 
	{
		blockerSum += shadowMapDepth;
		numBlockers++;
	}

	avgBlockerDepth = blockerSum / numBlockers;
}

float PenumbraSize( float zReceiver, float zBlocker ) //Parallel plane estimation
{
	return (zReceiver - zBlocker) / zBlocker;
}

float PCFForPCSS4X4( float2 uv, sampler depthMap, float zReceiver, float filterRadiusUV )
{
	float sum = tex2D( depthMap, uv + poissonDisk[0] * filterRadiusUV ) > zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[1] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[2] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[3] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[4] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[5] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[6] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[7] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[8] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[9] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[10] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[11] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[12] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[13] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[14] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;
	sum += tex2D( depthMap, uv + poissonDisk[15] * filterRadiusUV ) < zReceiver ? 0.0625 : 0;

	return sum;
}

float ShadowColor_PCSS4X4_PCF4X4( sampler depthMap, float3 uvw, float zNear, float lightSizeUV )
{
	float avgBlockerDepth = 0;
	float numBlockers = 0;

	FindBlocker4x4( avgBlockerDepth, numBlockers, depthMap, uvw.xy, uvw.z, zNear, lightSizeUV );

	float flOut = 1.0f;
	
	if( numBlockers >= 1 )
	{
		// STEP 2: penumbra size
		float penumbraRatio = PenumbraSize( uvw.z, avgBlockerDepth );
		float filterRadiusUV = penumbraRatio * lightSizeUV / uvw.z;

		flOut = PCFForPCSS4X4( uvw.xy, depthMap, uvw.z, filterRadiusUV );
	}

	return flOut;
}

/*
pFl0[0] = 1.0f / resx;
pFl0[1] = 1.0f / resy;
pFl0[2] = 2.0f / resx;
pFl0[3] = 2.0f / resy;

pFl1[0] = resx;
pFl1[1] = resy;
*/

float PerformShadowMapping( sampler depthMap, float3 uvw, float4 offsets_0, float4 offsets_1 )
{
#if SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_COLOR__RAW
	return ShadowColor_Raw( depthMap, uvw );

#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_COLOR__4X4_SOFTWARE_BILINEAR_BOX
	return ShadowColor_4x4SoftwareBilinear_Box( depthMap, uvw, offsets_0, offsets_1 );

#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_COLOR__4X4_SOFTWARE_BILINEAR_GAUSSIAN
	return ShadowColor_4x4SoftwareBilinear_Gauss( depthMap, uvw, offsets_0, offsets_1 );

#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_COLOR__5X5_SOFTWARE_BILINEAR_GAUSSIAN
	return ShadowColor_5x5SoftwareBilinear_Gauss( depthMap, uvw, offsets_0, offsets_1 );

#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_STENCIL__RAW
	return ShadowDepth_Raw_Nvidia( depthMap, uvw );

#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_STENCIL__3X3_GAUSSIAN
	return ShadowDepth_3x3Gauss_Nvidia( depthMap, uvw, offsets_0, offsets_1 );

#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_STENCIL__5X5_GAUSSIAN
	return ShadowDepth_5x5Gauss_Nvidia( depthMap, uvw, offsets_0, offsets_1 );
#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_COLOR__PCSS_4X4_PCF_4X4
	return ShadowColor_PCSS4X4_PCF4X4( depthMap, uvw, 10, 0.15 ); //hijack offsets_0
#elif SHADOWMAPPING_METHOD == SHADOWMAPPING_DEPTH_COLOR__4X4_SOFTWARE_BILINEAR_PENUMBRA
	return ShadowColor_4x4SoftwareBilinear_PenumbraNoise( depthMap, uvw, offsets_0, offsets_1 );
#else
	unknown_shadow_mapping_method
#endif
}

float3 ToShadowSpace_Ortho( float3 worldPos, float viewFwdDot, float3 vecNormal,
	float3 vecSlopeData, float4x3 viewProjOrtho )
{
	worldPos += vecNormal * ( 1.0f - abs( viewFwdDot ) ) * vecSlopeData.z;

	float3 shadowPos = mul( float4( worldPos, 1 ), viewProjOrtho );

	return shadowPos.xyz;
}

float PerformCascadedShadow( sampler sShadowMap, float3 worldPos,
	float4x3 viewProjOrtho[SHADOW_NUM_CASCADES], float4 vecUVTransform[SHADOW_NUM_CASCADES], float3 vecSlopeData[SHADOW_NUM_CASCADES],
	float4 vecFilterConfig_A[SHADOW_NUM_CASCADES], float4 vecFilterConfig_B[SHADOW_NUM_CASCADES],
	float3 flNormal, float viewFwdDot )
{
#if 1
	float3 shadow_uvz = ToShadowSpace_Ortho( worldPos, viewFwdDot, flNormal, vecSlopeData[0], viewProjOrtho[0] );
	float3 shadow_uvz_2 = ToShadowSpace_Ortho( worldPos, viewFwdDot, flNormal, vecSlopeData[1], viewProjOrtho[1] );

#if VENDOR == VENDOR_FXC_AMD
	float3 AMDVec = abs( floor( (shadow_uvz.xyz - 0.0015f) * 1.003f ) );
	float AMDAmt = AMDVec.x + AMDVec.y + AMDVec.z;
	int flLerpTo1 = step( 0.0001f, AMDAmt );
#else
	float flLerpTo1 = any( floor( (shadow_uvz.xyz - 0.0015f) * 1.003f) );
#endif

	shadow_uvz = lerp( shadow_uvz, shadow_uvz_2, flLerpTo1 );

#if VENDOR == VENDOR_FXC_AMD
	AMDVec = abs( floor( (shadow_uvz_2.xyz - 0.003f) * 1.006f ) );
	AMDAmt = AMDVec.x + AMDVec.y + AMDVec.z;
	int flLerpTo2 = step( 0.0001f, AMDAmt );
#else
	float flLerpTo2 = any( floor( (shadow_uvz_2.xyz - 0.003f) * 1.006f) );
#endif

	shadow_uvz.xy = shadow_uvz.xy * vecUVTransform[flLerpTo1].zw + vecUVTransform[flLerpTo1].xy;

	float flLight = lerp( PerformShadowMapping( sShadowMap, shadow_uvz,
				vecFilterConfig_A[flLerpTo1], vecFilterConfig_B[flLerpTo1] ), 1, flLerpTo2 );
#else
	int curCascade = 0;
	bool bDoShadowmapping = true;
	float flLight = 1.0f;

	float3 shadow_uvz = ToShadowSpace_Ortho( worldPos, viewFwdDot, flNormal, vecSlopeData[curCascade], viewProjOrtho[curCascade] );

	if ( any( floor( (shadow_uvz.xyz - 0.0015f) * 1.003f) ) )
	{
		curCascade++;
		shadow_uvz = ToShadowSpace_Ortho( worldPos, viewFwdDot, flNormal, vecSlopeData[curCascade], viewProjOrtho[curCascade] );

		bDoShadowmapping = !any( floor( (shadow_uvz.xyz - 0.003f) * 1.006f ) );
	}

	if ( bDoShadowmapping )
	{
		shadow_uvz.xy = shadow_uvz.xy * vecUVTransform[curCascade].zw + vecUVTransform[curCascade].xy;

		flLight *= PerformShadowMapping( sShadowMap, shadow_uvz,
				vecFilterConfig_A[curCascade], vecFilterConfig_B[curCascade] );
	}
#endif

	return flLight;
}

float PerformDualParaboloidShadow( sampler shadowSampler, float3 vecLightToGeometry,
	float4 offsets_0, float4 offset_1,
	float lightToGeoDistance, float radius, float shadowMin )
{
	vecLightToGeometry = vecLightToGeometry / lightToGeoDistance;

	bool bBack = vecLightToGeometry.z < 0;

	vecLightToGeometry.z = abs(vecLightToGeometry.z) + 1;
	vecLightToGeometry.xy = vecLightToGeometry.xy / vecLightToGeometry.z;

	lightToGeoDistance = min( 0.99f, lightToGeoDistance/radius );

	vecLightToGeometry.y = vecLightToGeometry.y * 0.2475f + lerp( 0.25f, 0.75f, bBack );
	vecLightToGeometry.x = vecLightToGeometry.x * lerp( -0.495f, 0.495f, bBack ) + 0.5f;

	float3 uvw = float3( vecLightToGeometry.xy, lightToGeoDistance );

	return max( shadowMin, PerformShadowMapping( shadowSampler, uvw, offsets_0, offset_1 ) );
}

float PerformProjectedShadow( sampler shadowSampler, float3 uvw,
	float4 offsets_0, float4 offset_1, float shadowMin )
{
	return max( shadowMin, PerformShadowMapping( shadowSampler, uvw, offsets_0, offset_1 ) );
}

#endif