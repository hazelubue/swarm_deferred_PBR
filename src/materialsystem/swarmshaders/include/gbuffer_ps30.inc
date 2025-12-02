// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// !$BUMPMAP2 && $BLENDMODULATE
// !$BUMPMAP && $BUMPMAP2
// $MODEL && $BUMPMAP2
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
class gbuffer_ps30_Static_Index
{
	unsigned int m_nALPHATEST : 2;
	unsigned int m_nBUMPMAP : 2;
	unsigned int m_nNOCULL : 2;
	unsigned int m_nBUMPMAP2 : 2;
	unsigned int m_nBLENDMODULATE : 2;
	unsigned int m_nPARALLAXOCCLUSION : 2;
	unsigned int m_nTRANSLUCENT : 2;
	unsigned int m_nFLOWMAP : 2;
	unsigned int m_nDEDICATEDMRAO : 2;
#ifdef _DEBUG
	bool m_bALPHATEST : 1;
	bool m_bBUMPMAP : 1;
	bool m_bNOCULL : 1;
	bool m_bBUMPMAP2 : 1;
	bool m_bBLENDMODULATE : 1;
	bool m_bPARALLAXOCCLUSION : 1;
	bool m_bTRANSLUCENT : 1;
	bool m_bFLOWMAP : 1;
	bool m_bDEDICATEDMRAO : 1;
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

	void SetBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nBUMPMAP = i;
#ifdef _DEBUG
		m_bBUMPMAP = true;
#endif	// _DEBUG
	}

	void SetNOCULL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNOCULL = i;
#ifdef _DEBUG
		m_bNOCULL = true;
#endif	// _DEBUG
	}

	void SetBUMPMAP2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAP2 = i;
#ifdef _DEBUG
		m_bBUMPMAP2 = true;
#endif	// _DEBUG
	}

	void SetBLENDMODULATE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLENDMODULATE = i;
#ifdef _DEBUG
		m_bBLENDMODULATE = true;
#endif	// _DEBUG
	}

	void SetPARALLAXOCCLUSION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPARALLAXOCCLUSION = i;
#ifdef _DEBUG
		m_bPARALLAXOCCLUSION = true;
#endif	// _DEBUG
	}

	void SetTRANSLUCENT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTRANSLUCENT = i;
#ifdef _DEBUG
		m_bTRANSLUCENT = true;
#endif	// _DEBUG
	}

	void SetFLOWMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLOWMAP = i;
#ifdef _DEBUG
		m_bFLOWMAP = true;
#endif	// _DEBUG
	}

	void SetDEDICATEDMRAO( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDEDICATEDMRAO = i;
#ifdef _DEBUG
		m_bDEDICATEDMRAO = true;
#endif	// _DEBUG
	}

	gbuffer_ps30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
		m_nALPHATEST = 0;
		m_nBUMPMAP = 0;
		m_nNOCULL = 0;
		m_nBUMPMAP2 = 0;
		m_nBLENDMODULATE = 0;
		m_nPARALLAXOCCLUSION = 0;
		m_nTRANSLUCENT = 0;
		m_nFLOWMAP = 0;
		m_nDEDICATEDMRAO = 0;
#ifdef _DEBUG
		m_bALPHATEST = false;
		m_bBUMPMAP = false;
		m_bNOCULL = false;
		m_bBUMPMAP2 = false;
		m_bBLENDMODULATE = false;
		m_bPARALLAXOCCLUSION = false;
		m_bTRANSLUCENT = false;
		m_bFLOWMAP = false;
		m_bDEDICATEDMRAO = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bALPHATEST && m_bBUMPMAP && m_bNOCULL && m_bBUMPMAP2 && m_bBLENDMODULATE && m_bPARALLAXOCCLUSION && m_bTRANSLUCENT && m_bFLOWMAP && m_bDEDICATEDMRAO );
		AssertMsg( !( !m_nBUMPMAP2 && m_nBLENDMODULATE ), "Invalid combo combination ( !BUMPMAP2 && BLENDMODULATE )" );
		AssertMsg( !( !m_nBUMPMAP && m_nBUMPMAP2 ), "Invalid combo combination ( !BUMPMAP && BUMPMAP2 )" );
		return ( 1 * m_nALPHATEST ) + ( 2 * m_nBUMPMAP ) + ( 6 * m_nNOCULL ) + ( 12 * m_nBUMPMAP2 ) + ( 24 * m_nBLENDMODULATE ) + ( 48 * m_nPARALLAXOCCLUSION ) + ( 96 * m_nTRANSLUCENT ) + ( 192 * m_nFLOWMAP ) + ( 384 * m_nDEDICATEDMRAO ) + 0;
	}
};

#define shaderStaticTest_gbuffer_ps30 psh_forgot_to_set_static_ALPHATEST + psh_forgot_to_set_static_BUMPMAP + psh_forgot_to_set_static_NOCULL + psh_forgot_to_set_static_BUMPMAP2 + psh_forgot_to_set_static_BLENDMODULATE + psh_forgot_to_set_static_PARALLAXOCCLUSION + psh_forgot_to_set_static_TRANSLUCENT + psh_forgot_to_set_static_FLOWMAP + psh_forgot_to_set_static_DEDICATEDMRAO


class gbuffer_ps30_Dynamic_Index
{
public:
	gbuffer_ps30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_gbuffer_ps30 1

