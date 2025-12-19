//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "deferred_screenspace.h"
#include "include/screenspaceeffect_vs30.inc"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"
#include "IDeferredExt.h"
#include "lighting_helper.h"

ConVar r_post_ssr_steps("r_post_ssr_steps", "0.035", FCVAR_CHEAT);
ConVar r_post_ssr_rayAmount("r_post_ssr_rayAmount", "0.00", FCVAR_CHEAT);
ConVar r_post_ssr_minSteps("r_post_ssr_minSteps", "0.01", FCVAR_CHEAT);
ConVar r_post_ssr_gauss_steps("r_post_ssr_gauss_steps", "1.67", FCVAR_CHEAT);
ConVar r_post_ssr_noise_intensity("r_post_ssr_noise_intensity", "1.67", FCVAR_CHEAT);
ConVar r_post_L_world_scalar("r_post_L_world_scalar", "1.0", FCVAR_CHEAT);


static void GetAdjustedShaderName(char* pOutputBuffer, char const* pShader20Name)
{
	Q_strcpy(pOutputBuffer, pShader20Name);
	if (g_pHardwareConfig->SupportsPixelShaders_2_b())
	{
		size_t iLength = Q_strlen(pOutputBuffer);
		if ((iLength > 4) &&
			(
				(Q_stricmp(pOutputBuffer + iLength - 5, "_ps20") == 0) ||
				(Q_stricmp(pOutputBuffer + iLength - 5, "_vs20") == 0)
				)
			)
		{
			strcpy(pOutputBuffer + iLength - 2, "20b");
		}
	}
}

void InitParmsScreenspace(const defParms_screenspace& info, CBaseVSShader* pShader, IMaterialVar** params)
{
	if (!params[info.TCSIZE0]->IsDefined())
		params[info.TCSIZE0]->SetIntValue(2);
}

void InitPassScreenspace(const defParms_screenspace& info, CBaseVSShader* pShader, IMaterialVar** params)
{

	if (params[info.BASETEXTURE]->IsDefined())
	{
		pShader->LoadTexture(info.BASETEXTURE);
	}
	if (params[info.TEXTURE1]->IsDefined())
	{
		pShader->LoadTexture(info.TEXTURE1);
	}
	if (params[info.TEXTURE2]->IsDefined())
	{
		pShader->LoadTexture(info.TEXTURE2);
	}
	if (params[info.TEXTURE3]->IsDefined())
	{
		pShader->LoadTexture(info.TEXTURE3);
	}
	/*if (params[info.TEXTURE4]->IsDefined())
	{
		pShader->LoadTexture(info.TEXTURE3);
	}*/
}


void DrawPassScreenspace(const defParms_screenspace& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext)
{

	bool bCustomVertexShader = params[info.VERTEXSHADER]->IsDefined();
	const bool bPoint = DEFLIGHTTYPE_POINT;
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(params[info.WRITEDEPTH]->GetIntValue() != 0);
		if (params[info.WRITEDEPTH]->GetIntValue() != 0)
		{
			pShaderShadow->EnableDepthTest(true);
			pShaderShadow->DepthFunc(SHADER_DEPTHFUNC_ALWAYS);
		}
		pShaderShadow->EnableAlphaWrites(params[info.WRITEALPHA]->GetIntValue() != 0);
		pShaderShadow->EnableDepthTest(params[info.DEPTHTEST]->GetIntValue() != 0);
		pShaderShadow->EnableCulling(params[info.CULL]->GetIntValue() != 0);

		if (params[info.BASETEXTURE]->IsDefined())
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, !params[info.LINEARREAD_BASETEXTURE]->IsDefined() || !params[info.LINEARREAD_BASETEXTURE]->GetIntValue());
		}
		if (params[info.TEXTURE1]->IsDefined())
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, !params[info.LINEARREAD_TEXTURE1]->IsDefined() || !params[info.LINEARREAD_TEXTURE1]->GetIntValue());
		}
		if (params[info.TEXTURE2]->IsDefined())
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER2, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER2, !params[info.LINEARREAD_TEXTURE2]->IsDefined() || !params[info.LINEARREAD_TEXTURE2]->GetIntValue());
		}
		if (params[info.TEXTURE3]->IsDefined())
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3,false);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, !params[info.LINEARREAD_TEXTURE3]->IsDefined() || !params[info.LINEARREAD_TEXTURE3]->GetIntValue());
		}
		/*if (params[info.TEXTURE4]->IsDefined())
		{
			pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, false);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, !params[info.LINEARREAD_TEXTURE4]->IsDefined() || !params[info.LINEARREAD_TEXTURE3]->GetIntValue());
		}*/

		int fmt = VERTEX_POSITION;

		if (params[info.VERTEXCOLOR]->GetIntValue())
		{
			fmt |= VERTEX_COLOR;
		}



		int nTexCoordSize[8];
		int nNumTexCoords = 0;

		static int s_tcSizeIds[] = { info.TCSIZE0, info.TCSIZE1, info.TCSIZE2, info.TCSIZE3, info.TCSIZE4, info.TCSIZE5, info.TCSIZE6, info.TCSIZE7 };

		int nCtr = 0;
		while (nCtr < 8 && (params[s_tcSizeIds[nCtr]]->GetIntValue()))
		{
			nNumTexCoords++;
			nTexCoordSize[nCtr] = params[s_tcSizeIds[nCtr]]->GetIntValue();
			nCtr++;
		}
		pShaderShadow->VertexShaderVertexFormat(fmt, nNumTexCoords, nTexCoordSize, 0);

		if (IS_FLAG_SET(MATERIAL_VAR_ADDITIVE))
		{
			pShader->EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE);
		}
		else if (params[info.MULTIPLYCOLOR]->GetIntValue())
		{
			pShader->EnableAlphaBlending(SHADER_BLEND_ZERO, SHADER_BLEND_SRC_COLOR);
		}
		else if (params[info.ALPHABLEND]->GetIntValue())
		{
			pShader->EnableAlphaBlending(SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			pShaderShadow->EnableBlending(false);
		}

		// maybe convert from linear to gamma on write.
		bool srgb_write = true;
		if (params[info.LINEARWRITE]->GetFloatValue())
			srgb_write = false;
		pShaderShadow->EnableSRGBWrite(srgb_write);

		char szShaderNameBuf[256];

		if (bCustomVertexShader)
		{
			GetAdjustedShaderName(szShaderNameBuf, params[info.VERTEXSHADER]->GetStringValue());
			pShaderShadow->SetVertexShader(params[info.VERTEXSHADER]->GetStringValue(), 0); //szShaderNameBuf, 0 );
		}
		else
		{
			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER(screenspaceeffect_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO_HAS_DEFAULT(VERTEXCOLOR, params[info.VERTEXCOLOR]->GetIntValue());
			SET_STATIC_VERTEX_SHADER_COMBO_HAS_DEFAULT(TRANSFORMVERTS, params[info.VERTEXTRANSFORM]->GetIntValue());
			SET_STATIC_VERTEX_SHADER(screenspaceeffect_vs30);
		}

		if (params[info.DISABLE_COLOR_WRITES]->GetIntValue())
		{
			pShaderShadow->EnableColorWrites(false);
		}
		if (params[info.ALPHATESTED]->GetFloatValue())
		{
			pShaderShadow->EnableAlphaTest(true);
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GREATER,0.0);
		}

		GetAdjustedShaderName(szShaderNameBuf, params[info.PIXSHADER]->GetStringValue());
		pShaderShadow->SetPixelShader(szShaderNameBuf, 0);
		if (params[info.ALPHA_BLEND_COLOR_OVERLAY]->GetIntValue())
		{
			// Used for adding L4D halos
			pShader->EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
		}
		if (params[info.ALPHA_BLEND]->GetIntValue())
		{
			// Used for adding L4D halos
			pShader->EnableAlphaBlending(SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
		}


		if (params[info.COPYALPHA]->GetIntValue())
		{
			pShaderShadow->EnableBlending(false);
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_ALWAYS, 0.0f);
		}

	}

	DYNAMIC_STATE
	{
		if (params[info.BASETEXTURE]->IsDefined())
		{
			pShader->BindTexture(SHADER_SAMPLER0, info.BASETEXTURE, -1);
			if (params[info.POINTSAMPLE_BASETEXTURE]->GetIntValue())
				pShaderAPI->SetTextureFilterMode(SHADER_SAMPLER0, TFILTER_MODE_POINTSAMPLED);
		}
		if (params[info.TEXTURE1]->IsDefined())
		{
			pShader->BindTexture(SHADER_SAMPLER1, info.TEXTURE1, -1);
			if (params[info.POINTSAMPLE_TEXTURE1]->GetIntValue())
				pShaderAPI->SetTextureFilterMode(SHADER_SAMPLER1, TFILTER_MODE_POINTSAMPLED);
		}
		if (params[info.TEXTURE2]->IsDefined())
		{
			pShader->BindTexture(SHADER_SAMPLER2, info.TEXTURE2, -1);
			if (params[info.POINTSAMPLE_TEXTURE2]->GetIntValue())
				pShaderAPI->SetTextureFilterMode(SHADER_SAMPLER2, TFILTER_MODE_POINTSAMPLED);
		}
		if (params[info.TEXTURE3]->IsDefined())
		{
			pShader->BindTexture(SHADER_SAMPLER3, info.TEXTURE3, -1);
			if (params[info.POINTSAMPLE_TEXTURE3]->GetIntValue())
				pShaderAPI->SetTextureFilterMode(SHADER_SAMPLER3, TFILTER_MODE_POINTSAMPLED);
		}
		/*if (params[info.TEXTURE4]->IsDefined())
		{
			pShader->BindTexture(SHADER_SAMPLER4, info.TEXTURE4, -1);
			if (params[info.POINTSAMPLE_TEXTURE4]->GetIntValue())
				pShaderAPI->SetTextureFilterMode(SHADER_SAMPLER4, TFILTER_MODE_POINTSAMPLED);
		}*/
		float c0[] = {
			params[info.C0_X]->GetFloatValue(),
			params[info.C0_Y]->GetFloatValue(),
			params[info.C0_Z]->GetFloatValue(),
			params[info.C0_W]->GetFloatValue(),
			params[info.C1_X]->GetFloatValue(),
			params[info.C1_Y]->GetFloatValue(),
			params[info.C1_Z]->GetFloatValue(),
			params[info.C1_W]->GetFloatValue(),
			params[info.C2_X]->GetFloatValue(),
			params[info.C2_Y]->GetFloatValue(),
			params[info.C2_Z]->GetFloatValue(),
			params[info.C2_W]->GetFloatValue(),
			params[info.C3_X]->GetFloatValue(),
			params[info.C3_Y]->GetFloatValue(),
			params[info.C3_Z]->GetFloatValue(),
			params[info.C3_W]->GetFloatValue(),
			params[info.C4_X]->GetFloatValue(),
			params[info.C4_Y]->GetFloatValue(),
			params[info.C4_Z]->GetFloatValue(),
			params[info.C4_W]->GetFloatValue()
		};

		pShaderAPI->SetPixelShaderConstant(0, c0, ARRAYSIZE(c0) / 4);

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition(eyePos);
		pShaderAPI->SetPixelShaderConstant(40, eyePos, 1);

		float eyePos2[4];
		pShaderAPI->GetWorldSpaceCameraPosition(eyePos2);
		pShaderAPI->SetPixelShaderConstant(10, eyePos2, 1);

		float flFrameCount[4];
		flFrameCount[0] = (float)pShaderAPI->CurrentTime();
		flFrameCount[1] = flFrameCount[2] = flFrameCount[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant(45, flFrameCount, 1);

		// Roughness cutoff threshold
		float flRoughnessCutoff[4];
		flRoughnessCutoff[0] = 0.8f; // Adjust this value as needed (0.0 to 1.0)
		flRoughnessCutoff[1] = flRoughnessCutoff[2] = flRoughnessCutoff[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant(46, flRoughnessCutoff, 1);

		int x, y, w, t;
		pShaderAPI->GetCurrentViewport(x, y, w, t);
		float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

		pShaderAPI->SetPixelShaderConstant(4, fl1);

		
		const Matrix_Data_t& data = GetDeferredExt()->GetCommonData();


		pShaderAPI->SetPixelShaderConstant(6, data.vecOrigin.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(8, data.matViewInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(12, data.matProjInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(16, data.matView.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(20, data.matProj.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(24, data.flZDists, 1);
		pShaderAPI->SetPixelShaderConstant(42, data.matLockedViewProjInv.Base(), 4);
		

		pShaderAPI->SetVertexShaderIndex(0);
		pShaderAPI->SetPixelShaderIndex(0);


		float flssrsteps[1];
		UTIL_StringToFloatArray(flssrsteps, 1, r_post_ssr_steps.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_26, flssrsteps);

		float flssrRays[1];
		UTIL_StringToFloatArray(flssrRays, 1, r_post_ssr_rayAmount.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_27, flssrRays);

		float flssrRaysSize[1];
		UTIL_StringToFloatArray(flssrRaysSize, 1, r_post_ssr_minSteps.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_40, flssrRaysSize);

		float flTime[4];
		flTime[0] = pShaderAPI->CurrentTime();
		flTime[1] = flTime[2] = flTime[3] = flTime[0];
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_28, flTime);

		float Gauss[1];
		UTIL_StringToFloatArray(Gauss, 1, r_post_ssr_gauss_steps.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_31, Gauss);

		float FlLWorldCtrl[1];
		UTIL_StringToFloatArray(FlLWorldCtrl, 1, r_post_L_world_scalar.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_50, FlLWorldCtrl);

		//pShaderAPI->SetPixelShaderConstant(PSREG_UBERLIGHT_SMOOTH_EDGE_0, fl1);

		CommitBaseDeferredConstants_Origin(pShaderAPI, 30);
		pShaderAPI->SetPixelShaderFogParams(36);

		float Noise[1];
		UTIL_StringToFloatArray(Noise, 1, r_post_ssr_noise_intensity.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_33, Noise);

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

		pShaderAPI->SetPixelShaderConstant(45, vCameraRight.Base());
		pShaderAPI->SetPixelShaderConstant(47, vCameraForward.Base());

		int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
		pShaderAPI->GetCurrentViewport(nViewportX, nViewportY, nViewportWidth, nViewportHeight);

		int nRtWidth, nRtHeight;
		pShaderAPI->GetCurrentRenderTargetDimensions(nRtWidth, nRtHeight);

		float vViewportMad[4];

		// viewport->screen transform
		vViewportMad[0] = (float)nViewportWidth / (float)nRtWidth;
		vViewportMad[1] = (float)nViewportHeight / (float)nRtHeight;
		vViewportMad[2] = (float)nViewportX / (float)nRtWidth;
		vViewportMad[3] = (float)nViewportY / (float)nRtHeight;
		pShaderAPI->SetPixelShaderConstant(48, vViewportMad, 1);

		//CommitSSR_Matrix(pShaderAPI);

		CDeferredExtension* pExt = GetDeferredExt();

		Assert(pExt->GetActiveLightData() != NULL);
		Assert(pExt->GetActiveLights_NumRows() != NULL);

		const int iNumShadowedCookied = pExt->GetNumActiveLights_ShadowedCookied();
		const int iNumShadowed = pExt->GetNumActiveLights_Shadowed();
		const int iNumCookied = pExt->GetNumActiveLights_Cookied();

		int iSampler = 0;
		int iShadow = 0;
		int iCookie = 0;

		for (; iSampler < (iNumShadowedCookied * 2);)
		{
			ITexture* pDepth = (bPoint) ? GetDeferredExt()->GetTexture_ShadowDepth_DP(iShadow) :
				GetDeferredExt()->GetTexture_ShadowDepth_Proj(iShadow);

			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler), pDepth);
			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler + 1), GetDeferredExt()->GetTexture_Cookie(iCookie));

			iSampler += 2;
			iShadow++;
			iCookie++;
		}

		for (; iSampler < (iNumShadowedCookied * 2 + iNumShadowed); )
		{
			ITexture* pDepth = (bPoint) ? GetDeferredExt()->GetTexture_ShadowDepth_DP(iShadow) :
				GetDeferredExt()->GetTexture_ShadowDepth_Proj(iShadow);

			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler), pDepth);

			iSampler++;
			iShadow++;
		}
		//yooo this shit is important
		for (; iSampler < (iNumShadowedCookied * 2 + iNumShadowed + iNumCookied); )
		{
			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler), GetDeferredExt()->GetTexture_Cookie(iCookie));

			iSampler++;
			iCookie++;
		}

		if (!bCustomVertexShader)
		{
			DECLARE_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs30);
			SET_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs30);
		}
	}
	pShader->Draw();
}

