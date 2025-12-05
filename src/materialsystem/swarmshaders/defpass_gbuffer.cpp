
#include "BaseVSShader.h"
#include "convar.h"

#include "deferred_includes.h"

#include "include/gbuffer_vs30.inc"
#include "include/gbuffer_ps30.inc"
#include "include/gbuffer_translucent_ps30.inc"
#include "shaderapi\ishaderapi.h"
#include "tier0/memdbgon.h"

//BEGIN_VS_SHADER(DEFPASS_GBUFFER,
//	"Help for LightmappedGeneric")
//	BEGIN_SHADER_PARAMS
//
//	SHADER_PARAM(MRAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_MRAO", "Texture with metalness in R, roughness in G, ambient occlusion in B.");
//
//END_SHADER_PARAMS

const int PARALLAX_QUALITY_MAX = 3;

static ConVar mat_pbr_parallaxdepth("mat_pbr_parallaxdepth", ".1"); // 0.04
static ConVar mat_pbr_parallaxCenter("mat_pbr_parallaxCenter", ".9");
static ConVar mat_pbr_parallaxmap_quality("mat_pbr_parallaxmap_quality", "100", FCVAR_NONE, "", true, 0, true, PARALLAX_QUALITY_MAX);
static ConVar mat_pbr_parallaxmap("mat_pbr_parallaxmap", "1");
static ConVar mat_pbr_force_20b("mat_pbr_force_20b", "0", FCVAR_CHEAT);
static ConVar mat_pbr_iblIntensity("mat_pbr_iblIntensity", "1000.0", FCVAR_CHEAT);


static CCommandBufferBuilder< CFixedCommandStorageBuffer< 512 > > tmpBuf;

static void UTIL_StringToFloatArray(float* pVector, int count, const char* pString)
{
	char* pstr, * pfront, tempString[128];
	int	j;

	Q_strncpy(tempString, pString, sizeof(tempString));
	pstr = pfront = tempString;

	for (j = 0; j < count; j++)			// lifted from pr_edict.c
	{
		pVector[j] = atof(pfront);

		// skip any leading whitespace
		while (*pstr && *pstr <= ' ')
			pstr++;

		// skip to next whitespace
		while (*pstr && *pstr > ' ')
			pstr++;

		if (!*pstr)
			break;

		pstr++;
		pfront = pstr;
	}
	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}

void InitParmsGBuffer(const defParms_gBuffer0& info, CBaseVSShader* pShader, IMaterialVar** params)
{
	if (PARM_NO_DEFAULT(info.iAlphatestRef) ||
		(PARM_VALID(info.iAlphatestRef) && PARM_FLOAT(info.iAlphatestRef) == 0.0f))
		params[info.iAlphatestRef]->SetFloatValue(DEFAULT_ALPHATESTREF);

	PARM_INIT_FLOAT(info.iPhongExp, DEFAULT_PHONG_EXP);
	PARM_INIT_FLOAT(info.iPhongExp2, DEFAULT_PHONG_EXP);

	InitIntParam(info.m_nTreeSway, params, 0);
	InitFloatParam(info.m_nTreeSwayHeight, params, 1000.0f);
	InitFloatParam(info.m_nTreeSwayStartHeight, params, 0.1f);
	InitFloatParam(info.m_nTreeSwayRadius, params, 300.0f);
	InitFloatParam(info.m_nTreeSwayStartRadius, params, 0.2f);
	InitFloatParam(info.m_nTreeSwaySpeed, params, 1.0f);
	InitFloatParam(info.m_nTreeSwaySpeedHighWindMultiplier, params, 2.0f);
	InitFloatParam(info.m_nTreeSwayStrength, params, 10.0f);
	InitFloatParam(info.m_nTreeSwayScrumbleSpeed, params, 5.0f);
	InitFloatParam(info.m_nTreeSwayScrumbleStrength, params, 10.0f);
	InitFloatParam(info.m_nTreeSwayScrumbleFrequency, params, 12.0f);
	InitFloatParam(info.m_nTreeSwayFalloffExp, params, 1.5f);
	InitFloatParam(info.m_nTreeSwayScrumbleFalloffExp, params, 1.0f);
	InitFloatParam(info.m_nTreeSwaySpeedLerpStart, params, 3.0f);
	InitFloatParam(info.m_nTreeSwaySpeedLerpEnd, params, 6.0f);

}

void InitPassGBuffer(const defParms_gBuffer0& info, CBaseVSShader* pShader, IMaterialVar** params)
{

	//bool bModel = info.bModel;
	if (PARM_DEFINED(info.iBumpmap))
		pShader->LoadBumpMap(info.iBumpmap);

	if (PARM_DEFINED(info.iBumpmap2))
		pShader->LoadBumpMap(info.iBumpmap2);
	if (PARM_DEFINED(info.m_nMRAO)) pShader->LoadTexture(info.m_nMRAO);

	if (PARM_DEFINED(info.iBlendmodulate)) pShader->LoadTexture(info.iBlendmodulate);

	if (PARM_DEFINED(info.iAlbedo)) pShader->LoadTexture(info.iAlbedo);

#if DEFCFG_DEFERRED_SHADING == 1
	if (PARM_DEFINED(info.iAlbedo2)) pShader->LoadTexture(info.iAlbedo2);
#endif


	if (params[info.m_nMRAO]->IsDefined())
	{
		pShader->LoadTexture(info.m_nMRAO);
	}
	//if (PARM_DEFINED(info.m_nAlpha)) pShader->LoadTexture(info.m_nAlpha);
}

void DrawPassGBuffer(const defParms_gBuffer0& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext)
{

	const bool bModel = info.bModel;
	//const bool bWater = info.bWater;

	const bool bIsDecal = IS_FLAG_SET(MATERIAL_VAR_DECAL);
	const bool bFastVTex = g_pHardwareConfig->HasFastVertexTextures();
	const bool bNoCull = IS_FLAG_SET(MATERIAL_VAR_NOCULL);

	const bool bAlbedo = PARM_TEX(info.iAlbedo);
	const bool bAlbedo2 = PARM_TEX(info.iAlbedo2);
	const bool bBumpmap = PARM_TEX(info.iBumpmap);
	const bool bBumpmap2 = bBumpmap && PARM_TEX(info.iBumpmap2);
	const bool bSpecular = PARM_TEX(info.iSpecularTexture);
	//const bool bAlpha = PARM_TEX(info.m_nAlpha);

	const bool bBlendmodulate = (bAlbedo2 || bBumpmap2) && PARM_TEX(info.iBlendmodulate);

	const int nTreeSwayMode = clamp(GetIntParam(info.m_nTreeSway, params, 0), 0, 2);
	const bool bTreeSway = nTreeSwayMode != 0;

	const bool bAlphatest = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) && bAlbedo;
	//const bool bOpaque = !IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);

	// here we are debugging what makes ONLY glass transparent truly to the gbuffer truth.
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT); /*|| PARM_DEFINED(info.m_nTrasncluent);*/

	const bool bSSBump = bBumpmap && PARM_SET(info.iSSBump);

	const bool useParallax = mat_pbr_parallaxmap.GetBool();
	bool bhasMRAO = IsTextureSet(info.m_nMRAO, params);

	bool bHasFlowmap = params[info.FLOWMAP]->IsTexture();

	//bool bHasCubemap = PARM_TEX(info.Envmap);


	//Assert(!bIsDecal || bDeferredShading);
	//Assert(!bTranslucent || bDeferredShading);

	SHADOW_STATE
	{
		pShaderShadow->SetDefaultState();
		/*if (bTranslucent)
		{
			pShaderShadow->EnableBlending(true);
		}*/

		pShaderShadow->EnableSRGBWrite(false);

		if (bNoCull)
		{
			pShaderShadow->EnableCulling(false);
		}

		int iVFmtFlags = VERTEX_POSITION | VERTEX_NORMAL;
		int iUserDataSize = 0;

		int* pTexCoordDim;
		int iTexCoordNum;
		GetTexcoordSettings((bModel && bIsDecal && bFastVTex), 0,
			iTexCoordNum, &pTexCoordDim);

		if (bModel)
		{
			iVFmtFlags |= VERTEX_FORMAT_COMPRESSED;
		}
		else
		{
			if (bBumpmap2 || bAlbedo2)
				iVFmtFlags |= VERTEX_COLOR;
		}


		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, false);


		pShaderShadow->EnableTexture(SHADER_SAMPLER8, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER8, false);


		pShaderShadow->EnableTexture(SHADER_SAMPLER7, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER7, false);

		if (bBumpmap)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, false);

			if (bModel)
				iUserDataSize = 4;
			else
			{
				iVFmtFlags |= VERTEX_TANGENT_SPACE;
			}
		}

			pShaderShadow->EnableTexture(SHADER_SAMPLER15, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER15, false);


		if (bAlbedo2 || bBumpmap2)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);
			if (bAlbedo2) pShaderShadow->EnableTexture(SHADER_SAMPLER9, true);

			if (bBlendmodulate)
				pShaderShadow->EnableTexture(SHADER_SAMPLER4, true);
		}

		if (bSpecular)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER5, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER5, false);
		}

		pShaderShadow->VertexShaderVertexFormat(iVFmtFlags, iTexCoordNum, pTexCoordDim, iUserDataSize);

		DECLARE_STATIC_VERTEX_SHADER(gbuffer_vs30);
		SET_STATIC_VERTEX_SHADER_COMBO(MODEL, bModel);
		SET_STATIC_VERTEX_SHADER_COMBO(MORPHING_VTEX, bModel && bFastVTex);
		SET_STATIC_VERTEX_SHADER_COMBO(TANGENTSPACE, bBumpmap);
		SET_STATIC_VERTEX_SHADER_COMBO(BUMPMAP2, bBumpmap2);
		SET_STATIC_VERTEX_SHADER_COMBO(BLENDMODULATE, bBlendmodulate);
		SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, nTreeSwayMode);
		SET_STATIC_VERTEX_SHADER(gbuffer_vs30);

		if (!bTranslucent)
		{
			DECLARE_STATIC_PIXEL_SHADER(gbuffer_ps30);
			SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAP2, bBumpmap2);
			SET_STATIC_PIXEL_SHADER_COMBO(ALPHATEST, bAlphatest);
			SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAP, bBumpmap ? bSSBump ? 2 : 1 : 0);
			SET_STATIC_PIXEL_SHADER_COMBO(NOCULL, bNoCull);
			SET_STATIC_PIXEL_SHADER_COMBO(BLENDMODULATE, bBlendmodulate);
			SET_STATIC_PIXEL_SHADER_COMBO(DEDICATEDMRAO, bhasMRAO ? 1 : 0);
			SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, useParallax);
			SET_STATIC_PIXEL_SHADER_COMBO(TRANSLUCENT, bTranslucent);
			SET_STATIC_PIXEL_SHADER_COMBO(FLOWMAP, bHasFlowmap);
			//SET_STATIC_PIXEL_SAHDER_COMBO(WATER, bWater);
			SET_STATIC_PIXEL_SHADER(gbuffer_ps30);
		}
		else
		{
			DECLARE_STATIC_PIXEL_SHADER(gbuffer_translucent_ps30);
			SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAP2, bBumpmap2);
			SET_STATIC_PIXEL_SHADER_COMBO(ALPHATEST, bAlphatest);
			SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAP, bBumpmap ? bSSBump ? 2 : 1 : 0);
			SET_STATIC_PIXEL_SHADER_COMBO(NOCULL, bNoCull);
			SET_STATIC_PIXEL_SHADER_COMBO(BLENDMODULATE, bBlendmodulate);
			SET_STATIC_PIXEL_SHADER_COMBO(DEDICATEDMRAO, bhasMRAO ? 1 : 0);
			SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, useParallax);
			SET_STATIC_PIXEL_SHADER_COMBO(TRANSLUCENT, bTranslucent);
			SET_STATIC_PIXEL_SHADER_COMBO(FLOWMAP, bHasFlowmap);
			//SET_STATIC_PIXEL_SAHDER_COMBO(WATER, bWater);
			SET_STATIC_PIXEL_SHADER(gbuffer_translucent_ps30);
		}
	}

		DYNAMIC_STATE
	{
		Assert(pDeferredContext != NULL);

		//for glass pass compatibility this uses DEFSTAGE_GBUFFER0 and glass uses DEFSTAGE_GBUFFER1 respectively. Implement this when glass is implemented.

		//if (pDeferredContext->m_bMaterialVarsChanged || !pDeferredContext->HasCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0))

		if (pDeferredContext->m_bMaterialVarsChanged || !pDeferredContext->HasCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0))
		{
			tmpBuf.Reset();

			if (bAlphatest)
			{
				PARM_VALIDATE(info.iAlphatestRef);

				tmpBuf.SetPixelShaderConstant4(0, PARM_FLOAT(info.iAlphatestRef), 0, 0, 0);
			}

				if (bAlbedo)
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER0, info.iAlbedo);
				else
					tmpBuf.BindStandardTexture(SHADER_SAMPLER0, TEXTURE_GREY);

			if (bBumpmap)
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER1, info.iBumpmap);


			if (bSpecular)
			{
				tmpBuf.BindTexture(SHADER_SAMPLER5, info.iSpecularTexture);
			}

			if (bAlbedo2 || bBumpmap2)
			{
				if (bBumpmap2)
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER3, info.iBumpmap2);
				else
					tmpBuf.BindStandardTexture(SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT);

				if (bAlbedo2)
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER9, info.iAlbedo2);
				else
					tmpBuf.BindStandardTexture(SHADER_SAMPLER9, TEXTURE_GREY);

				if (bBlendmodulate)
				{
					tmpBuf.SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, info.iBlendmodulateTransform);
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER4, info.iBlendmodulate);
				}
			}

			if (bhasMRAO)
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER15, info.m_nMRAO);

			if (bTreeSway)
			{
				float flParams[4];
				flParams[0] = GetFloatParam(info.m_nTreeSwaySpeedHighWindMultiplier, params, 2.0f);
				flParams[1] = GetFloatParam(info.m_nTreeSwayScrumbleFalloffExp, params, 1.0f);
				flParams[2] = GetFloatParam(info.m_nTreeSwayFalloffExp, params, 1.0f);
				flParams[3] = GetFloatParam(info.m_nTreeSwayScrumbleSpeed, params, 3.0f);
				tmpBuf.SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, flParams);

				flParams[0] = GetFloatParam(info.m_nTreeSwayHeight, params, 1000.0f);
				flParams[1] = GetFloatParam(info.m_nTreeSwayStartHeight, params, 0.1f);
				flParams[2] = GetFloatParam(info.m_nTreeSwayRadius, params, 300.0f);
				flParams[3] = GetFloatParam(info.m_nTreeSwayStartRadius, params, 0.2f);
				tmpBuf.SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, flParams);

				flParams[0] = GetFloatParam(info.m_nTreeSwaySpeed, params, 1.0f);
				flParams[1] = GetFloatParam(info.m_nTreeSwayStrength, params, 10.0f);
				flParams[2] = GetFloatParam(info.m_nTreeSwayScrumbleFrequency, params, 12.0f);
				flParams[3] = GetFloatParam(info.m_nTreeSwayScrumbleStrength, params, 10.0f);
				tmpBuf.SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, flParams);

				flParams[0] = GetFloatParam(info.m_nTreeSwaySpeedLerpStart, params, 3.0f);
				flParams[1] = GetFloatParam(info.m_nTreeSwaySpeedLerpEnd, params, 6.0f);
				tmpBuf.SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_9, flParams);
			}

			tmpBuf.SetPixelShaderConstant4(1,
				IS_FLAG_SET(MATERIAL_VAR_HALFLAMBERT) ? 1.0f : 0.0f,
				PARM_SET(info.iLitface) ? 1.0f : 0.0f,
				0, 0);

			tmpBuf.End();

			pDeferredContext->SetCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0, tmpBuf.Copy());

			//for glass pass compatibility this uses DEFSTAGE_GBUFFER0 and glass uses DEFSTAGE_GBUFFER1 respectively. Implement this when glass is implemented.

			//pDeferredContext->SetCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0, tmpBuf.Copy());
		}

		pShaderAPI->SetDefaultState();

		if (bModel && bFastVTex)
			pShader->SetHWMorphVertexShaderState(VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0);

		DECLARE_DYNAMIC_VERTEX_SHADER(gbuffer_vs30);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, (bModel && (int)vertexCompression) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, (bModel && pShaderAPI->GetCurrentNumBones() > 0) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, (bModel && pShaderAPI->IsHWMorphingEnabled()) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER(gbuffer_vs30);

#if DEFCFG_DEFERRED_SHADING == 1
		DECLARE_DYNAMIC_PIXEL_SHADER(gbuffer_defshading_ps30);
		SET_DYNAMIC_PIXEL_SHADER(gbuffer_defshading_ps30);
#else
		if (!bTranslucent)
		{
			DECLARE_DYNAMIC_PIXEL_SHADER(gbuffer_ps30);
			SET_DYNAMIC_PIXEL_SHADER(gbuffer_ps30);
		}
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER(gbuffer_translucent_ps30);
			SET_DYNAMIC_PIXEL_SHADER(gbuffer_translucent_ps30);
		}
		
#endif

		if (bModel && bFastVTex)
		{
			bool bUnusedTexCoords[3] = { false, true, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
			pShaderAPI->MarkUnusedVertexFields(0, 3, bUnusedTexCoords);
		}

		if (bTreeSway)
		{
			float fTempConst[4];
			fTempConst[0] = 0; // unused
			fTempConst[1] = pShaderAPI->CurrentTime();
			Vector windDir = pShaderAPI->GetVectorRenderingParameter(VECTOR_RENDERPARM_WIND_DIRECTION);
			fTempConst[2] = windDir.x;
			fTempConst[3] = windDir.y;
			pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, fTempConst);
		}

		// This has some spare space
		float vEyePos_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraPosition(vEyePos_SpecExponent);
		//vEyePos_SpecExponent[3] = iEnvMapLOD;
		pShaderAPI->SetPixelShaderConstant(11, vEyePos_SpecExponent, 1);

		

		/*float flZDists[2];
		flZDists[0] = GetDeferredExt()->GetZDistNear();
		flZDists[1] = GetDeferredExt()->GetZDistFar();
		flZDists[2] = GetDeferredExt()->GetZScale();
		pShaderAPI->SetPixelShaderConstant(6, flZDists);*/

		pShader->LoadViewMatrixIntoVertexShaderConstant(VERTEX_SHADER_AMBIENT_LIGHT);

		float flParallaxDepth[1];
		UTIL_StringToFloatArray(flParallaxDepth, 1, mat_pbr_parallaxdepth.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_08, flParallaxDepth);

		float flParallaxCenter[1];
		UTIL_StringToFloatArray(flParallaxCenter, 1, mat_pbr_parallaxCenter.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_09, flParallaxCenter);

		float flParallaxSamples[1];
		UTIL_StringToFloatArray(flParallaxSamples, 1, mat_pbr_parallaxmap_quality.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_10, flParallaxSamples);

		float c5[4] = { params[info.REFLECTAMOUNT]->GetFloatValue(), params[info.REFLECTAMOUNT]->GetFloatValue(),
				params[info.REFRACTAMOUNT]->GetFloatValue(), params[info.REFRACTAMOUNT]->GetFloatValue() };
		pShaderAPI->SetPixelShaderConstant(35, c5, 1);

		float fogColorConstant[4];

		params[info.FOGCOLOR]->GetVecValue(fogColorConstant, 3);
		fogColorConstant[3] = 0.0f;

		fogColorConstant[0] = SrgbGammaToLinear(fogColorConstant[0]);
		fogColorConstant[1] = SrgbGammaToLinear(fogColorConstant[1]);
		fogColorConstant[2] = SrgbGammaToLinear(fogColorConstant[2]);
		pShaderAPI->SetPixelShaderConstant(36, fogColorConstant, 1);

		if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER)
		{
			// Need to multiply by 4 in linear space since we premultiplied into
			// the render target by .25 to get overbright data in the reflection render target.
			float gammaReflectTint[3];
			params[info.REFLECTTINT]->GetVecValue(gammaReflectTint, 3);
			float linearReflectTint[4];
			linearReflectTint[0] = GammaToLinear(gammaReflectTint[0]) * 4.0f;
			linearReflectTint[1] = GammaToLinear(gammaReflectTint[1]) * 4.0f;
			linearReflectTint[2] = GammaToLinear(gammaReflectTint[2]) * 4.0f;
			linearReflectTint[3] = params[info.WATERBLENDFACTOR]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant(34, linearReflectTint, 1);
		}
		else
		{
			pShader->SetPixelShaderConstantGammaToLinear(34, info.REFLECTTINT, info.WATERBLENDFACTOR);
		}

		float c7[4] =
		{
			params[info.FOGSTART]->GetFloatValue(),
			params[info.FOGEND]->GetFloatValue() - params[info.FOGSTART]->GetFloatValue(),
			1.0f,
			0.0f
		};
		if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER)
		{
			// water overbright factor
			c7[2] = 4.0;
		}
		pShaderAPI->SetPixelShaderConstant(37, c7, 1);

		float vTimeConst[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float flTime = pShaderAPI->CurrentTime();
		vTimeConst[0] = flTime;
		//vTimeConst[0] -= ( float )( ( int )( vTimeConst[0] / 1000.0f ) ) * 1000.0f;
		pShaderAPI->SetPixelShaderConstant(38, vTimeConst, 1);

		if (bHasFlowmap)
		{
			pShader->BindTexture(SHADER_SAMPLER6, info.FLOWMAP, info.FLOWMAPFRAME);
			pShader->BindTexture(SHADER_SAMPLER7, info.FLOW_NOISE_TEXTURE);

			float vFlowConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			vFlowConst1[0] = 1.0f / params[info.FLOW_WORLDUVSCALE]->GetFloatValue();
			vFlowConst1[1] = 1.0f / params[info.FLOW_NORMALUVSCALE]->GetFloatValue();
			vFlowConst1[2] = params[info.FLOW_BUMPSTRENGTH]->GetFloatValue();
			vFlowConst1[3] = params[info.COLOR_FLOW_DISPLACEBYNORMALSTRENGTH]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant(13, vFlowConst1, 1);

			float vFlowConst2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			vFlowConst2[0] = params[info.FLOW_TIMEINTERVALINSECONDS]->GetFloatValue();
			vFlowConst2[1] = params[info.FLOW_UVSCROLLDISTANCE]->GetFloatValue();
			vFlowConst2[2] = params[info.FLOW_NOISE_SCALE]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant(14, vFlowConst2, 1);

			float vColorFlowConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			vColorFlowConst1[0] = 1.0f / params[info.COLOR_FLOW_UVSCALE]->GetFloatValue();
			vColorFlowConst1[1] = params[info.COLOR_FLOW_TIMEINTERVALINSECONDS]->GetFloatValue();
			vColorFlowConst1[2] = params[info.COLOR_FLOW_UVSCROLLDISTANCE]->GetFloatValue();
			vColorFlowConst1[3] = params[info.COLOR_FLOW_LERPEXP]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant(26, vColorFlowConst1, 1);
		}

		float vPos[4] = { 0,0,0,0 };
		pShaderAPI->GetWorldSpaceCameraPosition(vPos);
		float zScale[4] = { GetDeferredExt()->GetZScale(),0,0,0 };
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vPos);
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, GetDeferredExt()->GetForwardBase());
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, zScale);

		CommitBaseDeferredConstants_Origin(pShaderAPI, 1);
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, BASETEXTURETRANSFORM);
		pShaderAPI->ExecuteCommandBuffer(pDeferredContext->GetCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0));

		//for glass pass compatibility this uses DEFSTAGE_GBUFFER0 and glass uses DEFSTAGE_GBUFFER1 respectively. Implement this when glass is implemented.

		//pShaderAPI->ExecuteCommandBuffer(pDeferredContext->GetCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0));
	}



	pShader->Draw();
}



//// testing my crappy math
//float PackLightingControls(int phong_exp, int half_lambert, int litface)
//{
//	return (litface +
//		half_lambert * 2 +
//		phong_exp * 4) / 255.0f;
//}
//
//void UnpackLightingControls(float mixed,
//	float& phong_exp, float& half_lambert, float& litface)
//{
//	mixed *= 255.0f;
//
//	litface = fmod(mixed, 2.0f);
//	half_lambert = fmod(mixed -= litface, 4.0f);
//	phong_exp = fmod(mixed -= half_lambert, 256.0f);
//
//	half_lambert /= 2.0f;
//	phong_exp /= 252.0f;
//}
//
//static uint8 packed;
//
//CON_COMMAND(test_packing, "")
//{
//	if (args.ArgC() < 4)
//		return;
//
//	float res = PackLightingControls(atoi(args[1]),
//		atoi(args[2]),
//		atoi(args[3]));
//
//	res *= 255.0f;
//
//	packed = res;
//
//	Msg("packed to: %u\n", packed);
//}
//
//CON_COMMAND(test_unpacking, "")
//{
//	float o0, o1, o2;
//
//	UnpackLightingControls(packed / 255.0f, o0, o1, o2);
//
//	Msg("unpacked to: exp %f, halfl %f, litface %f\n", o0, o1, o2);
//}
//
