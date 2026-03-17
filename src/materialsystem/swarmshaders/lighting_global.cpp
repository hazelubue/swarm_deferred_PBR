
#include "deferred_includes.h"

#include "defconstruct_vs30.inc"
#include "include/lightingpass_global_illumination_ps30.inc"

extern ConVar cl_light_specular_factor;

ConVar cl_light_specular_global_boost("cl_light_specular_global_boost", ".1", FCVAR_CHEAT);
extern ConVar cl_light_specular_grazing_factor;
extern ConVar cl_light_specular_grazing_power;
ConVar cl_light_specular_global_size("cl_light_specular_global_size", "10", FCVAR_CHEAT);

extern ConVar cl_light_specular_scale;
ConVar cl_light_diffuse_strength_global("cl_light_diffuse_strength_global", "0.01", FCVAR_CHEAT);
extern ConVar cl_light_fresnel_strength;
extern ConVar cl_light_Sheen_strength;
extern ConVar cl_light_specular_roughness;

extern ConVar r_ss_distortion;
extern ConVar r_ss_power;
extern ConVar r_ss_scale;
extern ConVar r_debug_translucent_pipeline;
extern ConVar r_ibl_bluramt;
extern ConVar r_enable_ssr;

ConVar r_sss_thickness("r_sss_thickness", "0.653");

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
		pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER14, true);

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);

		DECLARE_STATIC_VERTEX_SHADER(defconstruct_vs30);
		SET_STATIC_VERTEX_SHADER_COMBO(USEWORLDTRANSFORM, 0);
		SET_STATIC_VERTEX_SHADER_COMBO(SENDWORLDPOS, 0);
		SET_STATIC_VERTEX_SHADER(defconstruct_vs30);

		DECLARE_STATIC_PIXEL_SHADER(lightingpass_global_illumination_ps30);
		SET_STATIC_PIXEL_SHADER(lightingpass_global_illumination_ps30);
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		lightData_Global_t data = GetDeferredExt()->GetLightData_Global();

		AssertMsg(data.bEnabled, "I shouldn't be drawn at all.");

		DECLARE_DYNAMIC_VERTEX_SHADER(defconstruct_vs30);
		SET_DYNAMIC_VERTEX_SHADER(defconstruct_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(lightingpass_global_illumination_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(HAS_SHADOW, data.bShadow);
		SET_DYNAMIC_PIXEL_SHADER(lightingpass_global_illumination_ps30);

		BindTexture(SHADER_SAMPLER0, GetDeferredExt()->GetTexture_Normals());
		BindTexture(SHADER_SAMPLER1, GetDeferredExt()->GetTexture_Depth());
		BindTexture(SHADER_SAMPLER3, GetDeferredExt()->GetTexture_WaterNormals());
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

		// Data passed from viewrender cpu side.
		Matrix_Data_t data2 = GetDeferredExt()->GetCommonData();

		pShaderAPI->SetPixelShaderConstant(108, data2.matViewInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(112, data2.matProjInv.Base(), 4);
		//pShaderAPI->SetPixelShaderConstant(42, data.matStaticViewInv.Base(), 4);
		pShaderAPI->SetPixelShaderConstant(36, data2.flZDists, 2);
		pShaderAPI->SetPixelShaderConstant(37, &data2.aspect, 1);
		pShaderAPI->SetPixelShaderConstant(38, &data2.fov, 1);

		// end data passed from viewrender.
		VMatrix mView;
		VMatrix mProj;
		pShaderAPI->GetMatrix(MATERIAL_VIEW, mView.m[0]);
		pShaderAPI->GetMatrix(MATERIAL_PROJECTION, mProj.m[0]);

		Vector4D vViewRight(mView.m[0][0], mView.m[1][0], mView.m[2][0], 0.0f);
		Vector4D vViewUp(mView.m[0][1], mView.m[1][1], mView.m[2][1], 0.0f);
		Vector4D vViewForward(mView.m[0][2], mView.m[1][2], mView.m[2][2], 0.0f);

		float vViewOrigin[4];
		pShaderAPI->GetWorldSpaceCameraPosition(vViewOrigin);
		vViewOrigin[3] = 0.0f;

		pShaderAPI->SetPixelShaderConstant(23, vViewForward.Base(), 1);
		pShaderAPI->SetPixelShaderConstant(24, vViewRight.Base(), 1);
		pShaderAPI->SetPixelShaderConstant(25, vViewUp.Base(), 1);
		pShaderAPI->SetPixelShaderConstant(26, vViewOrigin, 1);

		pShaderAPI->SetPixelShaderConstant(100, mView.m[0], 4);
		pShaderAPI->SetPixelShaderConstant(104, mProj.m[0], 4);

		float LightSpecularStrengthBoost[1];
		UTIL_StringToFloatArray(LightSpecularStrengthBoost, 1, cl_light_specular_global_boost.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_19, LightSpecularStrengthBoost);

		float LightDiffuseStrengthGlobal[1];
		UTIL_StringToFloatArray(LightDiffuseStrengthGlobal, 1, cl_light_diffuse_strength_global.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_20, LightDiffuseStrengthGlobal);

		float lightSheenStrength[1];
		UTIL_StringToFloatArray(lightSheenStrength, 1, cl_light_Sheen_strength.GetString());
		pShaderAPI->SetPixelShaderConstant(PSREG_CONSTANT_22, lightSheenStrength);

		int x, y, w, t;
		pShaderAPI->GetCurrentViewport(x, y, w, t);
		float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

		pShaderAPI->SetPixelShaderConstant(116, fl1);

		float flthickness = r_sss_thickness.GetFloat();;

		pShaderAPI->SetPixelShaderConstant(118, &flthickness, 1);

		float flscatteringVars[3];
		flscatteringVars[0] = r_ss_distortion.GetFloat();
		flscatteringVars[1] = r_ss_power.GetFloat();
		flscatteringVars[2] = r_ss_scale.GetFloat();

		pShaderAPI->SetPixelShaderConstant(117, flscatteringVars, 1);

		pShaderAPI->SetPixelShaderConstant(16, data.diff.Base());
		pShaderAPI->SetPixelShaderConstant(17, data.ambh.Base());
		pShaderAPI->SetPixelShaderConstant(18, MakeHalfAmbient(data.ambl, data.ambh).Base());
	}

	Draw();
}

END_SHADER