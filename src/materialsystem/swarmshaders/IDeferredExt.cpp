
#include "deferred_includes.h"

CDeferredExtension __g_defExt;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CDeferredExtension, IDeferredExtension, DEFERRED_EXTENSION_VERSION, __g_defExt );

CDeferredExtension::CDeferredExtension()
{
	m_bDefLightingEnabled = false;

	m_vecOrigin.Init();
	m_vecForward.Init();
	m_flZDists[0] = m_flZDists[1] = m_flZDists[2] = 0;
	m_matTFrustumD.Identity();

	m_pTexNormals = NULL;
	m_pTexWaterNormals = NULL;
	m_pTexDepth = NULL;
	m_pTexLightAccum = NULL;
#if ( DEFCFG_LIGHTCTRL_PACKING == 0 )
	m_pTexLightCtrl = NULL;
#endif
	Q_memset( m_pTexShadowDepth_Ortho, 0, sizeof( ITexture* ) * MAX_SHADOW_ORTHO );
	Q_memset( m_pTexShadowDepth_DP, 0, sizeof( ITexture* ) * MAX_SHADOW_DP );
	Q_memset( m_pTexShadowDepth_Proj, 0, sizeof( ITexture* ) * MAX_SHADOW_PROJ );
	Q_memset( m_pTexCookie, 0, sizeof( ITexture* ) * NUM_COOKIE_SLOTS );
	m_pTexVolumePrePass = NULL;

	m_pflCommonLightData = NULL;
	m_iCommon_NumRows = 0;
	m_iNumCommon_ShadowedCookied = 0;
	m_iNumCommon_Shadowed = 0;
	m_iNumCommon_Cookied = 0;
	m_iNumCommon_Simple = 0;
}

CDeferredExtension::~CDeferredExtension()
{
}


void CDeferredExtension::EnableDeferredLighting()
{
	m_bDefLightingEnabled = true;
}

bool CDeferredExtension::IsDeferredLightingEnabled()
{
	return m_bDefLightingEnabled;
}


void CDeferredExtension::CommitOrigin( const Vector &origin )
{
	VectorCopy( origin.Base(), m_vecOrigin.Base() );
}
void CDeferredExtension::CommitViewForward( const Vector &fwd )
{
	VectorCopy( fwd.Base(), m_vecForward.Base() );
}
void CDeferredExtension::CommitZDists( const float &zNear, const float &zFar )
{
	m_flZDists[0] = zNear;
	m_flZDists[1] = zFar;
}
void CDeferredExtension::CommitZScale( const float &zFar )
{
	m_flZDists[2] = zFar;
}
void CDeferredExtension::CommitFrustumDeltas( const VMatrix &matTFrustum )
{
	m_matTFrustumD = matTFrustum;
}

void CDeferredExtension::CommitShadowData_Ortho( const int &index, const shadowData_ortho_t &data )
{
	Assert( index >= 0 && index < SHADOW_NUM_CASCADES );
	m_dataOrtho[ index ] = data;
}
void CDeferredExtension::CommitShadowData_Proj( const int &index, const shadowData_proj_t &data )
{
	Assert( index >= 0 && index < MAX_SHADOW_PROJ );
	m_dataProj[ index ] = data;
}
void CDeferredExtension::CommitShadowData_General( const shadowData_general_t &data )
{
	m_dataGeneral = data;
}

void CDeferredExtension::CommitVolumeData( const volumeData_t &data )
{
	m_dataVolume = data;
}

void CDeferredExtension::CommitLightData_Global( const lightData_Global_t &data )
{
	m_globalLight = data;
}

float *CDeferredExtension::CommitLightData_Common( float *pFlData, int numRows,
		int numShadowedCookied, int numShadowed,
		int numCookied, int numSimple )
{
	float *pReturn = m_pflCommonLightData;

	m_pflCommonLightData = pFlData;
	m_iCommon_NumRows = numRows;
	m_iNumCommon_ShadowedCookied = numShadowedCookied;
	m_iNumCommon_Shadowed = numShadowed;
	m_iNumCommon_Cookied = numCookied;
	m_iNumCommon_Simple = numSimple;

	return pReturn;
}

void CDeferredExtension::CommitTexture_General( ITexture *pTexNormals, ITexture *pTexWaterNormals, ITexture *pTexDepth,
		ITexture *pTexLightingCtrl,
		ITexture *pTexLightAccum )
{
	m_pTexWaterNormals = pTexWaterNormals;
	m_pTexNormals = pTexNormals;
	m_pTexDepth = pTexDepth;
	m_pTexLightAccum = pTexLightAccum;
	m_pTexLightCtrl = pTexLightingCtrl;
}
void CDeferredExtension::CommitTexture_CascadedDepth( const int &index, ITexture *pTexShadowDepth )
{
	Assert( index >= 0 && index < MAX_SHADOW_ORTHO );
	m_pTexShadowDepth_Ortho[ index ] = pTexShadowDepth;
}
void CDeferredExtension::CommitTexture_DualParaboloidDepth( const int &index, ITexture *pTexShadowDepth )
{
	Assert( index >= 0 && index < MAX_SHADOW_DP );
	m_pTexShadowDepth_DP[ index ] = pTexShadowDepth;
}
void CDeferredExtension::CommitTexture_ProjectedDepth( const int &index, ITexture *pTexShadowDepth )
{
	Assert( index >= 0 && index < MAX_SHADOW_PROJ );
	m_pTexShadowDepth_Proj[ index ] = pTexShadowDepth;
}
void CDeferredExtension::CommitTexture_Cookie( const int &index, ITexture *pTexCookie )
{
	Assert( index >= 0 && index < NUM_COOKIE_SLOTS );
	m_pTexCookie[ index ] = pTexCookie;
}
void CDeferredExtension::CommitTexture_VolumePrePass( ITexture *pTexVolumePrePass )
{
	m_pTexVolumePrePass = pTexVolumePrePass;
}
void CDeferredExtension::ClearForwardLights()
{
    m_vecForwardLights.RemoveAll();
    m_vecForwardLightBuffer.RemoveAll();
    m_bForwardLightsDirty = true;
}

void CDeferredExtension::AddForwardLight(const Vector& pos, float radius,
    const Vector& color, float intensity,
    int type, const Vector& dir,
    float constantAtt, float linearAtt,
    float quadraticAtt, float spotCutoff)
{
    if (m_vecForwardLights.Count() >= 16)
        return;

    ForwardLightData light;

    // Position + radius
    light.position[0] = pos.x;
    light.position[1] = pos.y;
    light.position[2] = pos.z;
    light.position[3] = radius;

    // Color + light type (0=point, 1=spot)
    light.color[0] = color.x * intensity;
    light.color[1] = color.y * intensity;
    light.color[2] = color.z * intensity;
    light.color[3] = (float)type;  // Store type here

    // Direction + inner cone
    light.direction[0] = dir.x;
    light.direction[1] = dir.y;
    light.direction[2] = dir.z;
    light.direction[3] = constantAtt;  // Can be used for inner cone cosine

    // Attenuation + outer cone
    light.attenuation[0] = linearAtt;
    light.attenuation[1] = quadraticAtt;
    light.attenuation[2] = 0.0f;  // Unused
    light.attenuation[3] = spotCutoff;  // Outer cone cosine

    m_vecForwardLights.AddToTail(light);
    m_bForwardLightsDirty = true;
}

void CDeferredExtension::CommitForwardLightData(const ForwardLightData* pLights, int numLights)
{
    m_vecForwardLights.RemoveAll();
    m_vecForwardLightBuffer.RemoveAll();

    if (!pLights || numLights <= 0)
    {
        m_bForwardLightsDirty = false;
        return;
    }

    // Clamp to maximum supported lights
    numLights = MIN(numLights, 16);

    // Copy light data
    m_vecForwardLights.AddMultipleToTail(numLights, pLights);

    m_bForwardLightsDirty = true;
}

float* CDeferredExtension::GetForwardLightData()
{
    // Return cached data if not dirty
    if (!m_bForwardLightsDirty && m_vecForwardLightBuffer.Count() > 0)
    {
        return m_vecForwardLightBuffer.Base();
    }

    // Clear and rebuild buffer
    m_vecForwardLightBuffer.RemoveAll();

    if (m_vecForwardLights.Count() == 0)
        return NULL;

    // Each light needs 16 floats (4 float4s)
    int floatsPerLight = 16;
    m_vecForwardLightBuffer.EnsureCapacity(m_vecForwardLights.Count() * floatsPerLight);

    for (int i = 0; i < m_vecForwardLights.Count(); i++)
    {
        const ForwardLightData& light = m_vecForwardLights[i];

        // float4[0]: Position (xyz) + radius (w)
        m_vecForwardLightBuffer.AddToTail(light.position[0]);
        m_vecForwardLightBuffer.AddToTail(light.position[1]);
        m_vecForwardLightBuffer.AddToTail(light.position[2]);
        m_vecForwardLightBuffer.AddToTail(light.position[3]);

        // float4[1]: Color (xyz) + light type (w) - 0=point, 1=spot
        m_vecForwardLightBuffer.AddToTail(light.color[0]);
        m_vecForwardLightBuffer.AddToTail(light.color[1]);
        m_vecForwardLightBuffer.AddToTail(light.color[2]);
        m_vecForwardLightBuffer.AddToTail(light.color[3]);

        // float4[2]: Spotlight direction (xyz) + inner cone angle cosine (w)
        m_vecForwardLightBuffer.AddToTail(light.direction[0]);
        m_vecForwardLightBuffer.AddToTail(light.direction[1]);
        m_vecForwardLightBuffer.AddToTail(light.direction[2]);
        m_vecForwardLightBuffer.AddToTail(light.direction[3]);

        // float4[3]: Unused (xyz) + outer cone angle cosine (w)
        m_vecForwardLightBuffer.AddToTail(light.attenuation[0]);
        m_vecForwardLightBuffer.AddToTail(light.attenuation[1]);
        m_vecForwardLightBuffer.AddToTail(light.attenuation[2]);
        m_vecForwardLightBuffer.AddToTail(light.attenuation[3]);
    }

    m_bForwardLightsDirty = false;
    return m_vecForwardLightBuffer.Base();
}

float* CDeferredExtension::GetForwardSpotlightData()
{
    // Return cached data if not dirty
    if (!m_bForwardLightsDirty && m_vecForwardLightBuffer.Count() > 0)
    {
        return m_vecForwardLightBuffer.Base();
    }

    // Clear and rebuild buffer
    m_vecForwardLightBuffer.RemoveAll();

    if (m_vecForwardLights.Count() == 0)
        return NULL;

    // Each light needs 16 floats (4 float4s)
    int floatsPerLight = 16;
    m_vecForwardLightBuffer.EnsureCapacity(m_vecForwardLights.Count() * floatsPerLight);

    for (int i = 0; i < m_vecForwardLights.Count(); i++)
    {
        const ForwardLightData& light = m_vecForwardLights[i];

        // float4[0]: Position (xyz) + radius (w)
        m_vecForwardLightBuffer.AddToTail(light.direction[0]);
        m_vecForwardLightBuffer.AddToTail(light.direction[1]);
        m_vecForwardLightBuffer.AddToTail(light.direction[2]);
        m_vecForwardLightBuffer.AddToTail(light.direction[3]);

        // float4[1]: Color (xyz) + light type (w) - 0=point, 1=spot
        m_vecForwardLightBuffer.AddToTail(light.attenuation[0]);
        m_vecForwardLightBuffer.AddToTail(light.attenuation[1]);
        m_vecForwardLightBuffer.AddToTail(light.attenuation[2]);
        m_vecForwardLightBuffer.AddToTail(light.attenuation[3]);


    }

    m_bForwardLightsDirty = false;
    return m_vecForwardLightBuffer.Base();
}

int CDeferredExtension::GetForwardLights_NumRows()
{
    // Each light uses 4 float4 rows
    return m_vecForwardLights.Count() * 4;
}

int CDeferredExtension::GetNumActiveForwardLights()
{
    return m_vecForwardLights.Count();
}

