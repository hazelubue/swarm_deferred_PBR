#pragma once
#include <strtools.h>
#include "BaseVSShader.h"

#ifndef DEFPASS_SCREENSPACE_H
#define DEFPASS_SCREENSPACE_H

class CDeferredPerMaterialContextData;

struct defParms_screenspace
{
	defParms_screenspace()
	{
		Q_memset(this, 0xFF, sizeof(defParms_screenspace));
	};

	int C0_X;
	int C0_Y;
	int C0_Z;
	int C0_W;
	int C1_X;
	int C1_Y;
	int C1_Z;
	int C1_W;
	int C2_X;
	int C2_Y;
	int C2_Z;
	int C2_W;
	int C3_X;
	int C3_Y;
	int C3_Z;
	int C3_W;
	int C4_X;
	int C4_Y;
	int C4_Z;
	int C4_W;
	int BASETEXTURE;
	int PIXSHADER;
	int VERTEXSHADER;
	int DISABLE_COLOR_WRITES;
	int ALPHATESTED;
	int TEXTURE1;
	int TEXTURE2;
	int TEXTURE3;
	int TEXTURE4;
	int LINEARREAD_BASETEXTURE;
	int LINEARREAD_TEXTURE1;
	int LINEARREAD_TEXTURE2;
	int LINEARREAD_TEXTURE3;
	int LINEARREAD_TEXTURE4;
	int LINEARWRITE;
	int VERTEXCOLOR;
	int VERTEXTRANSFORM;
	int ALPHABLEND;
	int MULTIPLYCOLOR;
	int WRITEALPHA;
	int WRITEDEPTH;
	int TCSIZE0;
	int TCSIZE1;
	int TCSIZE2;
	int TCSIZE3;
	int TCSIZE4;
	int TCSIZE5;
	int TCSIZE6;
	int TCSIZE7;
	int POINTSAMPLE_BASETEXTURE;
	int POINTSAMPLE_TEXTURE1;
	int POINTSAMPLE_TEXTURE2;
	int POINTSAMPLE_TEXTURE3;
	int POINTSAMPLE_TEXTURE4;
	int ALPHA_BLEND_COLOR_OVERLAY;
	int CULL;
	int DEPTHTEST;
	int COPYALPHA;
	int ALPHA_BLEND;
	int USES_VIEWPROJ;
};


void InitParmsScreenspace(const defParms_screenspace& info, CBaseVSShader* pShader, IMaterialVar** params);
void InitPassScreenspace(const defParms_screenspace& info, CBaseVSShader* pShader, IMaterialVar** params);
void DrawPassScreenspace(const defParms_screenspace& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext);


#endif