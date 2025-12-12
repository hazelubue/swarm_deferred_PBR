#include "deferred_includes.h"

#include "tier0/memdbgon.h"


BEGIN_VS_SHADER(DEFERRED_BRUSH, "")
BEGIN_SHADER_PARAMS

SHADER_PARAM(BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(SSBUMP, SHADER_PARAM_TYPE_BOOL, "", "")

SHADER_PARAM(MRAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "")

SHADER_PARAM(LITFACE, SHADER_PARAM_TYPE_BOOL, "0", "")

SHADER_PARAM(PHONG_SCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(PHONG_EXP, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(PHONG_EXP2, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(PHONG_MAP, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(PHONG_FRESNEL, SHADER_PARAM_TYPE_BOOL, "", "")

SHADER_PARAM(FRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "", "")
SHADER_PARAM(ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "", "")

SHADER_PARAM(ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap")
SHADER_PARAM(ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint")
SHADER_PARAM(ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color")
SHADER_PARAM(ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal")
SHADER_PARAM(ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask")

SHADER_PARAM(BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(BUMPMAP2, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(ENVMAPMASK2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask")
SHADER_PARAM(BLENDMODULATETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "texture to use r/g channels for blend range for")
SHADER_PARAM(BLENDMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$blendmodulatetexture texcoord transform")

SHADER_PARAM(MULTIBLEND, SHADER_PARAM_TYPE_BOOL, "", "")
SHADER_PARAM(BASETEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(BASETEXTURE4, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(BUMPMAP3, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(BUMPMAP4, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(BLENDMODULATETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "texture to use r/g channels for blend range for")
SHADER_PARAM(BLENDMODULATETEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "", "texture to use r/g channels for blend range for")
SHADER_PARAM(BLENDMASKTRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$blendmodulatetexture texcoord transform")
SHADER_PARAM(BLENDMASKTRANSFORM3, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$blendmodulatetexture texcoord transform")

SHADER_PARAM(FLOWMAP, SHADER_PARAM_TYPE_TEXTURE, "", "flowmap")
SHADER_PARAM(FLOWMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $flowmap")
SHADER_PARAM(FLOW_NOISE_TEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "flow noise texture")
SHADER_PARAM(FLOW_WORLDUVSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(FLOW_NORMALUVSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(FLOW_BUMPSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(COLOR_FLOW_DISPLACEBYNORMALSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(FLOW_TIMEINTERVALINSECONDS, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(FLOW_UVSCROLLDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(FLOW_NOISE_SCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(COLOR_FLOW_UVSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(COLOR_FLOW_TIMEINTERVALINSECONDS, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(COLOR_FLOW_UVSCROLLDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(COLOR_FLOW_LERPEXP, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint")
SHADER_PARAM(REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.8", "")
SHADER_PARAM(REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "")
SHADER_PARAM(FOGCOLOR, SHADER_PARAM_TYPE_COLOR, "", "")
SHADER_PARAM(REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint")
SHADER_PARAM(WATERBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "")
SHADER_PARAM(FOGSTART, SHADER_PARAM_TYPE_FLOAT, "", "")
SHADER_PARAM(FOGEND, SHADER_PARAM_TYPE_FLOAT, "", "")

SHADER_PARAM(PARALLAX, SHADER_PARAM_TYPE_FLOAT, "", "");

SHADER_PARAM(ALPHATEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "a alpha texture!");
SHADER_PARAM(TRANSPARENCY, SHADER_PARAM_TYPE_INTEGER, "", "");
SHADER_PARAM(TRANSLUCENT, SHADER_PARAM_TYPE_INTEGER, "", "");
SHADER_PARAM(SELFILLUM, SHADER_PARAM_TYPE_INTEGER, "", "");
END_SHADER_PARAMS

void SetupParmsGBuffer0(defParms_gBuffer0& p)
{
	p.bModel = false;
	p.bWater = false;

	p.iAlbedo = BASETEXTURE;
#if DEFCFG_DEFERRED_SHADING == 1
	p.iAlbedo2 = BASETEXTURE2;
#endif
	p.iBumpmap = BUMPMAP;
	p.iBumpmap2 = BUMPMAP2;
	p.PARALLAX = PARALLAX;

	p.iSSBump = SSBUMP;
	p.iAlphatestRef = ALPHATESTREFERENCE;

	p.m_nMRAO = MRAOTEXTURE;
	p.m_nAlpha = ALPHATEXTURE;
	p.m_nTransparency = TRANSPARENCY;
	p.m_nTrasncluent = TRANSLUCENT;

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
	p.WATERBLENDFACTOR = WATERBLENDFACTOR;
	p.FOGSTART = FOGSTART;
	p.FOGEND = FOGEND;

}

void SetupParmsShadow(defParms_shadow& p)
{
	p.bModel = false;
	p.iAlbedo = BASETEXTURE;
	p.iAlphatestRef = ALPHATESTREFERENCE;
	p.iMultiblend = MULTIBLEND;
}

void SetupParmsComposite(defParms_composite& p)
{
	p.bModel = false;
	p.iAlbedo = BASETEXTURE;
	p.iAlbedo2 = BASETEXTURE2;
	p.iAlbedo3 = BASETEXTURE3;
	p.iAlbedo4 = BASETEXTURE4;

	p.iEnvmap = ENVMAP;
	p.iEnvmapMask = ENVMAPMASK;
	p.iEnvmapMask2 = 0;
	p.iEnvmapTint = ENVMAPTINT;
	p.iEnvmapContrast = ENVMAPCONTRAST;
	p.iEnvmapSaturation = ENVMAPSATURATION;

	p.iAlphatestRef = ALPHATESTREFERENCE;

	p.iPhongScale = 0;
	p.iPhongFresnel = 0;

	p.iBlendmodulate = BLENDMODULATETEXTURE;
	p.iBlendmodulate2 = BLENDMODULATETEXTURE2;
	p.iBlendmodulate3 = BLENDMODULATETEXTURE3;
	p.iBlendmodulateTransform = BLENDMASKTRANSFORM;
	p.iBlendmodulateTransform2 = BLENDMASKTRANSFORM2;
	p.iBlendmodulateTransform3 = BLENDMASKTRANSFORM3;
	p.iMultiblend = MULTIBLEND;

	p.SelfShadowedBumpFlag = SSBUMP;
	p.BUMPMAP = BUMPMAP;

	p.MRAOTEXTURE = MRAOTEXTURE;
	p.SELFILLUM = SELFILLUM;

	p.iFresnelRanges = FRESNELRANGES;
}

void SetupParmsComposite_translucent(defParms_composite_translucent& p)
{
	p.bModel = false;
	p.iAlbedo = BASETEXTURE;
	p.iAlbedo2 = BASETEXTURE2;
	p.iAlbedo3 = BASETEXTURE3;
	p.iAlbedo4 = BASETEXTURE4;

	p.iEnvmap = ENVMAP;
	p.iEnvmapMask = ENVMAPMASK;
	p.iEnvmapMask2 = 0;
	p.iEnvmapTint = ENVMAPTINT;
	p.iEnvmapContrast = ENVMAPCONTRAST;
	p.iEnvmapSaturation = ENVMAPSATURATION;

	p.iAlphatestRef = ALPHATESTREFERENCE;

	p.iPhongScale = 0;
	p.iPhongFresnel = 0;

	p.iBlendmodulate = BLENDMODULATETEXTURE;
	p.iBlendmodulate2 = BLENDMODULATETEXTURE2;
	p.iBlendmodulate3 = BLENDMODULATETEXTURE3;
	p.iBlendmodulateTransform = BLENDMASKTRANSFORM;
	p.iBlendmodulateTransform2 = BLENDMASKTRANSFORM2;
	p.iBlendmodulateTransform3 = BLENDMASKTRANSFORM3;
	p.iMultiblend = MULTIBLEND;

	p.SelfShadowedBumpFlag = SSBUMP;
	p.BUMPMAP = BUMPMAP;

	p.MRAOTEXTURE = MRAOTEXTURE;

	p.iFresnelRanges = FRESNELRANGES;
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
	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);
	if (PARM_DEFINED(BUMPMAP))
		SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);

	//const bool bDrawToGBuffer = DrawToGBuffer(params);
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
	bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();

	if (bDeferredActive)
	{
		defParms_gBuffer0 parms_gbuffer;
		SetupParmsGBuffer0(parms_gbuffer);
		InitParmsGBuffer(parms_gbuffer, this, params);

		defParms_shadow parms_shadow;
		SetupParmsShadow(parms_shadow);
		InitParmsShadowPass(parms_shadow, this, params);
	}

	if (!bTranslucent)
	{
		defParms_composite parms_composite;
		SetupParmsComposite(parms_composite);
		InitParmsComposite(parms_composite, this, params);
	}
	else
	{
		defParms_composite_translucent parms_composite_translucent;
		SetupParmsComposite_translucent(parms_composite_translucent);
		InitParmsComposite_translucent(parms_composite_translucent, this, params);
	}
	
}

SHADER_INIT
{
	//const bool bDrawToGBuffer = DrawToGBuffer(params);
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
	bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();

	if (bDeferredActive)
	{
		defParms_gBuffer0 parms_gbuffer;
		SetupParmsGBuffer0(parms_gbuffer);
		InitPassGBuffer(parms_gbuffer, this, params);

		defParms_shadow parms_shadow;
		SetupParmsShadow(parms_shadow);
		InitPassShadowPass(parms_shadow, this, params);
	}

	if (!bTranslucent)
	{
		defParms_composite parms_composite;
		SetupParmsComposite(parms_composite);
		InitPassComposite(parms_composite, this, params);
	}
	else
	{
		defParms_composite_translucent parms_composite_translucent;
		SetupParmsComposite_translucent(parms_composite_translucent);
		InitPassComposite_translucent(parms_composite_translucent, this, params);
	}
	
}

SHADER_FALLBACK
{
	if (!GetDeferredExt()->IsDeferredLightingEnabled())
	{
		if (PARM_SET(MULTIBLEND))
			return "MultiBlend";

		return "LightmappedGeneric";
	}

	//const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
	const bool bIsDecal = IS_FLAG_SET(MATERIAL_VAR_DECAL);

	if (bIsDecal)
		return "LightmappedGeneric";

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

	//const bool bDrawToGBuffer = DrawToGBuffer(params);
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
	bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();

	Assert(pShaderAPI == NULL ||
		iDeferredRenderStage != DEFERRED_RENDER_STAGE_INVALID);

	if (bDeferredActive)
	{
		if (pShaderShadow != NULL ||
			iDeferredRenderStage == DEFERRED_RENDER_STAGE_GBUFFER)
		{
			defParms_gBuffer0 parms_gbuffer;
			SetupParmsGBuffer0(parms_gbuffer);
			DrawPassGBuffer(parms_gbuffer, this, params, pShaderShadow, pShaderAPI,
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

	if (bTranslucent)
	{
		if (pShaderShadow != NULL ||
		iDeferredRenderStage == DEFERRED_RENDER_STAGE_COMPOSITION)
		{
			defParms_composite_translucent parms_composite_translucent;
			SetupParmsComposite_translucent(parms_composite_translucent);
			DrawPassComposite_translucent(parms_composite_translucent, this, params, pShaderShadow, pShaderAPI,
				vertexCompression, pDefContext);
		}
		else
			SkipPass();
	}
	else
	{
		if (pShaderShadow != NULL ||
			iDeferredRenderStage == DEFERRED_RENDER_STAGE_COMPOSITION)
		{
			defParms_composite parms_composite;
			SetupParmsComposite(parms_composite);
			DrawPassComposite(parms_composite, this, params, pShaderShadow, pShaderAPI,
				vertexCompression, pDefContext);
		}
		else
			SkipPass();
	}
	
	if (pShaderAPI != NULL && pDefContext->m_bMaterialVarsChanged)
		pDefContext->m_bMaterialVarsChanged = false;
}

END_SHADER