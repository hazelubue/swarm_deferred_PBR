// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class skydome_ps30_Static_Index
{
	unsigned int m_nCONVERT_TO_SRGB : 1;
#ifdef _DEBUG
	bool m_bCONVERT_TO_SRGB : 1;
#endif	// _DEBUG
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nCONVERT_TO_SRGB = i;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif	// _DEBUG
	}

	skydome_ps30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
		m_nCONVERT_TO_SRGB = 0;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCONVERT_TO_SRGB );
		return ( 8 * m_nCONVERT_TO_SRGB ) + 0;
	}
};

#define shaderStaticTest_skydome_ps30 psh_forgot_to_set_static_CONVERT_TO_SRGB


class skydome_ps30_Dynamic_Index
{
	unsigned int m_nRENDER_SKY : 2;
	unsigned int m_nENABLE_STARS : 2;
	unsigned int m_nENABLE_SUN : 2;
#ifdef _DEBUG
	bool m_bRENDER_SKY : 1;
	bool m_bENABLE_STARS : 1;
	bool m_bENABLE_SUN : 1;
#endif	// _DEBUG
public:
	void SetRENDER_SKY( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nRENDER_SKY = i;
#ifdef _DEBUG
		m_bRENDER_SKY = true;
#endif	// _DEBUG
	}

	void SetENABLE_STARS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENABLE_STARS = i;
#ifdef _DEBUG
		m_bENABLE_STARS = true;
#endif	// _DEBUG
	}

	void SetENABLE_SUN( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENABLE_SUN = i;
#ifdef _DEBUG
		m_bENABLE_SUN = true;
#endif	// _DEBUG
	}

	skydome_ps30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
		m_nRENDER_SKY = 0;
		m_nENABLE_STARS = 0;
		m_nENABLE_SUN = 0;
#ifdef _DEBUG
		m_bRENDER_SKY = false;
		m_bENABLE_STARS = false;
		m_bENABLE_SUN = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bRENDER_SKY && m_bENABLE_STARS && m_bENABLE_SUN );
		return ( 1 * m_nRENDER_SKY ) + ( 2 * m_nENABLE_STARS ) + ( 4 * m_nENABLE_SUN ) + 0;
	}
};

#define shaderDynamicTest_skydome_ps30 psh_forgot_to_set_dynamic_RENDER_SKY + psh_forgot_to_set_dynamic_ENABLE_STARS + psh_forgot_to_set_dynamic_ENABLE_SUN

