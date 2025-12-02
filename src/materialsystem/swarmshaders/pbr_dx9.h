// ---------------------- per-material context for PBR shader ----------------------
#ifndef DEFPASS_PBR_H
#define DEFPASS_PBR_H

const int PARALLAX_QUALITY_MAX = 3;

static ConVar mat_pbr_parallaxdepth("mat_pbr_parallaxdepth", ".1"); // 0.04
static ConVar mat_pbr_parallaxCenter("mat_pbr_parallaxCenter", ".9");
static ConVar mat_pbr_parallaxmap_quality("mat_pbr_parallaxmap_quality", "100", FCVAR_NONE, "", true, 0, true, PARALLAX_QUALITY_MAX);
static ConVar mat_pbr_parallaxmap("mat_pbr_parallaxmap", "1");
static ConVar mat_pbr_force_20b("mat_pbr_force_20b", "0", FCVAR_CHEAT);
static ConVar mat_pbr_iblIntensity("mat_pbr_iblIntensity", "1000.0", FCVAR_CHEAT);

class CDeferredPerMaterialContextData;

struct PBRParms_composition
{
	PBRParms_composition()
	{
		Q_memset(this, 0xFF, sizeof(PBRParms_composition));

		bModel = false;
	};

	int BASETEXTURE;
	int BASETEXTURE2;

	int NORMALTEXTURE;
	int BUMPMAP;
	int BUMPMAP2;


	int MRAOTEXTURE;
	int USESMRAO;
	int MRAOTEXTURE2;

	int HEIGHTMAP;

	int ENVMAP;

	int BRDF_INTEGRATION;

	int FRAME2;
	int BUMPFRAME;
	int ENVMAPFRAME;
	int BUMPFRAME2;

	int MRAOFRAME;
	int MRAOFRAME2;
	int EMISSIONFRAME;
	int EMISSIONFRAME2;

	int WRITE_DEPTH_TO_ALPHA;

	int TINTCOLOR;

	int PARALLAX;
	int PARALLAXSCALE;
	int PARALLAXDITHER;
	int PARALLAXCENTER;

	int MRAOSCALE;
	int MRAOSCALE2;
	int EMISSIONFRESNEL;
	int EMISSIONSCALE;
	int EMISSIONSCALE2;
	int HSV;
	int BLENDTINTBYMRAOALPHA;
	int FLASHLIGHTNOLAMBERT;
	int TREESWAY;

	int EMISSIONTEXTURE;
	int EMISSIONTEXTURE2;

	int HSV_BLEND;

	int ENVMAPPARALLAX;
	int ENVMAPORIGIN;
	int ENVMAPPARALLAXOBB1;
	int ENVMAPPARALLAXOBB2;
	int ENVMAPPARALLAXOBB3;

	int TREESWAYSPEEDHIGHWINDMULTIPLIER;
	int TREESWAYSCRUMBLEFALLOFFEXP;
	int TREESWAYFALLOFFEXP;
	int TREESWAYSCRUMBLESPEED;

	int TREESWAYSPEEDLERPSTART;
	int TREESWAYSPEEDLERPEND;

	int TREESWAYHEIGHT;
	int TREESWAYSTARTHEIGHT;
	int TREESWAYRADIUS;
	int TREESWAYSTARTRADIUS;

	int TREESWAYSPEED;
	int TREESWAYSTRENGTH;
	int TREESWAYSCRUMBLEFREQUENCY;
	int TREESWAYSCRUMBLESTRENGTH;

	int ENVMAPTINT;
	int ENVMAPMASK;

	int FRAMEBUFFER;
	int DEPTHBUFFER;

	int ENVMAPSATURATION;
	int ENVMAPCONTRAST;

	int ALPHATESTREFERENCE;

	int PARALLAXDEPTH;

	int SelfShadowedBumpFlag;

	int TRANSPARENCY;

	int VERTEXCOLORMODULATE;

	int ALPHA;

	int VERTEXCOMPRESSION;

	int BASETEXTURETRANSFORM;

	int PAINTMRAOTEXTURE;

	int PAINTSPLATNORMAL;

	int SPECULARTEXTURE;
	int BASECOLOR;

	//REMOVE IF USING OLD SHADER
	int useEnvAmbient;

	int TRANCLUECENT;

	int SSR;

	int IRRADIANCEMAP;

	bool bModel;

	
};

struct PBR_Switches_t {
	bool bHasBaseTexture;
	bool bHasNormalTexture;
	bool bHasMraoTexture;
	bool bHasEmissionTexture;
	bool bHasEnvTexture;
	bool bIsAlphaTested;
	bool bHasFlashlight;
	bool bHasColor;
	bool bLightMapped;
	bool bUseEnvAmbient;
	bool bHasSpecularTexture;
	bool bHasParallaxCorrection;
	int bUseParallax;
	bool bFullyOpaque;
	int nShadowFilterMode;
	BlendType_t nBlendType;
};

void BindLightmapBumped();

void InitParmsPBR(const PBRParms_composition& info, CBaseVSShader* pShader, IMaterialVar** params);
void InitPassPBR(const PBRParms_composition& info, CBaseVSShader* pShader, IMaterialVar** params);
void DrawPassPBR(const PBRParms_composition& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext);

void InitParmsPBRGlass(const PBRParms_composition& info, CBaseVSShader* pShader, IMaterialVar** params);
void InitPassPBRGlass(const PBRParms_composition& info, CBaseVSShader* pShader, IMaterialVar** params);
void DrawPassPBRGlass(const PBRParms_composition& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext);

// the paint pbr pass
void DrawPbrPass_Paint(CBaseVSShader* pShader, IMaterialVar** params, IShaderDynamicAPI* pShaderAPI, IShaderShadow* pShaderShadow,
	PBRParms_composition& info, PBR_Switches_t const& switches, CDeferredPerMaterialContextData* pContextDataPtr);

#endif
