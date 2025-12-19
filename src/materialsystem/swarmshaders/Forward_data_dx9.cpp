
#include "deferred_includes.h"

#include "include/forward_data_vs30.inc"
#include "include/forward_data_ps30.inc"

BEGIN_VS_SHADER(WriteFloatData, "")
BEGIN_SHADER_PARAMS

END_SHADER_PARAMS

//void SetupParamsFowardData(SetupParamsFowardData& p)
//{
//}

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
			pShaderShadow->VertexShaderVertexFormat(
				VERTEX_POSITION, 0, 0, 0);

            pShaderShadow->EnableAlphaTest(false);
            pShaderShadow->EnableBlending(false);

            pShaderShadow->EnableDepthWrites(true);
            pShaderShadow->EnableDepthTest(true);
            pShaderShadow->DepthFunc(SHADER_DEPTHFUNC_NEAREROREQUAL);
            pShaderShadow->EnableCulling(true);

			DECLARE_STATIC_VERTEX_SHADER(forward_data_vs30);
			SET_STATIC_VERTEX_SHADER(forward_data_vs30);

			DECLARE_STATIC_PIXEL_SHADER(forward_data_ps30);
			SET_STATIC_PIXEL_SHADER(forward_data_ps30);
		}
		DYNAMIC_STATE
		{
			LoadModelViewMatrixIntoVertexShaderConstant(VERTEX_SHADER_MODELVIEWPROJ);

			DECLARE_DYNAMIC_VERTEX_SHADER(forward_data_vs30);
			SET_DYNAMIC_VERTEX_SHADER(forward_data_vs30);

			DECLARE_DYNAMIC_PIXEL_SHADER(forward_data_ps30);
			SET_DYNAMIC_PIXEL_SHADER(forward_data_ps30);
		}

	

	// This loads the texture transform into register c8-c9
	SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, BASETEXTURETRANSFORM);

    Draw();
}

END_SHADER