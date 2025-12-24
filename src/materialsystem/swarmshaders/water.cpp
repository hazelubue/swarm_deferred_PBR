//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "basevsshader.h"
#include "mathlib/vmatrix.h"
#include "common_hlsl_cpp_consts.h" // hack hack hack!
#include "convar.h"

#include "include/water_vs30.inc"
#include "include/water_deferred_ps30.inc"
#include "shaderlib/commandbuilder.h"
#include "deferred_water.h"
#include "deferred_includes.h"
// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"





void InitParamsWater_DX9(CBaseVSShader* pShader, IMaterialVar** params, const char* pMaterialName, defParms_Water& info)
	{
		if( !params[info.ABOVEWATER]->IsDefined() )
		{
			Warning( "***need to set $abovewater for material %s\n", pMaterialName );
			params[info.ABOVEWATER]->SetIntValue( 1 );
		}
		if (!params[info.LIGHTMAPWATERFOG]->IsDefined())
		{
			Warning("***need to set $LIGHTMAPWATERFOG for material %s\n", pMaterialName);
			params[info.LIGHTMAPWATERFOG]->SetIntValue(1);
		}

		/*if (!params[info.BASETEXTURE]->IsDefined())
		{
			params[info.BASETEXTURE]->SetStringValue("nature/water_lake_color");
		}*/
		
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
		if( !params[info.REFLECTBLENDFACTOR]->IsDefined() )
		{
			params[info.REFLECTBLENDFACTOR]->SetFloatValue( 1.0f );
		}

		SET_FLAGS(MATERIAL_VAR_TRANSLUCENT);

		InitFloatParam(info.FLOW_WORLDUVSCALE, params, 1.0f );
		InitFloatParam(info.FLOW_NORMALUVSCALE, params, 1.0f );
		InitFloatParam(info.FLOW_TIMEINTERVALINSECONDS, params, 0.4f );
		InitFloatParam(info.FLOW_UVSCROLLDISTANCE, params, 0.2f );
		InitFloatParam(info.FLOW_BUMPSTRENGTH, params, 1.0f );
		InitFloatParam(info.FLOW_TIMESCALE, params, 1.0f );
		InitFloatParam(info.FLOW_NOISE_SCALE, params, 0.0002f );

		InitFloatParam(info.COLOR_FLOW_UVSCALE, params, 1.0f );
		InitFloatParam(info.COLOR_FLOW_TIMESCALE, params, 1.0f );
		InitFloatParam(info.COLOR_FLOW_TIMEINTERVALINSECONDS, params, 0.4f );
		InitFloatParam(info.COLOR_FLOW_UVSCROLLDISTANCE, params, 0.2f );
		InitFloatParam(info.COLOR_FLOW_LERPEXP, params, 1.0f );

		InitIntParam(info.FORCECHEAP, params, 0 );
		InitFloatParam(info.FLASHLIGHTTINT, params, 1.0f );
		InitIntParam(info.LIGHTMAPWATERFOG, params, 0 );
		InitFloatParam(info.FORCEFRESNEL, params, -1.0f );

		// Fallbacks for water need lightmaps usually
		if ( params[info.BASETEXTURE]->IsDefined() || ( params[info.LIGHTMAPWATERFOG]->GetIntValue() != 0 ) )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		// Don't need bumped lightmaps unless we have a basetexture.  We only use them otherwise for lighting the water fog, which only needs one sample.
		if( params[info.BASETEXTURE]->IsDefined() && g_pConfig->UseBumpmapping() && params[info.NORMALMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
		}
	}

void InitWater_DX9(CBaseVSShader* pShader, IMaterialVar** params, defParms_Water& info)
	{
		Assert( params[info.WATERDEPTH]->IsDefined() );

		if( params[info.REFRACTTEXTURE]->IsDefined() )
		{
			pShader->LoadTexture(info.REFRACTTEXTURE );
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
		if (params[info.MRAO]->IsDefined())
		{
			pShader->LoadTexture(info.MRAO);
		}
	}


	void DrawWater_DX9(defParms_Water& info, CBaseVSShader* pShader, IMaterialVar** params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData* pDeferredContext)
	{
		Vector4D Scroll1;
		params[info.SCROLL1]->GetVecValue( Scroll1.Base(), 4 );

		bool bReflection = params[info.REFLECTTEXTURE]->IsTexture();
		bool bRefraction = params[info.REFRACTTEXTURE]->IsTexture();
		bool bHasFlowmap = params[info.FLOWMAP]->IsTexture();
		bool hasFlashlight = pShader->UsingFlashlight( params );
		bool bHasBaseTexture = params[info.BASETEXTURE]->IsTexture();
		bool bHasMultiTexture = fabs( Scroll1.x ) > 0.0f;
		bool bLightmapWaterFog = ( params[info.LIGHTMAPWATERFOG]->GetIntValue() != 0 );

		bool bForceFresnel = ( params[info.FORCEFRESNEL]->GetFloatValue() != -1.0f );

		//const bool bIsTranslucent = IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT);
		bool bForwardRendered = true;

		if ( bHasFlowmap )
		{
			bHasMultiTexture = false;
		}

		if ( bHasBaseTexture || bHasMultiTexture )
		{
			//hasFlashlight = false;
			//bLightmapWaterFog = false;
		}

		// LIGHTMAP - needed either with basetexture or lightmapwaterfog.  Not sure where the bReflection restriction comes in.
		bool bUsingLightmap = bLightmapWaterFog || ( bReflection && bHasBaseTexture );

		SHADOW_STATE
		{
		    pShaderShadow->EnableCulling(false);
			//pShaderShadow->EnableDepthWrites(true);
			pShader->SetInitialShadowState( );
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

			if ( bHasBaseTexture )
			{
				// BASETEXTURE
				pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER10, true );
			}

			// normal map
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );

			if ( bUsingLightmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, false );
			}

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
				pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER7 );

				pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );
			}

			pShaderShadow->EnableTexture(SHADER_SAMPLER11, true);

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
			
			DECLARE_STATIC_VERTEX_SHADER( water_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( MULTITEXTURE, bHasMultiTexture );
			SET_STATIC_VERTEX_SHADER_COMBO( BASETEXTURE, bHasBaseTexture );
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, hasFlashlight );
			SET_STATIC_VERTEX_SHADER_COMBO( LIGHTMAPWATERFOG, bLightmapWaterFog );
			SET_STATIC_VERTEX_SHADER_COMBO( FLOWMAP, bHasFlowmap );
			SET_STATIC_VERTEX_SHADER( water_vs30 );

			// "REFLECT" "0..1"
			// "REFRACT" "0..1"
			
				DECLARE_STATIC_PIXEL_SHADER(water_deferred_ps30);
				SET_STATIC_PIXEL_SHADER_COMBO( REFLECT,  bReflection );
				SET_STATIC_PIXEL_SHADER_COMBO( REFRACT,  bRefraction );
				SET_STATIC_PIXEL_SHADER_COMBO( ABOVEWATER,  params[info.ABOVEWATER]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( MULTITEXTURE, bHasMultiTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE, bHasBaseTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( FLOWMAP, bHasFlowmap );
				SET_STATIC_PIXEL_SHADER_COMBO( FLOW_DEBUG, clamp( params[info.FLOW_DEBUG ]->GetIntValue(), 0, 2 ) );
				SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHT, hasFlashlight);
				SET_STATIC_PIXEL_SHADER_COMBO(LIGHTMAPWATERFOG, bLightmapWaterFog);
				SET_STATIC_PIXEL_SHADER_COMBO( FORCEFRESNEL, bForceFresnel );
				SET_STATIC_PIXEL_SHADER( water_deferred_ps30 );
			

				pShader->FogToFogColor();

			// we are writing linear values from this shader.
			pShaderShadow->EnableSRGBWrite( true );

			pShaderShadow->EnableAlphaWrites( true );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			if( bRefraction )
			{
				// HDRFIXME: add comment about binding.. Specify the number of MRTs in the enable
				pShader->BindTexture( SHADER_SAMPLER0, info.REFRACTTEXTURE, -1 );
			}
			if( bReflection )
			{
				pShader->BindTexture( SHADER_SAMPLER1, info.REFLECTTEXTURE, -1 );
			}
			pShader->BindTexture( SHADER_SAMPLER2, info.NORMALMAP, info.BUMPFRAME );

			if ( bUsingLightmap )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_LIGHTMAP );
			}

			if( bHasBaseTexture )
			{
				pShader->BindTexture( SHADER_SAMPLER10, info.BASETEXTURE, FRAME );
			}

			if (params[info.MRAO]->IsDefined())
			{
				pShader->BindTexture(SHADER_SAMPLER11, info.MRAO);
			}

			if ( bHasFlowmap )
			{
				pShader->BindTexture( SHADER_SAMPLER4, info.FLOWMAP, info.FLOWMAPFRAME );
				pShader->BindTexture( SHADER_SAMPLER5, info.FLOW_NOISE_TEXTURE );

				float vFlowConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				vFlowConst1[0] = 1.0f / params[info.FLOW_WORLDUVSCALE ]->GetFloatValue();
				vFlowConst1[1] = 1.0f / params[info.FLOW_NORMALUVSCALE ]->GetFloatValue();
				vFlowConst1[2] = params[info.FLOW_BUMPSTRENGTH ]->GetFloatValue();
				vFlowConst1[3] = params[info.FLOW_TIMESCALE ]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 13, vFlowConst1, 1 );

				float vFlowConst2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				vFlowConst2[0] = params[info.FLOW_TIMEINTERVALINSECONDS ]->GetFloatValue();
				vFlowConst2[1] = params[info.FLOW_UVSCROLLDISTANCE ]->GetFloatValue();
				vFlowConst2[2] = params[info.FLOW_NOISE_SCALE ]->GetFloatValue();
				vFlowConst2[3] = params[info.COLOR_FLOW_LERPEXP ]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 14, vFlowConst2, 1 );

				float vColorFlowConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				vColorFlowConst1[0] = 1.0f / params[info.COLOR_FLOW_UVSCALE ]->GetFloatValue();
				vColorFlowConst1[1] = params[info.COLOR_FLOW_TIMESCALE ]->GetFloatValue();
				vColorFlowConst1[2] = params[info.COLOR_FLOW_TIMEINTERVALINSECONDS ]->GetFloatValue();
				vColorFlowConst1[3] = params[info.COLOR_FLOW_UVSCROLLDISTANCE ]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 26, vColorFlowConst1, 1 );
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
			if( bReflection )
			{
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
				{
					// Need to multiply by 4 in linear space since we premultiplied into
					// the render target by .25 to get overbright data in the reflection render target.
					float gammaReflectTint[3];
					params[info.REFLECTTINT]->GetVecValue( gammaReflectTint, 3 );
					float linearReflectTint[4];
					linearReflectTint[0] = GammaToLinear( gammaReflectTint[0] ) * 4.0f;
					linearReflectTint[1] = GammaToLinear( gammaReflectTint[1] ) * 4.0f;
					linearReflectTint[2] = GammaToLinear( gammaReflectTint[2] ) * 4.0f;
					linearReflectTint[3] = 1.0f;
					pShaderAPI->SetPixelShaderConstant( 4, linearReflectTint, 1 );
				}
				else
				{
					pShader->SetPixelShaderConstantGammaToLinear( 4, info.REFLECTTINT );
				}
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

			pShaderAPI->SetPixelShaderFogParams( 12 );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 10, vEyePos_SpecExponent, 1 );

			if( bHasFlowmap )
			{
				pShader->SetPixelShaderConstant( 9, info.FLOWMAPSCROLLRATE );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( water_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( water_vs30 );
			
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;

			// Get viewport and render target dimensions and set shader constant to do a 2D mad
			int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
			pShaderAPI->GetCurrentViewport( nViewportX, nViewportY, nViewportWidth, nViewportHeight );

			int nRtWidth, nRtHeight;
			pShaderAPI->GetCurrentRenderTargetDimensions( nRtWidth, nRtHeight );

			float vViewportMad[4];

			// viewport->screen transform
			vViewportMad[0] = ( float )nViewportWidth / ( float )nRtWidth;
			vViewportMad[1] = ( float )nViewportHeight / ( float )nRtHeight;
			vViewportMad[2] = ( float )nViewportX / ( float )nRtWidth;
			vViewportMad[3] = ( float )nViewportY / ( float )nRtHeight;
			DynamicCmdsOut.SetPixelShaderConstant( 24, vViewportMad, 1 );

			CommitBaseDeferredConstants_Origin(pShaderAPI, 36);

			CDeferredExtension* pExt = GetDeferredExt();
			int numForwardLights = pExt->GetNumActiveForwardLights();

			float forwardLightCount[4] = { (float)numForwardLights, 0, 0, 0 };
			DynamicCmdsOut.SetPixelShaderConstant(11, forwardLightCount);

			if (pShaderAPI != NULL && numForwardLights > 0 && numForwardLights < 14)
			{
				float* pLightData = pExt->GetForwardLightData();
				if (pLightData)
				{
					DynamicCmdsOut.SetPixelShaderConstant(69,
						pLightData,
						pExt->GetForwardLights_NumRows());
				}
			}

			float* pSpotlightData = pExt->GetForwardSpotlightData();
			if (pSpotlightData)
			{
				DynamicCmdsOut.SetPixelShaderConstant(37,
					pSpotlightData,
					pExt->GetForwardSpotLights_NumRows());
			}

			const lightData_Global_t& globalLight = pExt->GetLightData_Global();

			if (globalLight.bEnabled)
			{
				float globalLightData[4];

				globalLightData[0] = globalLight.vecLight.x;
				globalLightData[1] = globalLight.vecLight.y;
				globalLightData[2] = globalLight.vecLight.z;
				globalLightData[3] = globalLight.bShadow ? 1.0f : 0.0f;
				pShaderAPI->SetPixelShaderConstant(34, globalLightData, 1);

				pShaderAPI->SetPixelShaderConstant(35, globalLight.diff.Base(), 1);
				pShaderAPI->SetPixelShaderConstant(32, globalLight.ambh.Base(), 1);
				pShaderAPI->SetPixelShaderConstant(33, globalLight.ambl.Base(), 1);
			}

			pShader->BindTexture(SHADER_SAMPLER9, GetDeferredExt()->GetTexture_LightAccum(), 0);

			int x, y, w, t;
			pShaderAPI->GetCurrentViewport(x, y, w, t);
			float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

			pShaderAPI->SetPixelShaderConstant(27, fl1);

			DECLARE_DYNAMIC_PIXEL_SHADER( water_deferred_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, false ); 
			SET_DYNAMIC_PIXEL_SHADER_COMBO(USE_FORWARD_RENDERING, bForwardRendered);
			//SET_DYNAMIC_PIXEL_SHADER_COMBO(USE_GLOBAL_LIGHT, globalLight.bEnabled);
			SET_DYNAMIC_PIXEL_SHADER(water_deferred_ps30);

			DynamicCmdsOut.End();
			pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
		}
		pShader->Draw();
	}
	