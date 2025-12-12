#ifndef DEFPASS_GBUFFER_TRANSLUCENT_H
#define DEFPASS_GBUFFER_TRANSLUCENT_H

class CDeferredPerMaterialContextData;

struct defParms_gBuffer_translucent
{
	defParms_gBuffer_translucent()
	{
		Q_memset(this, 0xFF, sizeof(defParms_gBuffer_translucent));

		bModel = false;
		bWater = false;
	};

	// textures
	int iAlbedo;
	int iAlbedo2;

	int iBumpmap;
	int iBumpmap2;
	int iBlendmodulate;

	int iSpecularTexture;

	// control
	int iAlphatestRef;
	int iLitface;
	int iPhongExp;
	int iPhongExp2;
	int iSSBump;

	// blending
	int iBlendmodulateTransform;

	int m_nMRAO;

	int m_nTransparency;

	int m_nTrasncluent;

	int m_nAlpha;

	//Sub surface scattering
	int m_nSSS;

	// Tree Sway!
	int m_nTreeSway;
	int m_nTreeSwayHeight;
	int m_nTreeSwayStartHeight;
	int m_nTreeSwayRadius;
	int m_nTreeSwayStartRadius;
	int m_nTreeSwaySpeed;
	int m_nTreeSwaySpeedHighWindMultiplier;
	int m_nTreeSwayStrength;
	int m_nTreeSwayScrumbleSpeed;
	int m_nTreeSwayScrumbleStrength;
	int m_nTreeSwayScrumbleFrequency;
	int m_nTreeSwayFalloffExp;
	int m_nTreeSwayScrumbleFalloffExp;
	int m_nTreeSwaySpeedLerpStart;
	int m_nTreeSwaySpeedLerpEnd;

	int PARALLAX;

	int FLOWMAP;
	int FLOWMAPFRAME;
	int FLOW_NOISE_TEXTURE;
	int FLOW_WORLDUVSCALE;
	int FLOW_NORMALUVSCALE;
	int FLOW_BUMPSTRENGTH;
	int COLOR_FLOW_DISPLACEBYNORMALSTRENGTH;
	int FLOW_TIMEINTERVALINSECONDS;
	int FLOW_UVSCROLLDISTANCE;
	int FLOW_NOISE_SCALE;
	int COLOR_FLOW_UVSCALE;
	int COLOR_FLOW_TIMEINTERVALINSECONDS;
	int COLOR_FLOW_UVSCROLLDISTANCE;
	int COLOR_FLOW_LERPEXP;

	int REFRACTTINT;
	int REFLECTAMOUNT;
	int REFRACTAMOUNT;
	int FOGCOLOR;
	int REFLECTTINT;
	int WATERBLENDFACTOR;
	int FOGSTART;
	int FOGEND;

	// config
	bool bModel;
	bool bWater;
};


void InitParmsGBuffer_translucent(const defParms_gBuffer_translucent& info, CBaseVSShader* pShader, IMaterialVar** params);
void InitPassGBuffer_translucent(const defParms_gBuffer_translucent& info, CBaseVSShader* pShader, IMaterialVar** params);
void DrawPassGBuffer_translucent(const defParms_gBuffer_translucent& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext);


#endif
