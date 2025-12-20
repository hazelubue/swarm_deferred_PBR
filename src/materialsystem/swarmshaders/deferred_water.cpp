
#include "deferred_includes.h"
#include "deferred_water.h"
//glass implentation
//#include "defpass_gbuffer1.h"

#include "tier0/memdbgon.h"


BEGIN_VS_SHADER(DEFERRED_WATER,
	"Help for Water")

	BEGIN_SHADER_PARAMS
	SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "")
	SHADER_PARAM(REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterRefraction", "")
	SHADER_PARAM(REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "")
	SHADER_PARAM(REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "")
	SHADER_PARAM(REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint")
	SHADER_PARAM(REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.8", "")
	SHADER_PARAM(REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint")
	SHADER_PARAM(NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "dev/water_normal", "normal map")
	SHADER_PARAM(BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap")
	SHADER_PARAM(BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform")
	SHADER_PARAM(TIME, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(WATERDEPTH, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(CHEAPWATERSTARTDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should start transitioning to a cheaper water shader.")
	SHADER_PARAM(CHEAPWATERENDDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should finish transitioning to a cheaper water shader.")
	SHADER_PARAM(ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "env_cubemap", "envmap")
	SHADER_PARAM(ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "")
	SHADER_PARAM(FOGCOLOR, SHADER_PARAM_TYPE_COLOR, "", "")
	SHADER_PARAM(FORCECHEAP, SHADER_PARAM_TYPE_BOOL, "", "")
	SHADER_PARAM(REFLECTENTITIES, SHADER_PARAM_TYPE_BOOL, "", "")
	SHADER_PARAM(FOGSTART, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FOGEND, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(ABOVEWATER, SHADER_PARAM_TYPE_BOOL, "", "")
	SHADER_PARAM(REFLECTBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "")
	SHADER_PARAM(NOFRESNEL, SHADER_PARAM_TYPE_BOOL, "0", "")
	SHADER_PARAM(NOLOWENDLIGHTMAP, SHADER_PARAM_TYPE_BOOL, "0", "")
	SHADER_PARAM(SCROLL1, SHADER_PARAM_TYPE_COLOR, "", "")
	SHADER_PARAM(SCROLL2, SHADER_PARAM_TYPE_COLOR, "", "")
	SHADER_PARAM(FLOWMAP, SHADER_PARAM_TYPE_TEXTURE, "", "flowmap")
	SHADER_PARAM(FLOWMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $flowmap")
	SHADER_PARAM(FLOWMAPSCROLLRATE, SHADER_PARAM_TYPE_VEC2, "[0 0", "2D rate to scroll $flowmap")
	SHADER_PARAM(FLOW_NOISE_TEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "flow noise texture")

	SHADER_PARAM(FLASHLIGHTTINT, SHADER_PARAM_TYPE_FLOAT, "0", "")
	SHADER_PARAM(LIGHTMAPWATERFOG, SHADER_PARAM_TYPE_BOOL, "0", "")
	SHADER_PARAM(FORCEFRESNEL, SHADER_PARAM_TYPE_FLOAT, "0", "")

	// New flow params
	SHADER_PARAM(FLOW_WORLDUVSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_NORMALUVSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_TIMEINTERVALINSECONDS, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_UVSCROLLDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_BUMPSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_TIMESCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_NOISE_SCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(FLOW_DEBUG, SHADER_PARAM_TYPE_BOOL, "0", "")

	SHADER_PARAM(COLOR_FLOW_UVSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(COLOR_FLOW_TIMESCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(COLOR_FLOW_TIMEINTERVALINSECONDS, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(COLOR_FLOW_UVSCROLLDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(COLOR_FLOW_DISPLACEBYNORMALSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "", "")
	SHADER_PARAM(COLOR_FLOW_LERPEXP, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(MRAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Texture with metalness in R, roughness in G, ambient occlusion in B.")

END_SHADER_PARAMS




void SetupParmsGBuffer0(defParms_gBuffer_translucent& p)
{
	p.bModel = false;

	p.iAlbedo = BASETEXTURE;
#if DEFCFG_DEFERRED_SHADING == 1
	p.iAlbedo2 = BASETEXTURE2;
#endif
	p.iBumpmap = NORMALMAP;
	p.iBumpmap2 = 0;
	p.PARALLAX = 0;

	p.iSSBump = 0;
	//p.iAlphatestRef = ALPHATESTREFERENCE;

	p.m_nMRAO = MRAOTEXTURE;
	p.m_nAlpha = 0;
	//p.m_nTransparency = TRANSPARENCY;
	//p.m_nTrasncluent = TRANSLUCENT;

	p.FLOWMAP = FLOWMAP;
	p.FLOWMAPFRAME = FLOWMAPFRAME;
	p.FLOW_NOISE_TEXTURE = FLOW_NOISE_TEXTURE;

	p.FLOW_WORLDUVSCALE = FLOW_WORLDUVSCALE;
	p.FLOW_NORMALUVSCALE = FLOW_NORMALUVSCALE;
	p.FLOW_BUMPSTRENGTH = FLOW_BUMPSTRENGTH;
	p.COLOR_FLOW_DISPLACEBYNORMALSTRENGTH = COLOR_FLOW_DISPLACEBYNORMALSTRENGTH;

	p.FLOW_TIMEINTERVALINSECONDS = FLOW_TIMEINTERVALINSECONDS;
	p.FLOW_UVSCROLLDISTANCE = FLOW_UVSCROLLDISTANCE;
	p.FLOW_NOISE_SCALE = FLOW_NOISE_SCALE;

	p.COLOR_FLOW_UVSCALE = COLOR_FLOW_UVSCALE;
	p.COLOR_FLOW_TIMEINTERVALINSECONDS = COLOR_FLOW_TIMEINTERVALINSECONDS;
	p.COLOR_FLOW_UVSCROLLDISTANCE = COLOR_FLOW_UVSCROLLDISTANCE;
	p.COLOR_FLOW_LERPEXP = COLOR_FLOW_LERPEXP;

	p.REFRACTTINT = REFRACTTINT;
	p.REFLECTAMOUNT = REFLECTAMOUNT;
	p.REFRACTAMOUNT = REFRACTAMOUNT;
	p.FOGCOLOR = FOGCOLOR;
	p.REFLECTTINT = REFLECTTINT;
	//p.WATERBLENDFACTOR = WATERBLENDFACTOR;
	p.FOGSTART = FOGSTART;
	p.FOGEND = FOGEND;

}

void SetupParmsShadow(defParms_shadow& p)
{
	p.bModel = false;
	p.iAlbedo = BASETEXTURE;


	//p.iAlphatestRef = ALPHATESTREFERENCE;
}

void SetupParamsWater(defParms_Water& info)
{
	info.BASETEXTURE = BASETEXTURE;
	info.REFRACTTEXTURE = REFRACTTEXTURE;
	//info.SCENEDEPTH = SCENEDEPTH;
	info.REFLECTTEXTURE = REFLECTTEXTURE;
	info.REFRACTAMOUNT = REFRACTAMOUNT;
	info.REFRACTTINT = REFRACTTINT;
	info.REFLECTAMOUNT = REFLECTAMOUNT;
	info.REFLECTTINT = REFLECTTINT;
	info.NORMALMAP = NORMALMAP;
	info.BUMPFRAME = BUMPFRAME;
	info.BUMPTRANSFORM = BUMPTRANSFORM;
	info.TIME = TIME;
	info.WATERDEPTH = WATERDEPTH;
	info.CHEAPWATERSTARTDISTANCE = CHEAPWATERSTARTDISTANCE;
	info.CHEAPWATERENDDISTANCE = CHEAPWATERENDDISTANCE;
	info.ENVMAP = ENVMAP;
	info.ENVMAPFRAME = ENVMAPFRAME;
	info.FOGCOLOR = FOGCOLOR;
	info.FORCECHEAP = FORCECHEAP;
	info.REFLECTENTITIES = REFLECTENTITIES;
	info.FOGSTART = FOGSTART;
	info.FOGEND = FOGEND;
	info.ABOVEWATER = ABOVEWATER;
	//info.WATERBLENDFACTOR = WATERBLENDFACTOR;
	info.NOFRESNEL = NOFRESNEL;
	info.NOLOWENDLIGHTMAP = NOLOWENDLIGHTMAP;
	info.SCROLL1 = SCROLL1;
	info.SCROLL2 = SCROLL2;
	info.FLASHLIGHTTINT = FLASHLIGHTTINT;
	info.LIGHTMAPWATERFOG = LIGHTMAPWATERFOG;
	info.FORCEFRESNEL = FORCEFRESNEL;
	///info.FORCEENVMAP = FORCEENVMAP;
	//info.DEPTH_FEATHER = DEPTH_FEATHER;
	info.FLOWMAP = FLOWMAP;
	info.FLOWMAPFRAME = FLOWMAPFRAME;
	info.FLOWMAPSCROLLRATE = FLOWMAPSCROLLRATE;
	info.FLOW_NOISE_TEXTURE = FLOW_NOISE_TEXTURE;
	info.FLOW_WORLDUVSCALE = FLOW_WORLDUVSCALE;
	info.FLOW_NORMALUVSCALE = FLOW_NORMALUVSCALE;
	info.FLOW_TIMEINTERVALINSECONDS = FLOW_TIMEINTERVALINSECONDS;
	info.FLOW_UVSCROLLDISTANCE = FLOW_UVSCROLLDISTANCE;
	info.FLOW_BUMPSTRENGTH = FLOW_BUMPSTRENGTH;
	info.FLOW_NOISE_SCALE = FLOW_NOISE_SCALE;
	info.FLOW_DEBUG = FLOW_DEBUG;
	info.COLOR_FLOW_UVSCALE = COLOR_FLOW_UVSCALE;
	info.COLOR_FLOW_TIMEINTERVALINSECONDS = COLOR_FLOW_TIMEINTERVALINSECONDS;
	info.COLOR_FLOW_UVSCROLLDISTANCE = COLOR_FLOW_UVSCROLLDISTANCE;
	info.COLOR_FLOW_LERPEXP = COLOR_FLOW_LERPEXP;
	
	info.COLOR_FLOW_DISPLACEBYNORMALSTRENGTH = COLOR_FLOW_DISPLACEBYNORMALSTRENGTH;

	info.MRAO = MRAOTEXTURE;
	
	info.REFLECTBLENDFACTOR = REFLECTBLENDFACTOR;
	info.FLOW_TIMESCALE = FLOW_TIMESCALE;
	info.COLOR_FLOW_TIMESCALE = COLOR_FLOW_TIMESCALE;

}


bool DrawToGBuffer(IMaterialVar** params)
{
	const bool bIsDecal = IS_FLAG_SET(MATERIAL_VAR_DECAL);
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);

	return !bTranslucent && !bIsDecal;
}
SHADER_INIT_PARAMS()
{
	// for fallback shaders
	// SWARMS VBSP BETTER FUCKING CALL MODINIT NOW
	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_HW_SKINNING);

	const bool bDrawToGBuffer = DrawToGBuffer(params);

	if (bDrawToGBuffer)
	{
		defParms_gBuffer_translucent parms_gbuffer;
		SetupParmsGBuffer0(parms_gbuffer);
		InitParmsGBuffer_translucent(parms_gbuffer, this, params);

		defParms_shadow parms_shadow;
		SetupParmsShadow(parms_shadow);
		InitParmsShadowPass(parms_shadow, this, params);
	}

	defParms_Water parms_compositePBR;
	SetupParamsWater(parms_compositePBR);
	InitParamsWater_DX9(this, params, pMaterialName, parms_compositePBR);
}

SHADER_INIT
{
	if (stricmp(params[ENVMAP]->GetStringValue(), "env_cubemap") == 0)
	{
		Warning("env_cubemap used on world geometry without rebuilding map. . ignoring: %s\n", pMaterialName);
		params[ENVMAP]->SetUndefined();
	}

	const bool bDrawToGBuffer = DrawToGBuffer(params);


	if (bDrawToGBuffer)
	{
		defParms_gBuffer_translucent parms_gbuffer;
		SetupParmsGBuffer0(parms_gbuffer);
		InitPassGBuffer_translucent(parms_gbuffer, this, params);

		defParms_shadow parms_shadow;
		SetupParmsShadow(parms_shadow);
		InitPassShadowPass(parms_shadow, this, params);
	}

	defParms_Water parms_compositePBR;
	SetupParamsWater(parms_compositePBR);
	InitWater_DX9(this, params, parms_compositePBR);
}

SHADER_FALLBACK
{
return 0;
}

SHADER_DRAW
{

	if (pShaderAPI != NULL && *pContextDataPtr == NULL)
		*pContextDataPtr = new CDeferredPerMaterialContextData();

	CDeferredPerMaterialContextData* pDefContext = reinterpret_cast<CDeferredPerMaterialContextData*>(*pContextDataPtr);

	const int iDeferredRenderStage = pShaderAPI ?
		pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_DEFERRED_RENDER_STAGE)
		: DEFERRED_RENDER_STAGE_INVALID;

	const bool bDrawToGBuffer = DrawToGBuffer(params);
	//const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);

	Assert(pShaderAPI == NULL ||
		iDeferredRenderStage != DEFERRED_RENDER_STAGE_INVALID);

	if (bDrawToGBuffer)
	{
		if (pShaderShadow != NULL ||
			iDeferredRenderStage == DEFERRED_RENDER_STAGE_GBUFFER_WATER)
		{
			defParms_gBuffer_translucent parms_gbuffer;
			SetupParmsGBuffer0(parms_gbuffer);
			DrawPassGBuffer_translucent(parms_gbuffer, this, params, pShaderShadow, pShaderAPI,
				vertexCompression, pDefContext);
		}
		else
			SkipPass();

		if (pShaderShadow != NULL ||
			iDeferredRenderStage == DEFERRED_RENDER_STAGE_SHADOWPASS)
		{
			defParms_shadow parms_shadow;
			SetupParmsShadow(parms_shadow);
			DrawPassShadowPass(parms_shadow, this, params, pShaderShadow, pShaderAPI,
				vertexCompression, pDefContext);
		}
		else
			SkipPass();
	}

	if (pShaderShadow != NULL ||
		iDeferredRenderStage == DEFERRED_RENDER_STAGE_COMPOSITION)
	{
		defParms_Water parms_compositePBR;
		SetupParamsWater(parms_compositePBR);
		DrawWater_DX9(parms_compositePBR, this, params, pShaderShadow, pShaderAPI, vertexCompression, pDefContext);
	}
	else
		Draw(false);

	if (pShaderAPI != NULL && pDefContext->m_bMaterialVarsChanged)
		pDefContext->m_bMaterialVarsChanged = false;
}

END_SHADER
