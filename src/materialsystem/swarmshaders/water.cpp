//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "common_hlsl_cpp_consts.h" // hack hack hack!
#include "convar.h"


#include "deferred_includes.h"

ConVar r_buildingmapforworld( "r_buildingmapforworld", "0" );


#include "include/Water_vs20.inc"
#include "include/water_ps30.inc"
#include "include/water_deferred_ps30.inc";

#include "shaderlib/commandbuilder.h"
#include "deferred_water.h"
// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


	void InitParamsWater_DX9(CBaseVSShader* pShader, IMaterialVar** params, const char* pMaterialName, defParms_Water& info)
	{
		if( !params[info.ABOVEWATER]->IsDefined() )
		{
			Warning( "***need to set $abovewater for material %s\n", pMaterialName );
			params[info.ABOVEWATER]->SetIntValue( 1 );
		}

		//params[info.ABOVEWATER]->SetIntValue( 1 );

		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		if( !params[info.CHEAPWATERSTARTDISTANCE]->IsDefined() )
		{
			params[info.CHEAPWATERSTARTDISTANCE]->SetFloatValue( 500.0f );
		}
		if( !params[info.CHEAPWATERENDDISTANCE]->IsDefined() )
		{
			params[info.CHEAPWATERENDDISTANCE]->SetFloatValue( 1000.0f );
		}
		if( !params[info.SCROLL1]->IsDefined() )
		{
			params[info.SCROLL1]->SetVecValue( 0.0f, 0.0f, 0.0f );
		}

		if (!params[info.TRANSLUCENT]->IsDefined())
		{
			params[info.TRANSLUCENT]->SetIntValue(1);
		}

		//MATERIAL_VAR_TRANSLUCENT;

		/*if (!params[info.BASETEXTURE]->IsDefined())
		{
			params[info.BASETEXTURE]->SetStringValue("nature/water_lake_color");
		}*/
		if( !params[info.SCROLL2]->IsDefined() )
		{
			params[info.SCROLL2]->SetVecValue( 0.0f, 0.0f, 0.0f );
		}
		if( !params[info.FOGCOLOR]->IsDefined() )
		{
			params[info.FOGCOLOR]->SetVecValue( 1.0f, 0.0f, 0.0f );
			Warning( "material %s needs to have a $fogcolor.\n", pMaterialName );
		}
		if( !params[info.REFLECTENTITIES]->IsDefined() )
		{
			params[info.REFLECTENTITIES]->SetIntValue( 0 );
		}
		if( !params[info.WATERBLENDFACTOR]->IsDefined() )
		{
			params[info.WATERBLENDFACTOR]->SetFloatValue( 1.0f );
		}

		if ( IsPS3() && !params[info.SCENEDEPTH]->IsDefined() )
		{
			params[info.SCENEDEPTH]->SetStringValue( "^PS3^DEPTHBUFFER" );
		}
		
		/*if (!params[info.MRAO]->IsDefined())
			params[info.MRAO]->SetStringValue("dev/pbr/perfect_gloss");*/

		// If there's no envmap or reflection texture, make sure to set the reflection tint to 0 so we don't reflect garbage
		// (The better change would be to add static combos to support no environment map but this is a lower impact change at this point)
		if ( !params[info.ENVMAP ]->IsDefined() && !params[info.REFLECTTEXTURE ]->IsDefined() )
		{
			params[info.REFLECTTINT ]->SetVecValue( 0.0f, 0.0f, 0.0f, 0.0f );
		}

		InitFloatParam(info.FLOW_WORLDUVSCALE, params, 1.0f );
		InitFloatParam(info.FLOW_NORMALUVSCALE, params, 1.0f );
		InitFloatParam(info.FLOW_TIMEINTERVALINSECONDS, params, 0.4f );
		InitFloatParam(info.FLOW_UVSCROLLDISTANCE, params, 0.2f );
		InitFloatParam(info.FLOW_BUMPSTRENGTH, params, 1.0f );
		InitFloatParam(info.FLOW_NOISE_SCALE, params, 0.0002f );

		InitFloatParam(info.COLOR_FLOW_UVSCALE, params, 1.0f );
		InitFloatParam(info.COLOR_FLOW_TIMEINTERVALINSECONDS, params, 0.4f );
		InitFloatParam(info.COLOR_FLOW_UVSCROLLDISTANCE, params, 0.2f );
		InitFloatParam(info.COLOR_FLOW_LERPEXP, params, 1.0f );
		InitFloatParam(info.COLOR_FLOW_DISPLACEBYNORMALSTRENGTH, params, 0.0025f );

		InitIntParam(info.FORCEENVMAP, params, 0 );

		InitIntParam(info.FORCECHEAP, params, 0 );
		InitFloatParam(info.FLASHLIGHTTINT, params, 1.0f );
		InitIntParam(info.LIGHTMAPWATERFOG, params, 0 );
		InitFloatParam(info.FORCEFRESNEL, params, -1.0f );

		// Fallbacks for water need lightmaps usually
		//if ( params[info.BASETEXTURE]->IsDefined() || ( params[info.LIGHTMAPWATERFOG]->GetIntValue() != 0 ) )
		//{
		//	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		//}

		//SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		//// Don't need bumped lightmaps unless we have a basetexture.  We only use them otherwise for lighting the water fog, which only needs one sample.
		//if( params[info.BASETEXTURE]->IsDefined() && g_pConfig->UseBumpmapping() && params[info.NORMALMAP]->IsDefined() )
		//{
		//	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
		//}

		if ( !params[info.DEPTH_FEATHER]->IsDefined() )
		{
			params[info.DEPTH_FEATHER]->SetIntValue( 0 );
		}

		
	}

	void InitWater_DX9(CBaseVSShader* pShader, IMaterialVar** params, defParms_Water& info)
	{
		
		Assert( params[info.WATERDEPTH]->IsDefined() );

		if( params[info.REFRACTTEXTURE]->IsDefined() )
		{
			pShader->LoadTexture(info.REFRACTTEXTURE );
		}
		if( params[info.SCENEDEPTH]->IsDefined() )
		{
			pShader->LoadTexture(info.SCENEDEPTH );
		}
		if( params[info.REFLECTTEXTURE]->IsDefined() )
		{
			pShader->LoadTexture(info.REFLECTTEXTURE );
		}
		if ( params[info.ENVMAP]->IsDefined() )
		{
			pShader->LoadCubeMap(info.ENVMAP );
		}
		if ( params[info.NORMALMAP]->IsDefined() )
		{
			pShader->LoadBumpMap(info.NORMALMAP );
		}
		if( params[info.BASETEXTURE]->IsDefined() )
		{
			pShader->LoadTexture(info.BASETEXTURE );
		}
		if ( params[info.FLOWMAP]->IsDefined() )
		{
			pShader->LoadTexture(info.FLOWMAP );
		}
		if ( params[info.FLOW_NOISE_TEXTURE]->IsDefined() )
		{
			pShader->LoadTexture(info.FLOW_NOISE_TEXTURE );
		}
		if ( params[info.SIMPLEOVERLAY]->IsDefined() )
		{
			pShader->LoadTexture(info.SIMPLEOVERLAY );
		}
		/*if (params[info.MRAO]->IsDefined())
		{
			pShader->LoadTexture(info.MRAO);
		}*/
	}
	
	

	/*inline void GetVecParam( int constantVar, float *val )
	{
		if( constantVar == -1 )
			return;

		IMaterialVar* pVar = s_ppParams[constantVar];
		Assert( pVar );

		if (pVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pVar->GetVecValue( val, 4 );
		else
			val[0] = val[1] = val[2] = val[3] = pVar->GetFloatValue();
	}*/

	void DrawWater_DX9(defParms_Water& info, CBaseVSShader* pShader, IMaterialVar** params, IShaderShadow* pShaderShadow,
    IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext)
	{
		Vector4D Scroll1;
		params[info.SCROLL1]->GetVecValue( Scroll1.Base(), 4 );

		bool bReflection = params[info.REFLECTTEXTURE]->IsTexture();
		bool bRefraction = params[info.REFRACTTEXTURE]->IsTexture();
		bool bHasFlowmap = params[info.FLOWMAP]->IsTexture();
		bool bHasBaseTexture = params[info.BASETEXTURE]->IsTexture();
		bool bHasMultiTexture = fabs( Scroll1.x ) > 0.0f;
		bool hasFlashlight = !bHasMultiTexture && pShader->UsingFlashlight( params );
		bool bLightmapWaterFog = ( params[info.LIGHTMAPWATERFOG]->GetIntValue() != 0 );
		bool bHasSimpleOverlay = params[info.SIMPLEOVERLAY]->IsTexture();
		bool bForceFresnel = ( params[info.FORCEFRESNEL]->GetFloatValue() != -1.0f );

		bool bIsTranslucent = params[info.TRANSLUCENT]->GetIntValue() != 0;
		//bool bHasFlowAlpha = params[info.BASETEXTURE]->IsDefined() && params[info.FLOWMAP]->IsDefined();
		//bool bBuildingForWorld = r_buildingmapforworld.GetBool() ? 1 : 0;
		//bool bHasMRAO = IsTextureSet(info.MRAO, params);
		/*if (pDeferredContext) {
			pDeferredContext->bWaterHasMRAO = (info.MRAO != -1 && params[info.MRAO]->IsDefined());
		}*/
		

		if ( bHasFlowmap )
		{
			bHasMultiTexture = false;
		}

		//if (bIsTranslucent || bHasFlowAlpha)
		//{
		//	SET_FLAGS(MATERIAL_VAR_TRANSLUCENT);
		//	SET_FLAGS(MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY); // Helps with sorting
		//}

		// LIGHTMAP - needed either with basetexture or lightmapwaterfog.  Not sure where the bReflection restriction comes in.
		//bool bUsingLightmap = bLightmapWaterFog || ( bReflection && bHasBaseTexture );

		SHADOW_STATE
		{
			if ( bRefraction )
			{
				// refract sampler
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, !IsX360() );
			}

			if ( bReflection )
			{
				// reflect sampler
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, !IsX360() );
			}  
			else
			{
				// envmap sampler
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
			}

			if ( bHasBaseTexture )
			{
				// BASETEXTURE
				pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER10, true );
			}

			pShaderShadow->EnableTexture(SHADER_SAMPLER12, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER12, true);

			pShaderShadow->EnableTexture(SHADER_SAMPLER13, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER13, false);

			pShaderShadow->EnableTexture(SHADER_SAMPLER15, true);
			//pShaderShadow->EnableSRGBRead(SHADER_SAMPLER15, true);

			// normal map
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );

			/*if ( bUsingLightmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, false );
			}*/

			// flowmap
			if ( bHasFlowmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, false );

				pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, false );
			}

			if( hasFlashlight  )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );

				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );
				//pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER7 );

				pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );
			}
		

			if ( bHasSimpleOverlay )
			{
				// SIMPLEOVERLAY
				pShaderShadow->EnableTexture( SHADER_SAMPLER11, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER11, true );
			}

			if (bIsTranslucent)
			{
				pShader->EnableAlphaBlending(SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
			}

			pShaderShadow->EnableBlending(true);
			pShaderShadow->BlendFunc(SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
			pShaderShadow->EnableDepthWrites(!bIsTranslucent);

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;

			// texcoord0 : base texcoord
			// texcoord1 : lightmap texcoord
			// texcoord2 : lightmap texcoord offset
			int numTexCoords = 1;
			// You need lightmap data if you are using lightmapwaterfog or you have a basetexture.
			if ( bLightmapWaterFog || bHasBaseTexture )
			{
				numTexCoords = 3;
			}
			pShaderShadow->VertexShaderVertexFormat( fmt, numTexCoords, 0, 0 );
			
			DECLARE_STATIC_VERTEX_SHADER( water_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( MULTITEXTURE, bHasMultiTexture );
			SET_STATIC_VERTEX_SHADER_COMBO( BASETEXTURE, bHasBaseTexture );
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, hasFlashlight );
			SET_STATIC_VERTEX_SHADER_COMBO( LIGHTMAPWATERFOG, bLightmapWaterFog );
			SET_STATIC_VERTEX_SHADER_COMBO( FLOWMAP, bHasFlowmap );
			SET_STATIC_VERTEX_SHADER( water_vs20 );

			// "REFLECT" "0..1"
			// "REFRACT" "0..1"

			/*DECLARE_STATIC_PIXEL_SHADER(water_ps30);
			SET_STATIC_PIXEL_SHADER_COMBO( REFLECT,  bReflection );
			SET_STATIC_PIXEL_SHADER_COMBO( REFRACT,  bRefraction );
			SET_STATIC_PIXEL_SHADER_COMBO( ABOVEWATER,  params[info.ABOVEWATER]->GetIntValue() );
			SET_STATIC_PIXEL_SHADER_COMBO( MULTITEXTURE, bHasMultiTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE, bHasBaseTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( FLOWMAP, bHasFlowmap );
			SET_STATIC_PIXEL_SHADER_COMBO( FLOW_DEBUG, clamp( params[info.FLOW_DEBUG ]->GetIntValue(), 0, 2 ) );
			SET_STATIC_PIXEL_SHADER_COMBO( FORCEFRESNEL, bForceFresnel );
			SET_STATIC_PIXEL_SHADER_COMBO( SIMPLEOVERLAY, bHasSimpleOverlay );\
			SET_STATIC_PIXEL_SHADER_COMBO(LIGHTMAPWATERFOG, bLightmapWaterFog);
			SET_STATIC_PIXEL_SHADER(water_ps30);*/

			DECLARE_STATIC_PIXEL_SHADER(water_deferred_ps30);
			SET_STATIC_PIXEL_SHADER_COMBO(REFLECT, bReflection);
			SET_STATIC_PIXEL_SHADER_COMBO(REFRACT, bRefraction);
			SET_STATIC_PIXEL_SHADER_COMBO(ABOVEWATER, params[info.ABOVEWATER]->GetIntValue());
			SET_STATIC_PIXEL_SHADER_COMBO(MULTITEXTURE, bHasMultiTexture);
			SET_STATIC_PIXEL_SHADER_COMBO(BASETEXTURE, bHasBaseTexture);
			SET_STATIC_PIXEL_SHADER_COMBO(FLOWMAP, bHasFlowmap);
			SET_STATIC_PIXEL_SHADER_COMBO(FLOW_DEBUG, clamp(params[info.FLOW_DEBUG]->GetIntValue(), 0, 2));
			SET_STATIC_PIXEL_SHADER_COMBO(FORCEFRESNEL, bForceFresnel);
			SET_STATIC_PIXEL_SHADER_COMBO(SIMPLEOVERLAY, bHasSimpleOverlay); \
			SET_STATIC_PIXEL_SHADER_COMBO(LIGHTMAPWATERFOG, bLightmapWaterFog);
			SET_STATIC_PIXEL_SHADER(water_deferred_ps30);

			pShader->FogToFogColor();

			// we are writing linear values from this shader.
			pShaderShadow->EnableSRGBWrite( true );

			pShaderShadow->EnableAlphaWrites( true );
		}
		DYNAMIC_STATE
		{
			//bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();
			pShaderAPI->SetDefaultState();
			if ( bRefraction )
			{
				// HDRFIXME: add comment about binding.. Specify the number of MRTs in the enable
				pShader->BindTexture( SHADER_SAMPLER0, info.REFRACTTEXTURE, -1 );
			}

			if ( bReflection )
			{
				pShader->BindTexture( SHADER_SAMPLER1, info.REFLECTTEXTURE, -1 );
			}
			else if ( params[info.ENVMAP ]->IsDefined() )
			{
				pShader->BindTexture( SHADER_SAMPLER1, info.ENVMAP );
			}

			if (params[info.NORMALMAP]->IsDefined())
			{
				pShader->BindTexture(SHADER_SAMPLER2, info.NORMALMAP, info.BUMPFRAME);
			}

			/*if ( bUsingLightmap )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_LIGHTMAP );
			}*/

			if( bHasBaseTexture )
			{
				pShader->BindTexture( SHADER_SAMPLER10, info.BASETEXTURE, FRAME );
			}

			if (params[info.MRAO]->IsDefined())
			{
				pShader->BindTexture(SHADER_SAMPLER13, GetDeferredExt()->GetTexture_LightCtrl());
			}
			
			

			if ( bHasFlowmap )
			{
				pShader->BindTexture( SHADER_SAMPLER4, info.FLOWMAP, info.FLOWMAPFRAME );
				pShader->BindTexture( SHADER_SAMPLER5, info.FLOW_NOISE_TEXTURE );

				float vFlowConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				vFlowConst1[0] = 1.0f / params[info.FLOW_WORLDUVSCALE ]->GetFloatValue();
				vFlowConst1[1] = 1.0f / params[info.FLOW_NORMALUVSCALE ]->GetFloatValue();
				vFlowConst1[2] = params[info.FLOW_BUMPSTRENGTH ]->GetFloatValue();
				vFlowConst1[3] = params[info.COLOR_FLOW_DISPLACEBYNORMALSTRENGTH ]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 13, vFlowConst1, 1 );

				float vFlowConst2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				vFlowConst2[0] = params[info.FLOW_TIMEINTERVALINSECONDS ]->GetFloatValue();
				vFlowConst2[1] = params[info.FLOW_UVSCROLLDISTANCE ]->GetFloatValue();
				vFlowConst2[2] = params[info.FLOW_NOISE_SCALE ]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 14, vFlowConst2, 1 );

				float vColorFlowConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				vColorFlowConst1[0] = 1.0f / params[info.COLOR_FLOW_UVSCALE ]->GetFloatValue();
				vColorFlowConst1[1] = params[info.COLOR_FLOW_TIMEINTERVALINSECONDS ]->GetFloatValue();
				vColorFlowConst1[2] = params[info.COLOR_FLOW_UVSCROLLDISTANCE ]->GetFloatValue();
				vColorFlowConst1[3] = params[info.COLOR_FLOW_LERPEXP ]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 26, vColorFlowConst1, 1 );
			}
			
			if (bHasSimpleOverlay)
			{
				pShader->BindTexture(SHADER_SAMPLER11, info.SIMPLEOVERLAY);
			}
			
			// Time
			float vTimeConst[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			float flTime = pShaderAPI->CurrentTime();
			vTimeConst[0] = flTime;
			//vTimeConst[0] -= ( float )( ( int )( vTimeConst[0] / 1000.0f ) ) * 1000.0f;
			pShaderAPI->SetPixelShaderConstant( 8, vTimeConst, 1 );

			// These constants are used to rotate the world space water normals around the up axis to align the
			// normal with the camera and then give us a 2D offset vector to use for reflection and refraction uv's
			VMatrix mView;
			pShaderAPI->GetMatrix( MATERIAL_VIEW, mView.m[0] );
			mView = mView.Transpose3x3();

			Vector4D vCameraRight( mView.m[0][0], mView.m[0][1], mView.m[0][2], 0.0f );
			vCameraRight.z = 0.0f; // Project onto the plane of water
			vCameraRight.AsVector3D().NormalizeInPlace();

			Vector4D vCameraForward;
			CrossProduct( Vector( 0.0f, 0.0f, 1.0f ), vCameraRight.AsVector3D(), vCameraForward.AsVector3D() ); // I assume the water surface normal is pointing along z!

			pShaderAPI->SetPixelShaderConstant( 22, vCameraRight.Base() );
			pShaderAPI->SetPixelShaderConstant( 23, vCameraForward.Base() );

			pShader->SetPixelShaderConstant( 25, info.FORCEFRESNEL );

			// Refraction tint
			if( bRefraction )
			{
				pShader->SetPixelShaderConstantGammaToLinear( 1, info.REFRACTTINT );
			}

			// Reflection tint
			if ( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
			{
				// Need to multiply by 4 in linear space since we premultiplied into
				// the render target by .25 to get overbright data in the reflection render target.
				float gammaReflectTint[3];
				params[info.REFLECTTINT]->GetVecValue( gammaReflectTint, 3 );
				float linearReflectTint[4];
				linearReflectTint[0] = GammaToLinear( gammaReflectTint[0] ) * 4.0f;
				linearReflectTint[1] = GammaToLinear( gammaReflectTint[1] ) * 4.0f;
				linearReflectTint[2] = GammaToLinear( gammaReflectTint[2] ) * 4.0f;
				linearReflectTint[3] = params[info.WATERBLENDFACTOR]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 4, linearReflectTint, 1 );
			}
			else
			{
				pShader->SetPixelShaderConstantGammaToLinear( 4, info.REFLECTTINT, info.WATERBLENDFACTOR );
			}

			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, info.BUMPTRANSFORM );
			
			float curtime=pShaderAPI->CurrentTime();
			float vc0[4];
			float v0[4];
			params[info.SCROLL1]->GetVecValue(v0,4);
			vc0[0]=curtime*v0[0];
			vc0[1]=curtime*v0[1];
			params[info.SCROLL2]->GetVecValue(v0,4);
			vc0[2]=curtime*v0[0];
			vc0[3]=curtime*v0[1];
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vc0, 1 );

			float c0[4] = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, c0, 1 );
			
			float c2[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
			pShaderAPI->SetPixelShaderConstant( 2, c2, 1 );
			
			// fresnel constants
			float c3[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 3, c3, 1 );

			float c5[4] = { params[info.REFLECTAMOUNT]->GetFloatValue(), params[info.REFLECTAMOUNT]->GetFloatValue(),
				params[info.REFRACTAMOUNT]->GetFloatValue(), params[info.REFRACTAMOUNT]->GetFloatValue() };
			pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

#if 0
			SetPixelShaderConstantGammaToLinear( 6, FOGCOLOR );
#else
			// Need to use the srgb curve since that we do in UpdatePixelFogColorConstant so that we match the older version of water where we render to an offscreen buffer and fog on the way in.
			float fogColorConstant[4];

			params[info.FOGCOLOR]->GetVecValue( fogColorConstant, 3 );
			fogColorConstant[3] = 0.0f;

			fogColorConstant[0] = SrgbGammaToLinear( fogColorConstant[0] );
			fogColorConstant[1] = SrgbGammaToLinear( fogColorConstant[1] );
			fogColorConstant[2] = SrgbGammaToLinear( fogColorConstant[2] );
			pShaderAPI->SetPixelShaderConstant( 6, fogColorConstant, 1 );
#endif

			float c7[4] = 
			{ 
				params[info.FOGSTART]->GetFloatValue(),
				params[info.FOGEND]->GetFloatValue() - params[info.FOGSTART]->GetFloatValue(),
				1.0f, 
				0.0f 
			};
			if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
			{
				// water overbright factor
				c7[2] = 4.0;
			}
			pShaderAPI->SetPixelShaderConstant( 7, c7, 1 );

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

			if( bHasFlowmap )
			{
				pShader->SetPixelShaderConstant( 9, info.FLOWMAPSCROLLRATE );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( water_vs20 );

			SET_DYNAMIC_VERTEX_SHADER( water_vs20 );

#ifdef _PS3
			CCommandBufferBuilder< CDynamicCommandStorageBuffer > DynamicCmdsOut;
#else
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;
#endif

			bool bFlashlightShadows = false;
			bool bUberlight = false;
			if( hasFlashlight )
			{
#ifdef _PS3
				CCommandBufferBuilder< CFixedCommandStorageBuffer< 256 > > flashlightECB;
#endif

				pShaderAPI->GetFlashlightShaderInfo( &bFlashlightShadows, &bUberlight );
#ifdef _PS3
				{
					flashlightECB.SetVertexShaderFlashlightState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4 );
				}
#endif
				/*if( IsX360())
				{
					DynamicCmdsOut.SetVertexShaderFlashlightState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4 );
				}*/

				//CBCmdSetPixelShaderFlashlightState_t state;
				//state.m_LightSampler = SHADER_SAMPLER6; // FIXME . . don't want this here.
				//state.m_DepthSampler = SHADER_SAMPLER7;
				//state.m_ShadowNoiseSampler = SHADER_SAMPLER8;
				//state.m_nColorConstant = PSREG_FLASHLIGHT_COLOR;
				//state.m_nAttenConstant = 15;
				//state.m_nOriginConstant = 16;
				//state.m_nDepthTweakConstant = 21;
				//state.m_nScreenScaleConstant = PSREG_FLASHLIGHT_SCREEN_SCALE;
				//state.m_nWorldToTextureConstant = -1;
				//state.m_bFlashlightNoLambert = false;
				//state.m_bSinglePassFlashlight = true;

#ifdef _PS3
				{
					flashlightECB.SetPixelShaderFlashlightState( state );
					flashlightECB.End();

					ShaderApiFast( pShaderAPI )->ExecuteCommandBufferPPU( flashlightECB.Base() );
				}
#else
				/*{
					DynamicCmdsOut.SetPixelShaderFlashlightState( state );
				}*/
#endif
				DynamicCmdsOut.SetPixelShaderConstant( 10, info.FLASHLIGHTTINT );
			}

			// Get viewport and render target dimensions and set shader constant to do a 2D mad
			int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
			pShaderAPI->GetCurrentViewport( nViewportX, nViewportY, nViewportWidth, nViewportHeight );

			int nRtWidth, nRtHeight;
			pShaderAPI->GetCurrentRenderTargetDimensions( nRtWidth, nRtHeight );

			
			pShader->BindTexture(SHADER_SAMPLER12, GetDeferredExt()->GetTexture_LightAccum(), 0);
				//BindTexture(SHADER_SAMPLER15, GetDeferredExt()->GetTexture_LightAccum2(), 0);
				int x, y, w, t;
				pShaderAPI->GetCurrentViewport(x, y, w, t);
				float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

				pShaderAPI->SetPixelShaderConstant(27, fl1);
			

			float vViewportMad[4];

			// viewport->screen transform
			vViewportMad[0] = ( float )nViewportWidth / ( float )nRtWidth;
			vViewportMad[1] = ( float )nViewportHeight / ( float )nRtHeight;
			vViewportMad[2] = ( float )nViewportX / ( float )nRtWidth;
			vViewportMad[3] = ( float )nViewportY / ( float )nRtHeight;
			DynamicCmdsOut.SetPixelShaderConstant( 24, vViewportMad, 1 );

			/*DECLARE_DYNAMIC_PIXEL_SHADER(water_ps30);
			SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
			SET_DYNAMIC_PIXEL_SHADER(water_ps30);*/

			DECLARE_DYNAMIC_PIXEL_SHADER(water_deferred_ps30);
			//(BUILDWORLDIMPOSTER, bBuildingForWorld);
			//SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
			SET_DYNAMIC_PIXEL_SHADER(water_deferred_ps30);
			

			DynamicCmdsOut.End();
			pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
		}
		pShader->Draw();
	}


