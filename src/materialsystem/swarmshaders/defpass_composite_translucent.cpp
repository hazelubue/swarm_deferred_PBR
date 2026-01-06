
#include "deferred_includes.h"

#include "include/composite_translucent_vs30.inc"
#include "include/composite_translucent_ps30.inc"

#include "tier0/memdbgon.h"
#include "../../game/client/rendertexture.h"

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
	if (PARM_DEFINED(info.ReflectTexture))
		pShader->LoadTexture(info.ReflectTexture);

	/*if (PARM_DEFINED(info.ReflectTexture))
		params[info.ReflectTexture]->SetStringValue("_rt_fullframefb");*/

	if (bTranslucent)
	{

		if (PARM_DEFINED(info.BUMPMAP))
			pShader->LoadBumpMap(info.BUMPMAP);

		if (PARM_DEFINED(info.iEnvmap))
			params[info.iEnvmap]->SetStringValue("env_cubemap");

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
	//const bool bAlbedo2 = !bModel && bAlbedo && PARM_TEX(info.iAlbedo2);
	//const bool bAlbedo3 = !bModel && bAlbedo && PARM_TEX(info.iAlbedo3);
	//const bool bAlbedo4 = !bModel && bAlbedo && PARM_TEX(info.iAlbedo4);

	const bool bAlphatest = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) && bAlbedo;
	const bool bTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT) && bAlbedo && !bAlphatest;

	const bool bNoCull = IS_FLAG_SET(MATERIAL_VAR_NOCULL);

	const bool bUseSRGB = DEFCFG_USE_SRGB_CONVERSION != 0;
	const bool bPhongFresnel = PARM_SET(info.iPhongFresnel);

	const bool bEnvmap = PARM_TEX(info.iEnvmap);
	const bool bEnvmapMask = bEnvmap && PARM_TEX(info.iEnvmapMask);
	//const bool bEnvmapMask2 = bEnvmapMask && PARM_TEX(info.iEnvmapMask2);
	const bool bEnvmapFresnel = bEnvmap && PARM_SET(info.iEnvmapFresnel);

	const bool bRimLight = PARM_SET(info.iRimlightEnable);
	//const bool bRimLightModLight = bRimLight && PARM_SET(info.iRimlightModLight);

	const bool bSelfIllum = IS_FLAG_SET(MATERIAL_VAR_SELFILLUM);
	const bool bSelfIllumMaskInEnvmapMask = bSelfIllum && bEnvmapMask && PARM_SET(info.iSelfIllumMaskInEnvmapAlpha);
	const bool bSelfIllumMask = bSelfIllum && !bSelfIllumMaskInEnvmapMask && !bEnvmapMask && PARM_TEX(info.iSelfIllumMask);

	const bool bNeedsFresnel = bPhongFresnel || bEnvmapFresnel;
	const bool bGBufferNormal = bEnvmap || bRimLight || bNeedsFresnel;
	const bool bWorldEyeVec = bGBufferNormal;

	//const bool bMRAO = PARM_SET(info.MRAOTEXTURE);
	const bool bNormal = PARM_SET(info.BUMPMAP);
	//const bool bLightMapped = !bModel;

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
			GetTexcoordSettings((bModel && bIsDecal && bFastVTex), 0.0,
				iTexCoordNum, &pTexCoordDim);

			if (bModel)
			{
				iVFmtFlags |= VERTEX_NORMAL;
				iVFmtFlags |= VERTEX_FORMAT_COMPRESSED;
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

				}
			}
			else if (bSelfIllumMask)
			{
				pShaderShadow->EnableTexture(SHADER_SAMPLER4, true);
			}

			pShaderShadow->EnableTexture(SHADER_SAMPLER11, true);
			pShaderShadow->EnableTexture(SHADER_SAMPLER12, true);

			pShaderShadow->EnableAlphaWrites(false);
			pShaderShadow->EnableDepthWrites(!bTranslucent);

			pShader->DefaultFog();

			pShaderShadow->VertexShaderVertexFormat(iVFmtFlags, iTexCoordNum, pTexCoordDim, iUserDataSize);

			DECLARE_STATIC_VERTEX_SHADER(composite_translucent_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(MODEL, bModel);
			SET_STATIC_VERTEX_SHADER_COMBO(MORPHING_VTEX, bModel && bFastVTex);
			SET_STATIC_VERTEX_SHADER_COMBO(DECAL, bModel && bIsDecal);
			SET_STATIC_VERTEX_SHADER_COMBO(EYEVEC, bWorldEyeVec);
			SET_STATIC_VERTEX_SHADER_COMBO(BASETEXTURE2, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(BLENDMODULATE, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(MULTIBLEND, 0);
			SET_STATIC_VERTEX_SHADER(composite_translucent_vs30);

			DECLARE_STATIC_PIXEL_SHADER(composite_translucent_ps30);
			SET_STATIC_PIXEL_SHADER_COMBO(READNORMAL, bGBufferNormal);
			SET_STATIC_PIXEL_SHADER_COMBO(NOCULL, bNoCull);
			SET_STATIC_PIXEL_SHADER_COMBO(ENVMAP, bEnvmap);
			SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMASK, bEnvmapMask);
			SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvmapFresnel);
			SET_STATIC_PIXEL_SHADER_COMBO(PHONGFRESNEL, bPhongFresnel);
			SET_STATIC_PIXEL_SHADER_COMBO(BASETEXTURE2, 0);
			SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUM, bSelfIllum);
			SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUM_MASK, bSelfIllumMask);
			SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUM_ENVMAP_ALPHA, bSelfIllumMaskInEnvmapMask);
			SET_STATIC_PIXEL_SHADER_COMBO(ALPHATEST, bAlphatest);
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

			if (bNormal)
			{
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER1, info.BUMPMAP);
			}
			else
			{
				tmpBuf.BindStandardTexture( SHADER_SAMPLER1, TEXTURE_NORMALMAP_FLAT);
			}
			
			if (bSelfIllum && bSelfIllumMask)
			{
				tmpBuf.BindTexture(pShader, SHADER_SAMPLER4, info.iSelfIllumMask);
			}

			tmpBuf.SetPixelShaderFogParams(2);

			int x, y, w, t;
			pShaderAPI->GetCurrentViewport(x, y, w, t);
			float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

			tmpBuf.SetPixelShaderConstant(1, fl1);

			tmpBuf.End();

			pDeferredContext->SetCommands(CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE, tmpBuf.Copy());
		}

		pShaderAPI->SetDefaultState();

		if (bModel && bFastVTex)
			pShader->SetHWMorphVertexShaderState(VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0);

		DECLARE_DYNAMIC_VERTEX_SHADER(composite_translucent_vs30);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, (bModel && (int)vertexCompression) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, (bModel && pShaderAPI->GetCurrentNumBones() > 0) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, (bModel && pShaderAPI->IsHWMorphingEnabled()) ? 1 : 0);
		SET_DYNAMIC_VERTEX_SHADER(composite_translucent_vs30);

		CDeferredExtension* pExt = GetDeferredExt();
		const lightData_Global_t& globalLight = pExt->GetLightData_Global();

		DECLARE_DYNAMIC_PIXEL_SHADER(composite_translucent_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		////SET_DYNAMIC_PIXEL_SHADER_COMBO(USE_GLOBAL_LIGHT, globalLight.bEnabled);
		SET_DYNAMIC_PIXEL_SHADER(composite_translucent_ps30);

		if (bModel && bFastVTex)
		{
			bool bUnusedTexCoords[3] = { false, true, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
			pShaderAPI->MarkUnusedVertexFields(0, 3, bUnusedTexCoords);
		}

		ITexture* pSource = materials->FindTexture("_rt_fullframefb", TEXTURE_GROUP_RENDER_TARGET);

		pShader->BindTexture(SHADER_SAMPLER11, pSource);

		ITexture* pDepthTexture = materials->FindTexture("_rt_FullFrameDepth", TEXTURE_GROUP_RENDER_TARGET);

		pShader->BindTexture(SHADER_SAMPLER12, pDepthTexture);

		//pShader->BindTexture(SHADER_SAMPLER11, GetDeferredExt()->GetTexture_ForwardData());

		lightData_Global_t dataGlobal = GetDeferredExt()->GetLightData_Global();

		//if (dataGlobal.bShadow)
		//{
		//	for (int i = 0; i < 4; i++)
		//	{
		//		const shadowData_ortho_t& data = GetDeferredExt()->GetShadowData_Ortho(i);

		//		pShader->BindTexture(SHADER_SAMPLER11, GetDeferredExt()->GetTexture_ShadowDepth_Ortho(0));

		//		COMPILE_TIME_ASSERT(CSM_USE_COMPOSITED_TARGET == 1); // This shader relies on composited cascades!
		//		COMPILE_TIME_ASSERT(SHADOW_NUM_CASCADES == 2); // This shader has been made for 2 cascades!
		//		
		//		pShaderAPI->SetPixelShaderConstant(16, data.matWorldToTexture.Base(), 3);
		//		pShaderAPI->SetPixelShaderConstant(22, data.vecUVTransform.Base());
		//		pShaderAPI->SetPixelShaderConstant(24, data.vecSlopeSettings.Base());

		//		float fl_0[4] = { 0, 0, 0, 0 };
		//		float fl_1[4] = { 0, 0, 0, 0 };

		//		MakeShadowProjectionConstants(fl_0, fl_1, data.iRes_x, data.iRes_y);

		//		pShaderAPI->SetPixelShaderConstant(26, fl_0);
		//		pShaderAPI->SetPixelShaderConstant(8, fl_1);
		//	}
		//}

		pShaderAPI->ExecuteCommandBuffer(pDeferredContext->GetCommands(CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE));

		float vEyePos_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraPosition(vEyePos_SpecExponent);
		vEyePos_SpecExponent[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant(3, vEyePos_SpecExponent, 1);

		float vEyeDir_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraDirection(vEyeDir_SpecExponent);
		vEyeDir_SpecExponent[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant(4, vEyeDir_SpecExponent, 1);

		int numForwardLights = pExt->GetNumActiveForwardLights();

		float forwardLightCount[4] = { (float)numForwardLights, 0, 0, 0 };
		pShaderAPI->SetPixelShaderConstant(11, forwardLightCount);

		if (pShaderAPI != NULL && numForwardLights > 0 && numForwardLights < 14)
		{
			float* pLightData = pExt->GetForwardLightData();
			if (pLightData)
			{
				pShaderAPI->SetPixelShaderConstant(63,
					pLightData,
					pExt->GetForwardLights_NumRows());
			}
		}
		
		float* pSpotlightData = pExt->GetForwardSpotlightData();
		if (pSpotlightData)
		{
			pShaderAPI->SetPixelShaderConstant(38,
				pSpotlightData,
				pExt->GetForwardSpotLights_NumRows());
		}

		if (globalLight.bEnabled)
		{
			float globalLightData[4];

			globalLightData[0] = globalLight.vecLight.x;
			globalLightData[1] = globalLight.vecLight.y;
			globalLightData[2] = globalLight.vecLight.z;
			globalLightData[3] = globalLight.bShadow ? 1.0f : 0.0f;
			pShaderAPI->SetPixelShaderConstant(12, globalLightData, 1);

			pShaderAPI->SetPixelShaderConstant(13, globalLight.diff.Base(), 1);
			pShaderAPI->SetPixelShaderConstant(14, globalLight.ambh.Base(), 1);
			pShaderAPI->SetPixelShaderConstant(15, globalLight.ambl.Base(), 1);
		}

		// These constants are used to rotate the world space water normals around the up axis to align the
		// normal with the camera and then give us a 2D offset vector to use for reflection and refraction uv's
		VMatrix mView;
		pShaderAPI->GetMatrix(MATERIAL_VIEW, mView.m[0]);
		mView = mView.Transpose3x3();

		Vector4D vCameraRight(mView.m[0][0], mView.m[0][1], mView.m[0][2], 0.0f);
		vCameraRight.z = 0.0f; // Project onto the plane of water
		vCameraRight.AsVector3D().NormalizeInPlace();

		Vector4D vCameraForward;
		CrossProduct(Vector(0.0f, 0.0f, 1.0f), vCameraRight.AsVector3D(), vCameraForward.AsVector3D()); // I assume the water surface normal is pointing along z!

		////Vector4D vCameraRight(1.0f, 0.0f, 0.0f, 0.0f);   // World X axis
		////Vector4D vCameraForward(0.0f, 1.0f, 0.0f, 0.0f); // World Y axis

		//pShaderAPI->SetPixelShaderConstant(24, vCameraRight.Base());
		pShaderAPI->SetPixelShaderConstant(36, vCameraForward.Base());

		const Matrix_Data_t& data = GetDeferredExt()->GetCommonData();

		pShaderAPI->SetPixelShaderConstant(16, data.matViewInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(20, data.matProjInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(24, data.matView.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(31, data.matProj.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(35, data.flZDists, 2);
		pShaderAPI->SetPixelShaderConstant(36, &data.aspect, 1);
		pShaderAPI->SetPixelShaderConstant(37, &data.fov, 1);
		
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

		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, BASETEXTURETRANSFORM);
	}

	pShader->Draw();
}