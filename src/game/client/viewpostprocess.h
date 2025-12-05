//========== Copyright © 2005, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VIEWPOSTPROCESS_H
#define VIEWPOSTPROCESS_H

#if defined( _WIN32 )
#pragma once
#endif

#include "postprocess_shared.h"
#include "ScreenSpaceEffects.h"

struct RenderableInstance_t;

void DoEnginePostProcessing( int x, int y, int w, int h, bool bFlashlightIsOn, bool bPostVGui = false );
void DoImageSpaceMotionBlur( const CViewSetup &view );
bool IsDepthOfFieldEnabled();
void DoDepthOfField( const CViewSetup &view );
void BlurEntity( IClientRenderable *pRenderable, bool bPreDraw, int drawFlags, const RenderableInstance_t &instance, const CViewSetup &view, int x, int y, int w, int h );

void UpdateMaterialSystemTonemapScalar();

float GetCurrentTonemapScale();

void SetOverrideTonemapScale( bool bEnableOverride, float flTonemapScale );

void SetOverridePostProcessingDisable( bool bForceOff );

void DoBlurFade( float flStrength, float flDesaturate, int x, int y, int w, int h );

void SetPostProcessParams( const PostProcessParameters_t *pPostProcessParameters );

void SetViewFadeParams( byte r, byte g, byte b, byte a, bool bModulate );

//class CSSGI : public IScreenSpaceEffect
//{
//public:
//	CSSGI(void) {};
//
//	virtual void Init(void);
//	virtual void Shutdown(void);
//	virtual void SetParameters(KeyValues* params) {};
//	virtual void Enable(bool bEnable) { m_bEnabled = bEnable; }
//	virtual bool IsEnabled() { return m_bEnabled; }
//
//	virtual void Render(int x, int y, int w, int h);
//
//private:
//	bool				m_bEnabled;
//
//	CTextureReference	m_Normal;
//	CTextureReference	m_SSGI;
//
//	CTextureReference	m_SSAOY;
//
//
//	CMaterialReference	m_SSAO_BilateralY;
//	CMaterialReference	m_SSAO_BilateralX;
//	CMaterialReference	m_SSGI_Mat;
//	CMaterialReference	m_SSGI_Combine;
//};
//
//static CSSGI g_SSGI;


#endif // VIEWPOSTPROCESS_H
