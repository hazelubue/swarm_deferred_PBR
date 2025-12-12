
#include "deferred_includes.h"

#include "tier0/memdbgon.h"


BEGIN_VS_SHADER( DEFERRED_MODEL, "" )
	BEGIN_SHADER_PARAMS

		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( MRAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "")

		SHADER_PARAM( LITFACE, SHADER_PARAM_TYPE_BOOL, "0", "" )

		SHADER_PARAM( PHONG_SCALE, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( PHONG_EXP, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( PHONG_MAP, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( PHONG_FRESNEL, SHADER_PARAM_TYPE_BOOL, "", "" )

		SHADER_PARAM( FRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "", "" )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.5", "" )

		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( ENVMAPFRESNEL, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )

		SHADER_PARAM( RIMLIGHT, SHADER_PARAM_TYPE_BOOL, "0", "enables rim lighting" )
		SHADER_PARAM( RIMLIGHTEXPONENT, SHADER_PARAM_TYPE_FLOAT, "4.0", "Exponent for rim lights" )
		SHADER_PARAM( RIMLIGHTALBEDOSCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "Albedo influence on rim light" )
		SHADER_PARAM( RIMLIGHTTINT, SHADER_PARAM_TYPE_VEC3, "[1 1 1]", "Tint for rim lights" )
		SHADER_PARAM( RIMLIGHT_MODULATE_BY_LIGHT, SHADER_PARAM_TYPE_BOOL, "0", "Modulate rimlight by lighting." )

		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( SELFILLUM_ENVMAPMASK_ALPHA, SHADER_PARAM_TYPE_BOOL, "0", "defines that self illum value comes from env map mask alpha" )
		SHADER_PARAM( SELFILLUMFRESNEL, SHADER_PARAM_TYPE_BOOL, "0", "Self illum fresnel" )
		SHADER_PARAM( SELFILLUMMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "If we bind a texture here, it overrides base alpha (if any) for self illum" )

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

		p.iAlbedo = BASETEXTURE;
	#if DEFCFG_DEFERRED_SHADING == 1
		p.iAlbedo2 = BASETEXTURE2;
	#endif
		p.iBumpmap = BUMPMAP;
		//p.iBumpmap2 = BUMPMAP2;
		p.PARALLAX = PARALLAX;
		//p.iPhongmap = PHONG_MAP;
		//p.iSSBump = SSBUMP;
		p.iAlphatestRef = ALPHATESTREFERENCE;
		/*p.iLitface = LITFACE;
		p.iPhongExp = PHONG_EXP;
		p.iPhongExp2 = PHONG_EXP2;

		p
		p.iBlendmodulate = BLENDMODULATETEXTURE;
		p.iBlendmodulateTransform = BLENDMASKTRANSFORM;

		p.m_nRoughness = ROUGHNESS;

		p.m_nMetallic = METALLIC;*/
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
		//p.REFRACTTEXTURE = REFRACTTEXTURE;
		p.REFLECTAMOUNT = REFLECTAMOUNT;
		p.REFRACTAMOUNT = REFRACTAMOUNT;
		p.FOGCOLOR = FOGCOLOR;
		p.REFLECTTINT = REFLECTTINT;
		p.WATERBLENDFACTOR = WATERBLENDFACTOR;
		p.FOGSTART = FOGSTART;
		p.FOGEND = FOGEND;

		//p.Envmap = 0;

		p.bWater = false;
		p.bModel = true;
	}

	void SetupParmsShadow( defParms_shadow &p )
	{
		p.bModel = true;
		p.iAlbedo = BASETEXTURE;
		p.iAlphatestRef = ALPHATESTREFERENCE;
	}

	void SetupParmsComposite(defParms_composite& p)
	{
		p.bModel = true;
		p.iAlbedo = BASETEXTURE;

		p.iEnvmap = ENVMAP;
		p.iEnvmapMask = ENVMAPMASK;
		p.iEnvmapTint = ENVMAPTINT;
		p.iEnvmapContrast = ENVMAPCONTRAST;
		p.iEnvmapSaturation = ENVMAPSATURATION;
		p.iEnvmapFresnel = ENVMAPFRESNEL;

		p.iRimlightEnable = RIMLIGHT;
		p.iRimlightExponent = RIMLIGHTEXPONENT;
		p.iRimlightAlbedoScale = RIMLIGHTALBEDOSCALE;
		p.iRimlightTint = RIMLIGHTTINT;
		p.iRimlightModLight = RIMLIGHT_MODULATE_BY_LIGHT;

		p.iAlphatestRef = ALPHATESTREFERENCE;

		p.MRAOTEXTURE = MRAOTEXTURE;

		p.iPhongScale = 0;
		p.iPhongFresnel = 0;

		p.iSelfIllumTint = SELFILLUMTINT;
		p.iSelfIllumMaskInEnvmapAlpha = SELFILLUM_ENVMAPMASK_ALPHA;
		p.iSelfIllumFresnelModulate = SELFILLUMFRESNEL;
		p.iSelfIllumMask = SELFILLUMMASK;

		//p.SelfShadowedBumpFlag = SSBUMP;
		p.BUMPMAP = BUMPMAP;
		p.SELFILLUM = SELFILLUM;

		p.iFresnelRanges = FRESNELRANGES;
	}

	void SetupParmsComposite_translucent(defParms_composite_translucent& p)
	{
		p.bModel = true;
		p.iAlbedo = BASETEXTURE;

		p.iEnvmap = ENVMAP;
		p.iEnvmapMask = ENVMAPMASK;
		p.iEnvmapMask2 = 0;
		p.iEnvmapTint = ENVMAPTINT;
		p.iEnvmapContrast = ENVMAPCONTRAST;
		p.iEnvmapSaturation = ENVMAPSATURATION;

		p.iAlphatestRef = ALPHATESTREFERENCE;

		p.iPhongScale = 0;
		p.iPhongFresnel = 0;

		p.BUMPMAP = BUMPMAP;

		p.MRAOTEXTURE = MRAOTEXTURE;

		p.iFresnelRanges = FRESNELRANGES;
	}


	bool DrawToGBuffer( IMaterialVar **params )
	{

		const bool bIsDecal = IS_FLAG_SET( MATERIAL_VAR_DECAL );
		const bool bTranslucent = IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT );

		return !bTranslucent && !bIsDecal;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if ( g_pHardwareConfig->HasFastVertexTextures() )
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

		//const bool bDrawToGBuffer = DrawToGBuffer( params );
		const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
		bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();

		if (bDeferredActive)
		{
			defParms_gBuffer0 parms_gbuffer;
			SetupParmsGBuffer0( parms_gbuffer );
			InitParmsGBuffer( parms_gbuffer, this, params );
		
			defParms_shadow parms_shadow;
			SetupParmsShadow( parms_shadow );
			InitParmsShadowPass( parms_shadow, this, params );
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
		//const bool bDrawToGBuffer = DrawToGBuffer( params );
		const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
		bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();
		
		if (bDeferredActive)
		{
			defParms_gBuffer0 parms_gbuffer;
			SetupParmsGBuffer0( parms_gbuffer );
			InitPassGBuffer( parms_gbuffer, this, params );
		
			defParms_shadow parms_shadow;
			SetupParmsShadow( parms_shadow );
			InitPassShadowPass( parms_shadow, this, params );
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
		if ( !GetDeferredExt()->IsDeferredLightingEnabled() )
			return "VertexlitGeneric";

		//const bool bTranslucent = IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT );
		const bool bIsDecal = IS_FLAG_SET( MATERIAL_VAR_DECAL );

		if ( bIsDecal )
			return "VertexlitGeneric";

		return 0;
	}

	SHADER_DRAW
	{
		if ( pShaderAPI != NULL && *pContextDataPtr == NULL )
			*pContextDataPtr = new CDeferredPerMaterialContextData();

		CDeferredPerMaterialContextData *pDefContext = reinterpret_cast< CDeferredPerMaterialContextData* >(*pContextDataPtr);

		const int iDeferredRenderStage = pShaderAPI ?
			pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_DEFERRED_RENDER_STAGE )
			: DEFERRED_RENDER_STAGE_INVALID;

		//const bool bDrawToGBuffer = DrawToGBuffer( params );
		const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);

		Assert( pShaderAPI == NULL ||
			iDeferredRenderStage != DEFERRED_RENDER_STAGE_INVALID );

		bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();

		if (bDeferredActive)
		{
			if ( pShaderShadow != NULL ||
				iDeferredRenderStage == DEFERRED_RENDER_STAGE_GBUFFER )
			{
				defParms_gBuffer0 parms_gbuffer;
				SetupParmsGBuffer0( parms_gbuffer );
				DrawPassGBuffer( parms_gbuffer, this, params, pShaderShadow, pShaderAPI,
					vertexCompression, pDefContext );
			}
			else
				SkipPass();

			if ( pShaderShadow != NULL ||
				iDeferredRenderStage == DEFERRED_RENDER_STAGE_SHADOWPASS )
			{
				defParms_shadow parms_shadow;
				SetupParmsShadow( parms_shadow );
				DrawPassShadowPass( parms_shadow, this, params, pShaderShadow, pShaderAPI,
					vertexCompression, pDefContext );
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

		if ( pShaderAPI != NULL && pDefContext->m_bMaterialVarsChanged )
			pDefContext->m_bMaterialVarsChanged = false;
	}

END_SHADER