
#include "BaseVSShader.h"
#include "convar.h"

#include "deferred_includes.h"

#include "include/gbuffer_vs30.inc"
#include "include/gbuffer_ps30.inc"
#include "include/gbuffer_translucent_ps30.inc"
#include "shaderapi\ishaderapi.h"
#include "tier0/memdbgon.h"

const int PARALLAX_QUALITY_MAX = 3;

static ConVar mat_pbr_parallaxdepth("mat_pbr_parallaxdepth", ".1"); // 0.04
static ConVar mat_pbr_parallaxCenter("mat_pbr_parallaxCenter", ".9");
static ConVar mat_pbr_parallaxmap_quality("mat_pbr_parallaxmap_quality", "100", FCVAR_NONE, "", true, 0, true, PARALLAX_QUALITY_MAX);
static ConVar mat_pbr_parallaxmap("mat_pbr_parallaxmap", "1");
static ConVar mat_pbr_force_20b("mat_pbr_force_20b", "0", FCVAR_CHEAT);
static ConVar mat_pbr_iblIntensity("mat_pbr_iblIntensity", "1000.0", FCVAR_CHEAT);

extern ConVar r_ss_distortion;
extern ConVar r_ss_power;
extern ConVar r_ss_scale;


static CCommandBufferBuilder< CFixedCommandStorageBuffer< 512 > > tmpBuf;

void InitParmsGBuffer(const defParms_gBuffer0& info, CBaseVSShader* pShader, IMaterialVar** params)
{
	const bool bModel = info.bModel;
	const bool bBumpmap = PARM_TEX(info.iBumpmap);

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

	if (!bModel && bBumpmap)
	{
		SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);
	}
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


	/*if (params[info.m_nMRAO]->IsDefined())
	{
		pShader->LoadTexture(info.m_nMRAO);
	}*/
	//fucking crashing for some unbenounced reason. yes the gbuffer is contextualized correctly.
}

void DrawPassGBuffer(const defParms_gBuffer0& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext)
{

	const bool bModel = info.bModel;

	const bool bIsDecal = IS_FLAG_SET(MATERIAL_VAR_DECAL);
	const bool bFastVTex = g_pHardwareConfig->HasFastVertexTextures();
	const bool bNoCull = IS_FLAG_SET(MATERIAL_VAR_NOCULL);

	const bool bAlbedo = PARM_TEX(info.iAlbedo);
	const bool bAlbedo2 = PARM_TEX(info.iAlbedo2);
	const bool bBumpmap = PARM_TEX(info.iBumpmap);
	const bool bBumpmap2 = bBumpmap && PARM_TEX(info.iBumpmap2);
	const bool bSpecular = PARM_TEX(info.iSpecularTexture);

	const bool bBlendmodulate = (bAlbedo2 || bBumpmap2) && PARM_TEX(info.iBlendmodulate);

	const int nTreeSwayMode = clamp(GetIntParam(info.m_nTreeSway, params, 0), 0, 2);
	const bool bTreeSway = nTreeSwayMode != 0;

	const bool bAlphatest = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) && bAlbedo;
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);

	const bool bSSBump = bBumpmap && PARM_SET(info.iSSBump);

	const bool useParallax = mat_pbr_parallaxmap.GetBool();
	bool bhasMRAO = IsTextureSet(info.m_nMRAO, params);

	//bool bHasFlowmap = params[info.FLOWMAP]->IsTexture();
	//bool bLightmap = !bModel;

	SHADOW_STATE
	{
		pShaderShadow->SetDefaultState();

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

		DECLARE_STATIC_PIXEL_SHADER(gbuffer_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAP2, bBumpmap2);
		SET_STATIC_PIXEL_SHADER_COMBO(ALPHATEST, bAlphatest);
		SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAP, bBumpmap ? bSSBump ? 2 : 1 : 0);
		SET_STATIC_PIXEL_SHADER_COMBO(NOCULL, bNoCull);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDMODULATE, bBlendmodulate);
		SET_STATIC_PIXEL_SHADER_COMBO(DEDICATEDMRAO, bhasMRAO ? 1 : 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, useParallax);
		SET_STATIC_PIXEL_SHADER_COMBO(TRANSLUCENT, bTranslucent);
		SET_STATIC_PIXEL_SHADER(gbuffer_ps30);

		pShader->PI_BeginCommandBuffer();

		// Send ambient cube to the pixel shader, force to black if not available
		pShader->PI_SetPixelShaderAmbientLightCube(39);

		// Send lighting array to the pixel shader
		//pShader->PI_SetPixelShaderLocalLighting(PSREG_LIGHT_INFO_ARRAY);

		// Set up shader modulation color
		pShader->PI_SetModulationPixelShaderDynamicState_LinearColorSpace(PSREG_DIFFUSE_MODULATION);

		pShader->PI_EndCommandBuffer();
		
	}
	DYNAMIC_STATE
	{
		Assert(pDeferredContext != NULL);

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

			/*if (bLightmap)
				tmpBuf.BindStandardTexture(SHADER_SAMPLER8, TEXTURE_LIGHTMAP);*/

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

			int x, y, w, t;
			pShaderAPI->GetCurrentViewport(x, y, w, t);
			float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

			tmpBuf.SetPixelShaderConstant(13, fl1);

			tmpBuf.End();

			pDeferredContext->SetCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0, tmpBuf.Copy());
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

		DECLARE_DYNAMIC_PIXEL_SHADER(gbuffer_ps30);
		SET_DYNAMIC_PIXEL_SHADER(gbuffer_ps30);
#endif
		LightState_t lightState;
		pShaderAPI->GetDX9LightState(&lightState);

		ITexture* pSource = materials->FindTexture("_rt_fullframefb", TEXTURE_GROUP_RENDER_TARGET);

		pShader->BindTexture(SHADER_SAMPLER8, pSource);

		ITexture* pDepthTexture = materials->FindTexture("_rt_FullFrameDepth", TEXTURE_GROUP_RENDER_TARGET);

		pShader->BindTexture(SHADER_SAMPLER7, pDepthTexture);

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

		float vTimeConst[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float flTime = pShaderAPI->CurrentTime();
		vTimeConst[0] = flTime;
		//vTimeConst[0] -= ( float )( ( int )( vTimeConst[0] / 1000.0f ) ) * 1000.0f;
		pShaderAPI->SetPixelShaderConstant(12, vTimeConst, 1);

		// Data passed from viewrender cpu side.
		const Matrix_Data_t& data = GetDeferredExt()->GetCommonData();

		pShaderAPI->SetPixelShaderConstant(16, data.matViewInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(20, data.matProjInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(24, data.matView.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(31, data.matProj.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(38, data.matStaticView.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(42, data.matStaticViewInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(35, data.flZDists, 2);
		pShaderAPI->SetPixelShaderConstant(36, &data.aspect, 1);
		pShaderAPI->SetPixelShaderConstant(37, &data.fov, 1);

		float viewportOffset[2];
		viewportOffset[0] = data.viewportOffsetX;
		viewportOffset[1] = data.viewportOffsetY;

		pShaderAPI->SetPixelShaderConstant(38, viewportOffset, 1);

		// end data passed from viewrender.

		float flthickness = 0.125f;

		pShaderAPI->SetPixelShaderConstant(39, &flthickness, 1);

		float scatteringVars[3];
		scatteringVars[0] = r_ss_distortion.GetFloat();
		scatteringVars[1] = r_ss_power.GetFloat();
		scatteringVars[2] = r_ss_scale.GetFloat();

		pShaderAPI->SetPixelShaderConstant(41, scatteringVars, 1);

		float vPos[4] = { 0,0,0,0 };
		pShaderAPI->GetWorldSpaceCameraPosition(vPos);
		float zScale[4] = { GetDeferredExt()->GetZScale(),0,0,0 };
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vPos);
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, GetDeferredExt()->GetForwardBase());
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, zScale);

		CommitBaseDeferredConstants_Origin(pShaderAPI, 1);
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, BASETEXTURETRANSFORM);
		pShaderAPI->ExecuteCommandBuffer(pDeferredContext->GetCommands(CDeferredPerMaterialContextData::DEFSTAGE_GBUFFER0));

	}

	pShader->Draw();
}