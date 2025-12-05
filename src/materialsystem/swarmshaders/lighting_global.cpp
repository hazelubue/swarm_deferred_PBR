
#include "deferred_includes.h"

#include "defconstruct_vs30.inc"
#include "include/lightingpass_global_ps30.inc"

extern ConVar cl_light_specular_factor/*("cl_specular_factor", "1.4")*/;

ConVar cl_light_specular_global_boost("cl_light_specular_global_boost", ".1", FCVAR_CHEAT);
extern ConVar cl_light_specular_grazing_factor;
extern ConVar cl_light_specular_grazing_power;
ConVar cl_light_specular_global_size("cl_light_specular_global_size", "10", FCVAR_CHEAT);
//extern ConVar cl_light_gi_enable;
//extern ConVar cl_light_gi_power;
//extern ConVar cl_light_roughness_scale;
extern ConVar cl_light_specular_scale;
ConVar cl_light_diffuse_strength_global("cl_light_diffuse_strength_global", "0.01", FCVAR_CHEAT);
extern ConVar cl_light_fresnel_strength;
extern ConVar cl_light_Sheen_strength;
extern ConVar cl_light_specular_roughness;


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


BEGIN_VS_SHADER(LIGHTING_GLOBAL, "")
BEGIN_SHADER_PARAMS

END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
}

SHADER_INIT
{
}

SHADER_FALLBACK
{
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->SetDefaultState();
		pShaderShadow->EnableDepthTest(false);
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableAlphaWrites(true);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER2, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER14, true);

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);

		DECLARE_STATIC_VERTEX_SHADER(defconstruct_vs30);
		SET_STATIC_VERTEX_SHADER_COMBO(USEWORLDTRANSFORM, 0);
		SET_STATIC_VERTEX_SHADER_COMBO(SENDWORLDPOS, 0);
		SET_STATIC_VERTEX_SHADER(defconstruct_vs30);

		DECLARE_STATIC_PIXEL_SHADER(lightingpass_global_ps30);
		SET_STATIC_PIXEL_SHADER(lightingpass_global_ps30);
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		lightData_Global_t data = GetDeferredExt()->GetLightData_Global();

		AssertMsg(data.bEnabled, "I shouldn't be drawn at all.");

		DECLARE_DYNAMIC_VERTEX_SHADER(defconstruct_vs30);
		SET_DYNAMIC_VERTEX_SHADER(defconstruct_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(lightingpass_global_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(HAS_SHADOW, data.bShadow);
		SET_DYNAMIC_PIXEL_SHADER(lightingpass_global_ps30);

		BindTexture(SHADER_SAMPLER0, GetDeferredExt()->GetTexture_Normals());
		BindTexture(SHADER_SAMPLER1, GetDeferredExt()->GetTexture_Depth());

		BindTexture(SHADER_SAMPLER14, GetDeferredExt()->GetTexture_LightCtrl());

		if (data.bShadow)
		{
			BindTexture(SHADER_SAMPLER2, GetDeferredExt()->GetTexture_ShadowDepth_Ortho(0));

			COMPILE_TIME_ASSERT(CSM_USE_COMPOSITED_TARGET == 1); // This shader relies on composited cascades!
			COMPILE_TIME_ASSERT(SHADOW_NUM_CASCADES == 2); // This shader has been made for 2 cascades!

			CommitShadowProjectionConstants_Ortho_Composite(pShaderAPI, 2, 2);
		}

		CommitGlobalLightForward(pShaderAPI, 1);

		CommitBaseDeferredConstants_Frustum(pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_0);
		CommitBaseDeferredConstants_Origin(pShaderAPI, 0);

		float lightSpecularGlobalSize[1];
		UTIL_StringToFloatArray(lightSpecularGlobalSize, 1, cl_light_specular_global_size.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_22, lightSpecularGlobalSize);

		float LightSpecularStrengthBoost[1];
		UTIL_StringToFloatArray(LightSpecularStrengthBoost, 1, cl_light_specular_global_boost.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_21, LightSpecularStrengthBoost);

		float LightSpecularScale[1];
		UTIL_StringToFloatArray(LightSpecularScale, 1, cl_light_specular_scale.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_26, LightSpecularScale);

		float LightDiffuseStrengthGlobal[1];
		UTIL_StringToFloatArray(LightDiffuseStrengthGlobal, 1, cl_light_diffuse_strength_global.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_27, LightDiffuseStrengthGlobal);

		float lightSheenStrength[1];
		UTIL_StringToFloatArray(lightSheenStrength, 1, cl_light_Sheen_strength.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_29, lightSheenStrength);


		pShaderAPI->SetPixelShaderConstant(16, data.diff.Base());
		pShaderAPI->SetPixelShaderConstant(17, data.ambh.Base());
		pShaderAPI->SetPixelShaderConstant(18, MakeHalfAmbient(data.ambl, data.ambh).Base());
	}

	Draw();
}

END_SHADER