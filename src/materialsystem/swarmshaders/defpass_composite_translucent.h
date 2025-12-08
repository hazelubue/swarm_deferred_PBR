#ifndef DEFPASS_COMPOSITE_TRANSLUCENT_H
#define DEFPASS_COMPOSITE_TRANSLUCENT_H

class CDeferredPerMaterialContextData;

struct defParms_composite_translucent
{
	defParms_composite_translucent()
	{
		Q_memset(this, 0xFF, sizeof(defParms_composite_translucent));

		bModel = false;
	};

	int BUMPMAP;
	int MRAOTEXTURE;

	int SelfShadowedBumpFlag;

	// textures
	int iAlbedo;
	int iAlbedo2;
	int iAlbedo3;
	int iAlbedo4;
	int iEnvmap;
	int iEnvmapMask;
	int iEnvmapMask2;
	int iBlendmodulate;
	int iBlendmodulate2;
	int iBlendmodulate3;

	// envmapping
	int iEnvmapTint;
	int iEnvmapSaturation;
	int iEnvmapContrast;
	int iEnvmapFresnel;

	// rimlight
	int iRimlightEnable;
	int iRimlightExponent;
	int iRimlightAlbedoScale;
	int iRimlightTint;
	int iRimlightModLight;

	// alpha
	int iAlphatestRef;

	// phong
	int iPhongScale;
	int iPhongFresnel;

	// self illum
	int iSelfIllumTint;
	int iSelfIllumMaskInEnvmapAlpha;
	int iSelfIllumFresnelModulate;
	int iSelfIllumMask;

	// blendmod
	int iBlendmodulateTransform;
	int iBlendmodulateTransform2;
	int iBlendmodulateTransform3;
	int iMultiblend;

	int iFresnelRanges;
	int iFramebuffer;


	int iRefraction;
	// config
	bool bForward;
	bool bModel;
};


void InitParmsComposite_translucent(const defParms_composite_translucent& info, CBaseVSShader* pShader, IMaterialVar** params);
void InitPassComposite_translucent(const defParms_composite_translucent& info, CBaseVSShader* pShader, IMaterialVar** params);
void DrawPassComposite_translucent(const defParms_composite_translucent& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext);


#endif