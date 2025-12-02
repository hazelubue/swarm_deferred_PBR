// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// !$MODEL && $MORPHING_VTEX
// !$MODEL && $COMPRESSED_VERTS
// !$MODEL && $SKINNING
// !$MODEL && $MORPHING_VTEX

#pragma once
#include "shaderlib/cshader.h"
class shadowpass_ps30_Static_Index
{
	unsigned int m_nALPHATEST : 2;
#ifdef _DEBUG
	bool m_bALPHATEST : 1;
#endif	// _DEBUG
public:
	void SetALPHATEST( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nALPHATEST = i;
#ifdef _DEBUG
		m_bALPHATEST = true;
#endif	// _DEBUG
	}

	shadowpass_ps30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
		m_nALPHATEST = 0;
#ifdef _DEBUG
		m_bALPHATEST = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bALPHATEST );
		return ( 3 * m_nALPHATEST ) + 0;
	}
};

#define shaderStaticTest_shadowpass_ps30 psh_forgot_to_set_static_ALPHATEST


class shadowpass_ps30_Dynamic_Index
{
	unsigned int m_nSHADOW_MODE : 2;
#ifdef _DEBUG
	bool m_bSHADOW_MODE : 1;
#endif	// _DEBUG
public:
	void SetSHADOW_MODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nSHADOW_MODE = i;
#ifdef _DEBUG
		m_bSHADOW_MODE = true;
#endif	// _DEBUG
	}

	shadowpass_ps30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
		m_nSHADOW_MODE = 0;
#ifdef _DEBUG
		m_bSHADOW_MODE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bSHADOW_MODE );
		return ( 1 * m_nSHADOW_MODE ) + 0;
	}
};

#define shaderDynamicTest_shadowpass_ps30 psh_forgot_to_set_dynamic_SHADOW_MODE

