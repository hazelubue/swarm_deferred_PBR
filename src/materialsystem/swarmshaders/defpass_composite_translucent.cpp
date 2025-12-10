
#include "deferred_includes.h"

#include "include/composite_vs30.inc"
#include "include/composite_translucent_ps30.inc"

#include "tier0/memdbgon.h"

static CCommandBufferBuilder< CFixedCommandStorageBuffer< 512 > > tmpBuf;

extern ConVar building_cubemaps;

void InitParmsComposite_translucent(const defParms_composite_translucent& info, CBaseVSShader* pShader, IMaterialVar** params)
{
	if (PARM_NO_DEFAULT(info.iAlphatestRef) ||
		PARM_VALID(info.iAlphatestRef) && PARM_FLOAT(info.iAlphatestRef) == 0.0f)
		params[info.iAlphatestRef]->SetFloatValue(DEFAULT_ALPHATESTREF);

	PARM_INIT_FLOAT(info.iPhongScale, DEFAULT_PHONG_SCALE);
	PARM_INIT_INT(info.iPhongFresnel, 0);

	PARM_INIT_FLOAT(info.iEnvmapContrast, 0.0f);
	PARM_INIT_FLOAT(info.iEnvmapSaturation, 1.0f);
	PARM_INIT_VEC3(info.iEnvmapTint, 1.0f, 1.0f, 1.0f);
	PARM_INIT_INT(info.iEnvmapFresnel, 0);

	PARM_INIT_INT(info.iRimlightEnable, 0);
	PARM_INIT_FLOAT(info.iRimlightExponent, 4.0f);
	PARM_INIT_FLOAT(info.iRimlightAlbedoScale, 0.0f);
	PARM_INIT_VEC3(info.iRimlightTint, 1.0f, 1.0f, 1.0f);
	PARM_INIT_INT(info.iRimlightModLight, 0);

	PARM_INIT_VEC3(info.iSelfIllumTint, 1.0f, 1.0f, 1.0f);
	PARM_INIT_INT(info.iSelfIllumMaskInEnvmapAlpha, 0);
	PARM_INIT_INT(info.iSelfIllumFresnelModulate, 0);
}

void InitPassComposite_translucent(const defParms_composite_translucent& info, CBaseVSShader* pShader, IMaterialVar** params)
{
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);

	if (PARM_DEFINED(info.iAlbedo))
		pShader->LoadTexture(info.iAlbedo);

	/*if (PARM_DEFINED(info.BUMPMAP))
		params[info.BUMPMAP]->SetStringValue("dev/graygrid");*/

	if (bTranslucent)
	{

		if (PARM_DEFINED(info.BUMPMAP))
			pShader->LoadBumpMap(info.BUMPMAP);

		/*if (PARM_DEFINED(info.MRAOTEXTURE))
			params[info.MRAOTEXTURE]->SetStringValue("dev/dev_perfectgloss");*/

		if (PARM_DEFINED(info.MRAOTEXTURE))
			pShader->LoadTexture(info.MRAOTEXTURE);
	}

	if (PARM_DEFINED(info.iEnvmap))
		pShader->LoadCubeMap(info.iEnvmap);

	if (PARM_DEFINED(info.iEnvmapMask))
		pShader->LoadTexture(info.iEnvmapMask);

	if (PARM_DEFINED(info.iAlbedo2))
		pShader->LoadTexture(info.iAlbedo2);

	if (PARM_DEFINED(info.iAlbedo3))
		pShader->LoadTexture(info.iAlbedo3);

	if (PARM_DEFINED(info.iAlbedo4))
		pShader->LoadTexture(info.iAlbedo4);

	if (PARM_DEFINED(info.iBlendmodulate))
		pShader->LoadTexture(info.iBlendmodulate);

	if (PARM_DEFINED(info.iBlendmodulate2))
		pShader->LoadTexture(info.iBlendmodulate2);

	if (PARM_DEFINED(info.iBlendmodulate3))
		pShader->LoadTexture(info.iBlendmodulate3);

	if (PARM_DEFINED(info.iSelfIllumMask))
		pShader->LoadTexture(info.iSelfIllumMask);
}

void DrawPassComposite_translucent(const defParms_composite_translucent& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext)
{
	const bool bModel = info.bModel; 
	//const int iLightType_Point = PARM_INT(info.iLightTypePointVar);
	//const int iLightType_Spot = PARM_INT(info.iLightTypeSpotVar);

	const bool bIsDecal = IS_FLAG_SET(MATERIAL_VAR_DECAL);
	const bool bFastVTex = g_pHardwareConfig->HasFastVertexTextures();

	const bool bAlbedo = PARM_TEX(info.iAlbedo);
	const bool bAlbedo2 = !bModel && bAlbedo && PARM_TEX(info.iAlbedo2);
	const bool bAlbedo3 = !bModel && bAlbedo && PARM_TEX(info.iAlbedo3);
	const bool bAlbedo4 = !bModel && bAlbedo && PARM_TEX(info.iAlbedo4);

	const bool bAlphatest = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) && bAlbedo;
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT) && bAlbedo && !bAlphatest;

	const bool bNoCull = IS_FLAG_SET(MATERIAL_VAR_NOCULL);

	const bool bUseSRGB = DEFCFG_USE_SRGB_CONVERSION != 0;
	const bool bPhongFresnel = PARM_SET(info.iPhongFresnel);

	const bool bEnvmap = PARM_TEX(info.iEnvmap);
	const bool bEnvmapMask = bEnvmap && PARM_TEX(info.iEnvmapMask);
	const bool bEnvmapMask2 = bEnvmapMask && PARM_TEX(info.iEnvmapMask2);
	const bool bEnvmapFresnel = bEnvmap && PARM_SET(info.iEnvmapFresnel);

	const bool bRimLight = PARM_SET(info.iRimlightEnable);
	const bool bRimLightModLight = bRimLight && PARM_SET(info.iRimlightModLight);
	const bool bBlendmodulate = bAlbedo2 && PARM_TEX(info.iBlendmodulate);
	const bool bBlendmodulate2 = bBlendmodulate && PARM_TEX(info.iBlendmodulate2);
	const bool bBlendmodulate3 = bBlendmodulate && PARM_TEX(info.iBlendmodulate3);

	const bool bSelfIllum = !bAlbedo2 && IS_FLAG_SET(MATERIAL_VAR_SELFILLUM);
	const bool bSelfIllumMaskInEnvmapMask = bSelfIllum && bEnvmapMask && PARM_SET(info.iSelfIllumMaskInEnvmapAlpha);
	const bool bSelfIllumMask = bSelfIllum && !bSelfIllumMaskInEnvmapMask && !bEnvmapMask && PARM_TEX(info.iSelfIllumMask);

	const bool bMultiBlend = PARM_SET(info.iMultiblend)
		&& bAlbedo && bAlbedo2 && bAlbedo3 && !bEnvmapMask && !bSelfIllumMask;

	const bool bNeedsFresnel = bPhongFresnel || bEnvmapFresnel;
	const bool bGBufferNormal = bEnvmap || bRimLight || bNeedsFresnel;
	const bool bWorldEyeVec = bGBufferNormal;

	//const bool bMRAO = PARM_SET(info.MRAOTEXTURE);
	const bool bNormal = PARM_SET(info.BUMPMAP);


	AssertMsgOnce(!(bTranslucent || bAlphatest) || !bAlbedo2,
		"blended albedo not supported by gbuffer pass!");

	AssertMsgOnce(IS_FLAG_SET(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK) == false,
		"Normal map sampling should stay out of composition pass.");

	AssertMsgOnce(!PARM_TEX(info.iSelfIllumMask) || !bEnvmapMask,
		"Can't use separate selfillum mask with envmap mask - use SELFILLUM_ENVMAPMASK_ALPHA instead.");

	AssertMsgOnce(PARM_SET(info.iMultiblend) == bMultiBlend,
		"Multiblend forced off due to invalid usage! May cause vertexformat mis-matches between passes.");


	SHADOW_STATE
	{
		pShaderShadow->SetDefaultState();
		pShaderShadow->EnableSRGBWrite(bUseSRGB);

		if (bNoCull)
		{
			pShaderShadow->EnableCulling(false);
		}

		int iVFmtFlags = VERTEX_POSITION;
		int iUserDataSize = 0;

		int* pTexCoordDim;
		int iTexCoordNum;
		GetTexcoordSettings((bModel && bIsDecal && bFastVTex), bMultiBlend,
			iTexCoordNum, &pTexCoordDim);

		if (bModel)
		{
			iVFmtFlags |= VERTEX_NORMAL;
			iVFmtFlags |= VERTEX_FORMAT_COMPRESSED;
		}
		else
		{
			if (bAlbedo2)
				iVFmtFlags |= VERTEX_COLOR;
		}

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, bUseSRGB);


		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, false);


		if (bTranslucent)
		{
			pShader->EnableAlphaBlending(SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
		}

		if (bEnvmap)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);

			if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE)
				pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, true);

			if (bEnvmapMask)
			{
				pShaderShadow->EnableTexture(SHADER_SAMPLER4, true);

				if (bAlbedo2)
					pShaderShadow->EnableTexture(SHADER_SAMPLER7, true);
			}
		}
		else if (bSelfIllumMask)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER4, true);
		}

		if (bAlbedo2)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER5, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER5, bUseSRGB);

			if (bBlendmodulate)
				pShaderShadow->EnableTexture(SHADER_SAMPLER6, true);
		}

		if (bMultiBlend)
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER7, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER7, bUseSRGB);

			if (bAlbedo4)
			{
				pShaderShadow->EnableTexture(SHADER_SAMPLER8, true);
				pShaderShadow->EnableSRGBRead(SHADER_SAMPLER8, bUseSRGB);
			}

			if (bBlendmodulate)
			{
				pShaderShadow->EnableTexture(SHADER_SAMPLER9, true);
				pShaderShadow->EnableTexture(SHADER_SAMPLER10, true);
			}
		}

			/*if (bMRAO)
			{*/
			//pShaderShadow->EnableTexture(SHADER_SAMPLER11, true);
			//}


			pShaderShadow->EnableAlphaWrites(false);
			pShaderShadow->EnableDepthWrites(!bTranslucent);

			pShader->DefaultFog();

			pShaderShadow->VertexShaderVertexFormat(iVFmtFlags, iTexCoordNum, pTexCoordDim, iUserDataSize);

			//int lighttype_variant = (iLightType_Point) ? 2 : iLightType_Spot;

			DECLARE_STATIC_VERTEX_SHADER(composite_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(MODEL, bModel);
			SET_STATIC_VERTEX_SHADER_COMBO(MORPHING_VTEX, bModel && bFastVTex);
			SET_STATIC_VERTEX_SHADER_COMBO(DECAL, bModel && bIsDecal);
			SET_STATIC_VERTEX_SHADER_COMBO(EYEVEC, bWorldEyeVec);
			SET_STATIC_VERTEX_SHADER_COMBO(BASETEXTURE2, bAlbedo2 && !bMultiBlend);
			SET_STATIC_VERTEX_SHADER_COMBO(BLENDMODULATE, bBlendmodulate);
			SET_STATIC_VERTEX_SHADER_COMBO(MULTIBLEND, bMultiBlend);
			SET_STATIC_VERTEX_SHADER(composite_vs30);

			DECLARE_STATIC_PIXEL_SHADER(composite_translucent_ps30);
			//SET_STATIC_PIXEL_SHDAER_COMBO(LIGHTTYPE, 0);
			SET_STATIC_PIXEL_SHADER_COMBO(READNORMAL, bGBufferNormal);
			SET_STATIC_PIXEL_SHADER_COMBO(NOCULL, bNoCull);
			SET_STATIC_PIXEL_SHADER_COMBO(ENVMAP, bEnvmap);
			SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMASK, bEnvmapMask);
			SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvmapFresnel);
			SET_STATIC_PIXEL_SHADER_COMBO(PHONGFRESNEL, bPhongFresnel);
			SET_STATIC_PIXEL_SHADER_COMBO(RIMLIGHT, bRimLight);
			SET_STATIC_PIXEL_SHADER_COMBO(RIMLIGHTMODULATELIGHT, bRimLightModLight);
			SET_STATIC_PIXEL_SHADER_COMBO(BASETEXTURE2, bAlbedo2 && !bMultiBlend);
			SET_STATIC_PIXEL_SHADER_COMBO(BLENDMODULATE, bBlendmodulate);
			SET_STATIC_PIXEL_SHADER_COMBO(MULTIBLEND, bMultiBlend);
			SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUM, bSelfIllum);
			SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUM_MASK, bSelfIllumMask);
			SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUM_ENVMAP_ALPHA, bSelfIllumMaskInEnvmapMask);
			SET_STATIC_PIXEL_SHADER(composite_translucent_ps30);
	}
		DYNAMIC_STATE
	{
		Assert(pDeferredContext != NULL);

		if (pDeferredContext->m_bMaterialVarsChanged || !pDeferredContext->HasCommands(CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE)
			|| building_cubemaps.GetBool())
		{
			tmpBuf.Reset();

			if (bAlphatest)
			{
				PARM_VALIDATE(info.iAlphatestRef);
				tmpBuf.SetPixelShaderConstant1(0, PARM_FLOAT(info.iAlphatestRef));
			}

			if (bAlbedo)
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER0, info.iAlbedo);
			else
				tmpBuf.BindStandardTexture(SHADER_SAMPLER0, TEXTURE_GREY);

			if (bEnvmap)
			{
				if (building_cubemaps.GetBool())
					tmpBuf.BindStandardTexture(SHADER_SAMPLER3, TEXTURE_BLACK);
				else
				{
					if (PARM_TEX(info.iEnvmap) && !bModel)
						tmpBuf.BindTexture(pShader, SHADER_SAMPLER3, info.iEnvmap);
					else
						tmpBuf.BindStandardTexture(SHADER_SAMPLER3, TEXTURE_LOCAL_ENV_CUBEMAP);
				}

				if (bEnvmapMask)
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER4, info.iEnvmapMask);

				if (bAlbedo2)
				{
					if (bEnvmapMask2)
						tmpBuf.BindTexture(pShader, SHADER_SAMPLER7, info.iEnvmapMask2);
					else
						tmpBuf.BindStandardTexture(SHADER_SAMPLER7, TEXTURE_WHITE);
				}

				tmpBuf.SetPixelShaderConstant(5, info.iEnvmapTint);

				float fl6[4] = { 0 };
				fl6[0] = PARM_FLOAT(info.iEnvmapSaturation);
				fl6[1] = PARM_FLOAT(info.iEnvmapContrast);
				tmpBuf.SetPixelShaderConstant(6, fl6);
			}

			if (bNeedsFresnel)
			{
				tmpBuf.SetPixelShaderConstant(7, info.iFresnelRanges);
			}

			if (bRimLight)
			{
				float fl9[4] = { 0 };
				fl9[0] = PARM_FLOAT(info.iRimlightExponent);
				fl9[1] = PARM_FLOAT(info.iRimlightAlbedoScale);
				tmpBuf.SetPixelShaderConstant(9, fl9);
			}

			if (bAlbedo2)
			{
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER5, info.iAlbedo2);

				if (bBlendmodulate)
				{
					tmpBuf.SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, info.iBlendmodulateTransform);
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER6, info.iBlendmodulate);
				}
			}

			if (bMultiBlend)
			{
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER7, info.iAlbedo3);

				if (bAlbedo4)
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER8, info.iAlbedo4);
				else
					tmpBuf.BindStandardTexture(SHADER_SAMPLER8, TEXTURE_WHITE);

				if (bBlendmodulate)
				{
					tmpBuf.SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, info.iBlendmodulateTransform2);
					tmpBuf.SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, info.iBlendmodulateTransform3);

					if (bBlendmodulate2)
						tmpBuf.BindTexture(pShader, SHADER_SAMPLER9, info.iBlendmodulate2);
					else
						tmpBuf.BindStandardTexture(SHADER_SAMPLER9, TEXTURE_BLACK);

					if (bBlendmodulate3)
						tmpBuf.BindTexture(pShader, SHADER_SAMPLER10, info.iBlendmodulate3);
					else
						tmpBuf.BindStandardTexture(SHADER_SAMPLER10, TEXTURE_BLACK);
				}
			}

			//if (bTranslucent)
			//{
				/*if (bMRAO)
				{
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER11, info.MRAOTEXTURE);
				}
				else
				{
					tmpBuf.BindStandardTexture(SHADER_SAMPLER11, TEXTURE_WHITE);
				}*/
				if (bNormal)
				{
					tmpBuf.BindTexture(pShader, SHADER_SAMPLER1, info.BUMPMAP);
				}
				else
				{
					tmpBuf.BindStandardTexture( SHADER_SAMPLER1, TEXTURE_NORMALMAP_FLAT);
				}
			//}

			if (bSelfIllum && bSelfIllumMask)
			{
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER4, info.iSelfIllumMask);
			}

			tmpBuf.SetPixelShaderFogParams(2);

			float fl4 = { PARM_FLOAT(info.iPhongScale) };
			tmpBuf.SetPixelShaderConstant1(4, fl4);

			tmpBuf.End();

			pDeferredContext->SetCommands(CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE, tmpBuf.Copy());
		}

		pShaderAPI->SetDefaultState();

		if (bModel && bFastVTex)
			pShader->SetHWMorphVertexShaderState(VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0);

		DECLARE_DYNAMIC_VERTEX_SHADER(composite_vs30);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, (bModel && (int)vertexCompression) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, (bModel && pShaderAPI->GetCurrentNumBones() > 0) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, (bModel && pShaderAPI->IsHWMorphingEnabled()) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER(composite_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(composite_translucent_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(composite_translucent_ps30);

		if (bModel && bFastVTex)
		{
			bool bUnusedTexCoords[3] = { false, true, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
			pShaderAPI->MarkUnusedVertexFields(0, 3, bUnusedTexCoords);
		}

		pShaderAPI->ExecuteCommandBuffer(pDeferredContext->GetCommands(CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE));

		CommitBaseDeferredConstants_Origin(pShaderAPI, 3);

		CDeferredExtension* pExt = GetDeferredExt();
		int numForwardLights = pExt->GetNumActiveForwardLights();

		float forwardLightCount[4] = { (float)numForwardLights, 0, 0, 0 };
		pShaderAPI->SetPixelShaderConstant(44, forwardLightCount);

		if (numForwardLights > 0)
		{
			float* pLightData = pExt->GetForwardLightData();
			if (pLightData)
			{
				pShaderAPI->SetPixelShaderConstant(45,
					pLightData,
					pExt->GetForwardLights_NumRows());
			}
		}

		/*float* pSpotlightData = pExt->GetForwardSpotlightData();
		if (pSpotlightData)
		{
			pShaderAPI->SetPixelShaderConstant(67,
				pSpotlightData,
				pExt->GetForwardLights_NumRows());
		}*/

		if (bWorldEyeVec)
		{
			float vEyepos[4] = {0,0,0,0};
			pShaderAPI->GetWorldSpaceCameraPosition(vEyepos);
			pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vEyepos);
		}

		if (bRimLight)
		{
			pShaderAPI->SetPixelShaderConstant(8, params[info.iRimlightTint]->GetVecValue());
		}

		if (bSelfIllum)
		{
			pShaderAPI->SetPixelShaderConstant(10, params[info.iSelfIllumTint]->GetVecValue());
		}
	}

	pShader->Draw();
}