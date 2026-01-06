// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// $SEAMLESS && $PARALLAX_MAPPING
// $SEAMLESS && ( $OUTLINE || $SOFTEDGES)
// $BASETEXTURE2 && ( $OUTLINE || $SOFTEDGES)
// $BUMPMAP2 && ( $OUTLINE || $SOFTEDGES)
// $SELFILLUM && ( $OUTLINE || $SOFTEDGES)
// $MASKEDBLENDING && ( $OUTLINE || $SOFTEDGES)
// $FANCY_BLENDING && ( $OUTLINE || $SOFTEDGES)
// $LIGHTING_PREVIEW && ( $OUTLINE || $SOFTEDGES)
// ($FASTPATH == 0) && ( $OUTLINE || $SOFTEDGES)
// (($DETAIL_BLEND_MODE != 12 ) && $BUMPMAP) && ( $OUTLINE || $SOFTEDGES)
// ($WARPLIGHTING) && ( $OUTLINE || $SOFTEDGES)
// ($BUMPMAP) && ( $OUTLINE || $SOFTEDGES)
// ($DETAIL_BLEND_MODE == 7 ) && ( $SOFTEDGES || $BUMPMAP2)
// ($DETAIL_BLEND_MODE == 6 )
// ($DETAIL_BLEND_MODE == 10 ) && ($BUMPMAP == 0 )
// ($DETAIL_BLEND_MODE == 11 ) && ($BUMPMAP != 0 )
// ($FANCY_BLENDING) && ($DETAIL_BLEND_MODE == 7 )
// ($PARALLAX_MAPPING) && ($BASETEXTURE2 || $BUMPMAP2 || $DETAILTEXTURE  || $BASETEXTURE2 )
// ($FANCY_BLENDING) && ($BASETEXTURE2 == 0 ) && ($BUMPMAP2 == 0 )
// ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )
// ($DETAILTEXTURE == 0) && ($DETAIL_BLEND_MODE != 12)
// $BUMPMAP2 && $WARPLIGHTING
// $WARPLIGHTING && ( $DETAIL_BLEND_MODE != 12 )
// $ENVMAPMASK && $BUMPMAP
// $NORMALMAPALPHAENVMAPMASK && $BASEALPHAENVMAPMASK
// $NORMALMAPALPHAENVMAPMASK && $ENVMAPMASK
// $BASEALPHAENVMAPMASK && $ENVMAPMASK
// $BASEALPHAENVMAPMASK && $SELFILLUM
// !$FASTPATH && $FASTPATHENVMAPCONTRAST
// !$FASTPATH && $FASTPATHENVMAPTINT
// !$BUMPMAP && $DIFFUSEBUMPMAP
// !$BUMPMAP && $BUMPMAP2
// $ENVMAPMASK && $BUMPMAP2
// $BASETEXTURENOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
// $BASETEXTURE2NOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
// $BASEALPHAENVMAPMASK && $BUMPMAP
// $PARALLAXMAP && ( $DETAIL_BLEND_MODE != 12 )
// $SEAMLESS && $PARALLAXMAP
// $SEAMLESS && ( $DETAIL_BLEND_MODE != 12 )
// $SEAMLESS && $MASKEDBLENDING
// $BUMPMASK && ( $SEAMLESS ||  ( $DETAILTEXTURE != 12 ) || $SELFILLUM || $BASETEXTURENOENVMAP || $BASETEXTURE2 )
// $FANCY_BLENDING && $BUMPMAP && $DETAILTEXTURE
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
// !$MODEL && $MORPHING_VTEX
// !$MODEL && $COMPRESSED_VERTS
// !$MODEL && $SKINNING
// !$MODEL && $MORPHING_VTEX
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class lightmappedgeneric_deferred_ps30_Static_Index
{
	unsigned int m_nMASKEDBLENDING : 2;
	unsigned int m_nBASETEXTURE2 : 2;
	unsigned int m_nBUMPMAP : 2;
	unsigned int m_nBUMPMAP2 : 2;
	unsigned int m_nCUBEMAP : 2;
	unsigned int m_nENVMAPMASK : 2;
	unsigned int m_nBASEALPHAENVMAPMASK : 2;
	unsigned int m_nSELFILLUM : 2;
	unsigned int m_nNORMALMAPALPHAENVMAPMASK : 2;
	unsigned int m_nDIFFUSEBUMPMAP : 2;
	unsigned int m_nFANCY_BLENDING : 2;
	unsigned int m_nSEAMLESS : 2;
	unsigned int m_nSOFTEDGES : 2;
	unsigned int m_nBUMPMASK : 2;
	unsigned int m_nDETAILTEXTURE : 2;
	unsigned int m_nDETAIL_BLEND_MODE : 3;
	unsigned int m_nDOPIXELFOG : 2;
#ifdef _DEBUG
	bool m_bMASKEDBLENDING : 1;
	bool m_bBASETEXTURE2 : 1;
	bool m_bBUMPMAP : 1;
	bool m_bBUMPMAP2 : 1;
	bool m_bCUBEMAP : 1;
	bool m_bENVMAPMASK : 1;
	bool m_bBASEALPHAENVMAPMASK : 1;
	bool m_bSELFILLUM : 1;
	bool m_bNORMALMAPALPHAENVMAPMASK : 1;
	bool m_bDIFFUSEBUMPMAP : 1;
	bool m_bFANCY_BLENDING : 1;
	bool m_bSEAMLESS : 1;
	bool m_bSOFTEDGES : 1;
	bool m_bBUMPMASK : 1;
	bool m_bDETAILTEXTURE : 1;
	bool m_bDETAIL_BLEND_MODE : 1;
#endif	// _DEBUG
public:
	void SetMASKEDBLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMASKEDBLENDING = i;
#ifdef _DEBUG
		m_bMASKEDBLENDING = true;
#endif	// _DEBUG
	}

	void SetBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
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

	void SetBUMPMAP2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAP2 = i;
#ifdef _DEBUG
		m_bBUMPMAP2 = true;
#endif	// _DEBUG
	}

	void SetCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nCUBEMAP = i;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif	// _DEBUG
	}

	void SetENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAPMASK = i;
#ifdef _DEBUG
		m_bENVMAPMASK = true;
#endif	// _DEBUG
	}

	void SetBASEALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASEALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bBASEALPHAENVMAPMASK = true;
#endif	// _DEBUG
	}

	void SetSELFILLUM( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUM = i;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif	// _DEBUG
	}

	void SetNORMALMAPALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALMAPALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bNORMALMAPALPHAENVMAPMASK = true;
#endif	// _DEBUG
	}

	void SetDIFFUSEBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDIFFUSEBUMPMAP = i;
#ifdef _DEBUG
		m_bDIFFUSEBUMPMAP = true;
#endif	// _DEBUG
	}

	void SetFANCY_BLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFANCY_BLENDING = i;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif	// _DEBUG
	}

	void SetSEAMLESS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEAMLESS = i;
#ifdef _DEBUG
		m_bSEAMLESS = true;
#endif	// _DEBUG
	}

	void SetSOFTEDGES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSOFTEDGES = i;
#ifdef _DEBUG
		m_bSOFTEDGES = true;
#endif	// _DEBUG
	}

	void SetBUMPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMASK = i;
#ifdef _DEBUG
		m_bBUMPMASK = true;
#endif	// _DEBUG
	}

	void SetDETAILTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
#endif	// _DEBUG
	}

	void SetDETAIL_BLEND_MODE( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nDETAIL_BLEND_MODE = i;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = true;
#endif	// _DEBUG
	}

	void SetDOPIXELFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOPIXELFOG = i;
	}

	lightmappedgeneric_deferred_ps30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
		m_nMASKEDBLENDING = 0;
		m_nBASETEXTURE2 = 0;
		m_nBUMPMAP = 0;
		m_nBUMPMAP2 = 0;
		m_nCUBEMAP = 0;
		m_nENVMAPMASK = 0;
		m_nBASEALPHAENVMAPMASK = 0;
		m_nSELFILLUM = 0;
		m_nNORMALMAPALPHAENVMAPMASK = 0;
		m_nDIFFUSEBUMPMAP = 0;
		m_nFANCY_BLENDING = 0;
		m_nSEAMLESS = 0;
		m_nSOFTEDGES = 0;
		m_nBUMPMASK = 0;
		m_nDETAILTEXTURE = 0;
		m_nDETAIL_BLEND_MODE = 0;
		m_nDOPIXELFOG = !IS_FLAG_SET( MATERIAL_VAR_VERTEXFOG ) ? 1 : 0 ;
#ifdef _DEBUG
		m_bMASKEDBLENDING = false;
		m_bBASETEXTURE2 = false;
		m_bBUMPMAP = false;
		m_bBUMPMAP2 = false;
		m_bCUBEMAP = false;
		m_bENVMAPMASK = false;
		m_bBASEALPHAENVMAPMASK = false;
		m_bSELFILLUM = false;
		m_bNORMALMAPALPHAENVMAPMASK = false;
		m_bDIFFUSEBUMPMAP = false;
		m_bFANCY_BLENDING = false;
		m_bSEAMLESS = false;
		m_bSOFTEDGES = false;
		m_bBUMPMASK = false;
		m_bDETAILTEXTURE = false;
		m_bDETAIL_BLEND_MODE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bMASKEDBLENDING && m_bBASETEXTURE2 && m_bBUMPMAP && m_bBUMPMAP2 && m_bCUBEMAP && m_bENVMAPMASK && m_bBASEALPHAENVMAPMASK && m_bSELFILLUM && m_bNORMALMAPALPHAENVMAPMASK && m_bDIFFUSEBUMPMAP && m_bFANCY_BLENDING && m_bSEAMLESS && m_bSOFTEDGES && m_bBUMPMASK && m_bDETAILTEXTURE && m_bDETAIL_BLEND_MODE );
		AssertMsg( !( ( m_nDETAIL_BLEND_MODE == 7 ) && ( m_nSOFTEDGES || m_nBUMPMAP2 ) ), "Invalid combo combination ( ( DETAIL_BLEND_MODE == 7 ) && ( SOFTEDGES || BUMPMAP2 ) )" );
		AssertMsg( !( m_nDETAIL_BLEND_MODE == 6 ), "Invalid combo combination ( DETAIL_BLEND_MODE == 6 )" );
		AssertMsg( !( ( m_nDETAIL_BLEND_MODE == 10 ) && ( m_nBUMPMAP == 0 ) ), "Invalid combo combination ( ( DETAIL_BLEND_MODE == 10 ) && ( BUMPMAP == 0 ) )" );
		AssertMsg( !( ( m_nDETAIL_BLEND_MODE == 11 ) && ( m_nBUMPMAP != 0 ) ), "Invalid combo combination ( ( DETAIL_BLEND_MODE == 11 ) && ( BUMPMAP != 0 ) )" );
		AssertMsg( !( m_nFANCY_BLENDING && ( m_nDETAIL_BLEND_MODE == 7 ) ), "Invalid combo combination ( FANCY_BLENDING && ( DETAIL_BLEND_MODE == 7 ) )" );
		AssertMsg( !( m_nFANCY_BLENDING && ( ( m_nBASETEXTURE2 == 0 ) && ( m_nBUMPMAP2 == 0 ) ) ), "Invalid combo combination ( FANCY_BLENDING && ( ( BASETEXTURE2 == 0 ) && ( BUMPMAP2 == 0 ) ) )" );
		AssertMsg( !( ( m_nDETAILTEXTURE == 0 ) && ( m_nDETAIL_BLEND_MODE != 12 ) ), "Invalid combo combination ( ( DETAILTEXTURE == 0 ) && ( DETAIL_BLEND_MODE != 12 ) )" );
		AssertMsg( !( m_nENVMAPMASK && m_nBUMPMAP ), "Invalid combo combination ( ENVMAPMASK && BUMPMAP )" );
		AssertMsg( !( m_nNORMALMAPALPHAENVMAPMASK && m_nBASEALPHAENVMAPMASK ), "Invalid combo combination ( NORMALMAPALPHAENVMAPMASK && BASEALPHAENVMAPMASK )" );
		AssertMsg( !( m_nNORMALMAPALPHAENVMAPMASK && m_nENVMAPMASK ), "Invalid combo combination ( NORMALMAPALPHAENVMAPMASK && ENVMAPMASK )" );
		AssertMsg( !( m_nBASEALPHAENVMAPMASK && m_nENVMAPMASK ), "Invalid combo combination ( BASEALPHAENVMAPMASK && ENVMAPMASK )" );
		AssertMsg( !( m_nBASEALPHAENVMAPMASK && m_nSELFILLUM ), "Invalid combo combination ( BASEALPHAENVMAPMASK && SELFILLUM )" );
		AssertMsg( !( !m_nBUMPMAP && m_nDIFFUSEBUMPMAP ), "Invalid combo combination ( !BUMPMAP && DIFFUSEBUMPMAP )" );
		AssertMsg( !( !m_nBUMPMAP && m_nBUMPMAP2 ), "Invalid combo combination ( !BUMPMAP && BUMPMAP2 )" );
		AssertMsg( !( m_nENVMAPMASK && m_nBUMPMAP2 ), "Invalid combo combination ( ENVMAPMASK && BUMPMAP2 )" );
		AssertMsg( !( m_nBASEALPHAENVMAPMASK && m_nBUMPMAP ), "Invalid combo combination ( BASEALPHAENVMAPMASK && BUMPMAP )" );
		AssertMsg( !( m_nSEAMLESS && ( m_nDETAIL_BLEND_MODE != 12 ) ), "Invalid combo combination ( SEAMLESS && ( DETAIL_BLEND_MODE != 12 ) )" );
		AssertMsg( !( m_nSEAMLESS && m_nMASKEDBLENDING ), "Invalid combo combination ( SEAMLESS && MASKEDBLENDING )" );
		AssertMsg( !( m_nFANCY_BLENDING && ( m_nBUMPMAP && m_nDETAILTEXTURE ) ), "Invalid combo combination ( FANCY_BLENDING && ( BUMPMAP && DETAILTEXTURE ) )" );
		return ( 32 * m_nMASKEDBLENDING ) + ( 64 * m_nBASETEXTURE2 ) + ( 128 * m_nBUMPMAP ) + ( 384 * m_nBUMPMAP2 ) + ( 768 * m_nCUBEMAP ) + ( 2304 * m_nENVMAPMASK ) + ( 4608 * m_nBASEALPHAENVMAPMASK ) + ( 9216 * m_nSELFILLUM ) + ( 18432 * m_nNORMALMAPALPHAENVMAPMASK ) + ( 36864 * m_nDIFFUSEBUMPMAP ) + ( 73728 * m_nFANCY_BLENDING ) + ( 147456 * m_nSEAMLESS ) + ( 294912 * m_nSOFTEDGES ) + ( 589824 * m_nBUMPMASK ) + ( 1179648 * m_nDETAILTEXTURE ) + ( 2359296 * m_nDETAIL_BLEND_MODE ) + ( 9437184 * m_nDOPIXELFOG ) + 0;
	}
};

#define shaderStaticTest_lightmappedgeneric_deferred_ps30 psh_forgot_to_set_static_MASKEDBLENDING + psh_forgot_to_set_static_BASETEXTURE2 + psh_forgot_to_set_static_BUMPMAP + psh_forgot_to_set_static_BUMPMAP2 + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_ENVMAPMASK + psh_forgot_to_set_static_BASEALPHAENVMAPMASK + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_NORMALMAPALPHAENVMAPMASK + psh_forgot_to_set_static_DIFFUSEBUMPMAP + psh_forgot_to_set_static_FANCY_BLENDING + psh_forgot_to_set_static_SEAMLESS + psh_forgot_to_set_static_SOFTEDGES + psh_forgot_to_set_static_BUMPMASK + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_DETAIL_BLEND_MODE


class lightmappedgeneric_deferred_ps30_Dynamic_Index
{
	unsigned int m_nFASTPATHENVMAPCONTRAST : 2;
	unsigned int m_nFASTPATH : 2;
	unsigned int m_nWRITEWATERFOGTODESTALPHA : 2;
	unsigned int m_nWRITE_DEPTH_TO_DESTALPHA : 2;
	unsigned int m_nPIXELFOGTYPE : 2;
#ifdef _DEBUG
	bool m_bFASTPATHENVMAPCONTRAST : 1;
	bool m_bFASTPATH : 1;
	bool m_bWRITEWATERFOGTODESTALPHA : 1;
	bool m_bWRITE_DEPTH_TO_DESTALPHA : 1;
#endif	// _DEBUG
public:
	void SetFASTPATHENVMAPCONTRAST( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATHENVMAPCONTRAST = i;
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = true;
#endif	// _DEBUG
	}

	void SetFASTPATH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATH = i;
#ifdef _DEBUG
		m_bFASTPATH = true;
#endif	// _DEBUG
	}

	void SetWRITEWATERFOGTODESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITEWATERFOGTODESTALPHA = i;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
#endif	// _DEBUG
	}

	void SetWRITE_DEPTH_TO_DESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITE_DEPTH_TO_DESTALPHA = i;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = true;
#endif	// _DEBUG
	}

	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
	}

	lightmappedgeneric_deferred_ps30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
		m_nFASTPATHENVMAPCONTRAST = 0;
		m_nFASTPATH = 0;
		m_nWRITEWATERFOGTODESTALPHA = 0;
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
		m_nPIXELFOGTYPE = ( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ;
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = false;
		m_bFASTPATH = false;
		m_bWRITEWATERFOGTODESTALPHA = false;
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bFASTPATHENVMAPCONTRAST && m_bFASTPATH && m_bWRITEWATERFOGTODESTALPHA && m_bWRITE_DEPTH_TO_DESTALPHA );
		AssertMsg( !( !m_nFASTPATH && m_nFASTPATHENVMAPCONTRAST ), "Invalid combo combination ( !FASTPATH && FASTPATHENVMAPCONTRAST )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		return ( 1 * m_nFASTPATHENVMAPCONTRAST ) + ( 2 * m_nFASTPATH ) + ( 4 * m_nWRITEWATERFOGTODESTALPHA ) + ( 8 * m_nWRITE_DEPTH_TO_DESTALPHA ) + ( 16 * m_nPIXELFOGTYPE ) + 0;
	}
};

#define shaderDynamicTest_lightmappedgeneric_deferred_ps30 psh_forgot_to_set_dynamic_FASTPATHENVMAPCONTRAST + psh_forgot_to_set_dynamic_FASTPATH + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA

