#include "deferred_includes.h"

#include "defconstruct_vs30.inc"
#include "include/lightingpass_pbr_point_ps30.inc"
#include "include/lightingpass_spot_ps30.inc"
#include "tier0/memdbgon.h"

extern ConVar mat_specular;

ConVar cl_light_specular_spot_boost("cl_light_specular_spot_boost", "0.5", FCVAR_CHEAT); // was .1 // was .43
ConVar cl_light_specular_point_boost("cl_light_specular_point_boost", "0.5", FCVAR_CHEAT); //was .1 // was .43
ConVar cl_light_specular_spot_size("cl_light_specular_spot_size", "0.001", FCVAR_CHEAT);
ConVar cl_light_specular_point_size("cl_light_specular_point_size", "0.001", FCVAR_CHEAT);
ConVar cl_light_specular_brightness_spot("cl_light_specular_brightness_spot", "25.0", FCVAR_CHEAT);
ConVar cl_light_specular_scale("cl_light_specular_scale", "2", FCVAR_CHEAT);
ConVar cl_light_diffuse_strength_point("cl_light_diffuse_strength_point", "1", FCVAR_CHEAT);
ConVar cl_light_diffuse_strength_spot("cl_light_diffuse_strength_spot", "1", FCVAR_CHEAT);
ConVar cl_light_fresnel_strength("cl_light_fresnel_strength", "10.0", FCVAR_CHEAT);
ConVar cl_light_Sheen_strength("cl_light_Sheen_strength", "0.01", FCVAR_CHEAT);

//make a system that detects if mrao is defined then resort to making mrao render with dedicated texture
//otherwise we use auto generation thus making the value for 1.86 to 10 when dedicated is detected.
//ConVar cl_light_MRAO_green("cl_light_MRAO_green", "1.86"); // defualt for MRAO dedicated textures is 10.
ConVar cl_light_MRAO_blue("cl_light_MRAO_blue", "2.0");

static ConVar cl_light_MRAO_green("cl_light_MRAO_green", "10.0");
static ConVar cl_light_MRAO_green2("cl_light_MRAO_green2", "18.66"); // 8.66 for REMASTERED

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

void InitParmsLightPass(const lightPassParms& info, CBaseVSShader* pShader, IMaterialVar** params)
{
	if (!PARM_DEFINED(info.iLightTypeVar))
		params[info.iLightTypeVar]->SetIntValue(DEFLIGHTTYPE_POINT);
}


void InitPassLightPass(const lightPassParms& info, CBaseVSShader* pShader, IMaterialVar** params)
{
}


void DrawPassLightPass(const lightPassParms& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression)
{
	const int bWorldProjection = PARM_SET(info.iWorldProjection);

	const int iLightType = PARM_INT(info.iLightTypeVar);
	const bool bPoint = iLightType == DEFLIGHTTYPE_POINT;

	SHADOW_STATE
	{
		pShaderShadow->SetDefaultState();
		pShaderShadow->EnableDepthTest(false);
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableAlphaWrites(true);

		pShader->EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER15, true);

		for (int i = 0; i < FREE_LIGHT_SAMPLERS; i++)
		{
			pShaderShadow->EnableTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + i), true);
		}

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);

		DECLARE_STATIC_VERTEX_SHADER(defconstruct_vs30);
		SET_STATIC_VERTEX_SHADER_COMBO(USEWORLDTRANSFORM, bWorldProjection ? 1 : 0);
		SET_STATIC_VERTEX_SHADER_COMBO(SENDWORLDPOS, 0);
		SET_STATIC_VERTEX_SHADER(defconstruct_vs30);

		switch (iLightType)
		{
			case DEFLIGHTTYPE_POINT:
			{
				DECLARE_STATIC_PIXEL_SHADER(lightingpass_pbr_point_ps30);
				SET_STATIC_PIXEL_SHADER_COMBO(USEWORLDTRANSFORM, bWorldProjection ? 1 : 0);
				//SET_STATIC_PIXEL_SHADER_COMBO(DISABLE_MRAO, false);
				SET_STATIC_PIXEL_SHADER(lightingpass_pbr_point_ps30);
			}
			break;
			case DEFLIGHTTYPE_SPOT:
			{
				DECLARE_STATIC_PIXEL_SHADER(lightingpass_spot_ps30);
				SET_STATIC_PIXEL_SHADER_COMBO(USEWORLDTRANSFORM, bWorldProjection ? 1 : 0);
				//SET_STATIC_PIXEL_SHADER_COMBO(DISABLE_MRAO, disaleMRAO);
				SET_STATIC_PIXEL_SHADER(lightingpass_spot_ps30);
			}
			break;
		}
	}
		DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		CDeferredExtension* pExt = GetDeferredExt();

		Assert(pExt->GetActiveLightData() != NULL);
		Assert(pExt->GetActiveLights_NumRows() != NULL);

		const int iNumShadowedCookied = pExt->GetNumActiveLights_ShadowedCookied();
		const int iNumShadowed = pExt->GetNumActiveLights_Shadowed();
		const int iNumCookied = pExt->GetNumActiveLights_Cookied();

		DECLARE_DYNAMIC_VERTEX_SHADER(defconstruct_vs30);
		SET_DYNAMIC_VERTEX_SHADER(defconstruct_vs30);

		switch (iLightType)
		{
			case DEFLIGHTTYPE_POINT:
			{
				DECLARE_DYNAMIC_PIXEL_SHADER(lightingpass_pbr_point_ps30);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_SHADOWED_COOKIE, iNumShadowedCookied);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_SHADOWED, iNumShadowed);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_COOKIE, iNumCookied);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_SIMPLE, pExt->GetNumActiveLights_Simple());
				//SET_DYNAMIC_PIXEL_SHADER_COMBO(SMOOTHNESS, bUseSmoothness);
				//SET_DYNAMIC_PIXEL_SHADER_COMBO(SPECULAR_AFFECTS_ROUGHNESS, iSpecRough);
				SET_DYNAMIC_PIXEL_SHADER(lightingpass_pbr_point_ps30);
			}
			break;
			case DEFLIGHTTYPE_SPOT:
			{
				DECLARE_DYNAMIC_PIXEL_SHADER(lightingpass_spot_ps30);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_SHADOWED_COOKIE, iNumShadowedCookied);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_SHADOWED, iNumShadowed);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_COOKIE, iNumCookied);
				//SET_DYNAMIC_PIXEL_SHADER_COMBO( SMOOTHNESS, bUseSmoothness);
				SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_SIMPLE, pExt->GetNumActiveLights_Simple());
				SET_DYNAMIC_PIXEL_SHADER(lightingpass_spot_ps30);
			}
			break;

		}

		pShader->BindTexture(SHADER_SAMPLER0, GetDeferredExt()->GetTexture_Normals());
		pShader->BindTexture(SHADER_SAMPLER1, GetDeferredExt()->GetTexture_Depth());

		//implement later on!
		pShader->BindTexture(SHADER_SAMPLER15, GetDeferredExt()->GetTexture_LightCtrl());

		//implement later on!
		//pShader->BindTexture(SHADER_SAMPLER14, GetDeferredExt()->GetTexture_Alpha());

		int iSampler = 0;
		int iShadow = 0;
		int iCookie = 0;

		for (; iSampler < (iNumShadowedCookied * 2);)
		{
			ITexture* pDepth = bPoint ? GetDeferredExt()->GetTexture_ShadowDepth_DP(iShadow) :
				GetDeferredExt()->GetTexture_ShadowDepth_Proj(iShadow);

			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler), pDepth);
			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler + 1), GetDeferredExt()->GetTexture_Cookie(iCookie));

			iSampler += 2;
			iShadow++;
			iCookie++;
		}

		for (; iSampler < (iNumShadowedCookied * 2 + iNumShadowed); )
		{
			ITexture* pDepth = bPoint ? GetDeferredExt()->GetTexture_ShadowDepth_DP(iShadow) :
				GetDeferredExt()->GetTexture_ShadowDepth_Proj(iShadow);

			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler), pDepth);

			iSampler++;
			iShadow++;
		}

		for (; iSampler < (iNumShadowedCookied * 2 + iNumShadowed + iNumCookied); )
		{
			pShader->BindTexture((Sampler_t)(FIRST_LIGHT_SAMPLER + iSampler), GetDeferredExt()->GetTexture_Cookie(iCookie));

			iSampler++;
			iCookie++;
		}

		const int frustumReg = bWorldProjection ? 3 : VERTEX_SHADER_SHADER_SPECIFIC_CONST_0;
		CommitBaseDeferredConstants_Frustum(pShaderAPI, frustumReg, !bWorldProjection);
		CommitBaseDeferredConstants_Origin(pShaderAPI, 0);

		switch (iLightType)
		{
		case DEFLIGHTTYPE_POINT:
				CommitShadowProjectionConstants_DPSM(pShaderAPI, 1);
			break;
		case DEFLIGHTTYPE_SPOT:
				CommitShadowProjectionConstants_Proj(pShaderAPI, 1);
			break;
		}

		pShaderAPI->SetPixelShaderConstant(FIRST_SHARED_LIGHTDATA_CONSTANT,
			pExt->GetActiveLightData(),
			pExt->GetActiveLights_NumRows());

		if (bWorldProjection)
		{
			CommitHalfScreenTexel(pShaderAPI, 6);
		}

		/*float LightSpecFact[1];
		UTIL_StringToFloatArray(LightSpecFact, 1, cl_light_specular_factor.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_10, LightSpecFact);*/

		float LightSpotBoost[1];
		UTIL_StringToFloatArray(LightSpotBoost, 1, cl_light_specular_spot_boost.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_13, LightSpotBoost);

		float LightSpotSize[1];
		UTIL_StringToFloatArray(LightSpotSize, 1, cl_light_specular_spot_size.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_14, LightSpotSize);

		float LightPointSize[1];
		UTIL_StringToFloatArray(LightPointSize, 1, cl_light_specular_point_size.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_15, LightPointSize);

		float LightPointBoost[1];
		UTIL_StringToFloatArray(LightPointBoost, 1, cl_light_specular_point_boost.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_16, LightPointBoost);

		/*float LightSpecScale[1];
		UTIL_StringToFloatArray(LightSpecScale, 1, cl_light_specular_scale.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_20, LightSpecScale);*/

		float lightDiffuseStrengthPoint[1];
		UTIL_StringToFloatArray(lightDiffuseStrengthPoint, 1, cl_light_diffuse_strength_point.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_21, lightDiffuseStrengthPoint);

		float LightBrightnessSpot[1];
		UTIL_StringToFloatArray(LightBrightnessSpot, 1, cl_light_specular_brightness_spot.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_24, LightBrightnessSpot);

		float LightFresnelStrength[1];
		UTIL_StringToFloatArray(LightFresnelStrength, 1, cl_light_fresnel_strength.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_22, LightFresnelStrength);

		float lightSheenStrength[1];
		UTIL_StringToFloatArray(lightSheenStrength, 1, cl_light_Sheen_strength.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_23, lightSheenStrength);

		float LightDiffuseStrengthSpot[1];
		UTIL_StringToFloatArray(LightDiffuseStrengthSpot, 1, cl_light_diffuse_strength_spot.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_25, LightDiffuseStrengthSpot);

		/*float LightMRAOGreen[1];
		UTIL_StringToFloatArray(LightMRAOGreen, 1, cl_light_MRAO_green.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_27, LightMRAOGreen);

		float lightMRAOBlue[1];
		UTIL_StringToFloatArray(lightMRAOBlue, 1, cl_light_MRAO_blue.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_28, lightMRAOBlue);

		float lightMRAOGreen2[1];
		UTIL_StringToFloatArray(lightMRAOGreen2, 1, cl_light_MRAO_green2.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_30, lightMRAOGreen2);*/

	}

	pShader->Draw();
}