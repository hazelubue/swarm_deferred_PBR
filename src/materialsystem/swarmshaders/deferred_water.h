//====== Copyright (c) 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef DEFERRED_WATER_H
#define DEFERRED_WATER_H
#ifdef _WIN32
#pragma once
#endif

#include <string.h>
#include "BaseVSShader.h"
#include "convar.h"

#include "shaderlib/commandbuilder.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;
class CDeferredPerMaterialContextData;


//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct defParms_Water
{
	defParms_Water() { memset(this, 0xFF, sizeof(*this)); 
	//_prev_MRAO = -2;
	}

	//bool bHasMRAOTexture = false;

	//int _prev_MRAO;

	//bool HasChanged() const
	//{
	//	// Check if texture is now defined when it wasn't before, or vice versa
	//	return (params[MRAO]->IsDefined() != _prev_MRAO_defined);
	//}
	//void UpdatePrevious()
	//{
	//	_prev_MRAO_defined = params[MRAO]->IsDefined();
	//}

	int BASETEXTURE;
	int REFRACTTEXTURE;
	int SCENEDEPTH;
	int REFLECTTEXTURE;
	int REFRACTAMOUNT;
	int REFRACTTINT;
	int REFLECTAMOUNT;
	int REFLECTTINT;
	int NORMALMAP;
	int BUMPFRAME;
	int BUMPTRANSFORM;
	int TIME;
	int WATERDEPTH;
	int CHEAPWATERSTARTDISTANCE;
	int CHEAPWATERENDDISTANCE;
	int ENVMAP;
	int ENVMAPFRAME;
	int FOGCOLOR;
	int FORCECHEAP;
	int REFLECTENTITIES;
	int FOGSTART;
	int FOGEND;
	int ABOVEWATER;
	int WATERBLENDFACTOR;
	int NOFRESNEL;
	int NOLOWENDLIGHTMAP;
	int SCROLL1;

	int TRANSLUCENT;

	int SCROLL2;
	int FLASHLIGHTTINT;
	int LIGHTMAPWATERFOG;
	int FORCEFRESNEL;
	int FORCEENVMAP;
	int DEPTH_FEATHER;
	int FLOWMAP;
	int FLOWMAPFRAME;
	int FLOWMAPSCROLLRATE;
	int FLOW_NOISE_TEXTURE;
	int FLOW_WORLDUVSCALE;
	int FLOW_NORMALUVSCALE;
	int FLOW_TIMEINTERVALINSECONDS;
	int FLOW_UVSCROLLDISTANCE;
	int FLOW_BUMPSTRENGTH;
	int FLOW_NOISE_SCALE;
	int FLOW_DEBUG;
	int COLOR_FLOW_UVSCALE;
	int COLOR_FLOW_TIMEINTERVALINSECONDS;
	int COLOR_FLOW_UVSCROLLDISTANCE;
	int COLOR_FLOW_LERPEXP;
	int COLOR_FLOW_DISPLACEBYNORMALSTRENGTH;
	int SIMPLEOVERLAY;
	int ALPHATESTREFERENCE;
	int PHONG_EXP;
	int PHONG_EXP2;
	int BRDF;
	int NOISE;
	int ROUGHNESS;
	int METALLIC;
	int AO;
	int EMISSIVE;
	int USESMOOTHNESS;
	int MRAO;



};


void InitParamsWater_DX9(CBaseVSShader* pShader, IMaterialVar** params, const char* pMaterialName,
	defParms_Water& info);
void InitWater_DX9(CBaseVSShader* pShader, IMaterialVar** params, defParms_Water& info);
void DrawWater_DX9(defParms_Water& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext);

#endif // REFRACT_DX9_HELPER_H
