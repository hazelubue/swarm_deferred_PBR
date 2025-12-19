
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
    m_pRefraction = NULL;
    m_pReflection = NULL;
    m_pForwardData = NULL;
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

void CDeferredExtension::CommitLightData_Global( const lightData_Global_t &data)
{
	m_globalLight = data;
}

void CDeferredExtension::CommitClock(const float& curTime)
{
    m_curTime = curTime;
}

void CDeferredExtension::CommitMatrixData(float* data, const Vector& origin, const float& zNear, const float& zFar,
    VMatrix& m_matView, VMatrix& m_matProj, VMatrix& m_matViewInv,
    VMatrix& m_matProjInv, VMatrix& m_matLockedViewProjInv)
{
    if (data)
    {
        memcpy(&m_commonData, data, sizeof(m_commonData));
    }
    else
    {
        m_commonData.vecOrigin = Vector4D(origin.x, origin.y, origin.z, 1.0f);
        m_commonData.flZDists[0] = zNear;
        m_commonData.flZDists[1] = zFar;
        m_commonData.matView = m_matView;
        m_commonData.matProj = m_matProj;
        m_commonData.matViewInv = m_matViewInv;
        m_commonData.matProjInv = m_matProjInv;
        m_commonData.matLockedViewProjInv = m_matLockedViewProjInv;
    }
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

void CDeferredExtension::CommitTexture_General( ITexture *pTexNormals, ITexture *pTexWaterNormals, ITexture* pTexReflection, ITexture* pTexRefraction, ITexture *pTexDepth, ITexture* pTexForwardData,
		ITexture *pTexLightingCtrl,
		ITexture *pTexLightAccum )
{
	m_pTexWaterNormals = pTexWaterNormals;
	m_pTexNormals = pTexNormals;
	m_pTexDepth = pTexDepth;
    m_pForwardData = pTexForwardData;
    m_pRefraction = pTexRefraction;
    m_pReflection = pTexReflection;
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

float CDeferredExtension::GetCurrentTime()
{
    return m_curTime;
}

void CDeferredExtension::ClearForwardLights()
{
    m_vecForwardLights.RemoveAll();
    m_vecForwardSpotLights.RemoveAll(); 
    m_vecForwardLightBuffer.RemoveAll();
    m_vecForwardSpotLightBuffer.RemoveAll();
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
    ForwardSpotLightData SpotLight;

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
    SpotLight.direction[0] = dir.x;
    SpotLight.direction[1] = dir.y;
    SpotLight.direction[2] = dir.z;
    SpotLight.direction[3] = constantAtt;  // Can be used for inner cone cosine

    // Attenuation + outer cone
    SpotLight.attenuation[0] = linearAtt;
    SpotLight.attenuation[1] = quadraticAtt;
    SpotLight.attenuation[2] = 0.0f;  // Unused
    SpotLight.attenuation[3] = spotCutoff;  // Outer cone cosine

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

void CDeferredExtension::CommitForwardSpotLightData(const ForwardSpotLightData* pLights, int numLights)
{
    m_vecForwardSpotLights.RemoveAll();
    m_vecForwardSpotLightBuffer.RemoveAll();

    if (!pLights || numLights <= 0)
    {
        m_bForwardLightsDirty = false;
        return;
    }

    numLights = MIN(numLights, 16);
    m_vecForwardSpotLights.AddMultipleToTail(numLights, pLights);
    m_bForwardLightsDirty = true;
}

float* CDeferredExtension::GetForwardLightData()
{
    if (!m_bForwardLightsDirty && m_vecForwardLightBuffer.Count() > 0)
        return m_vecForwardLightBuffer.Base();

    m_vecForwardLightBuffer.RemoveAll();

    if (m_vecForwardLights.Count() == 0)
        return NULL;

    // Each light needs 8 floats (2 float4s)
    int floatsPerLight = 8;
    m_vecForwardLightBuffer.EnsureCapacity(m_vecForwardLights.Count() * floatsPerLight);

    for (int i = 0; i < m_vecForwardLights.Count(); i++)
    {
        const ForwardLightData& light = m_vecForwardLights[i];

        // float4[0]: Position (xyz) + radius (w)
        m_vecForwardLightBuffer.AddToTail(light.position[0]);
        m_vecForwardLightBuffer.AddToTail(light.position[1]);
        m_vecForwardLightBuffer.AddToTail(light.position[2]);
        m_vecForwardLightBuffer.AddToTail(light.position[3]);

        // float4[1]: Color (xyz) + type (w)
        m_vecForwardLightBuffer.AddToTail(light.color[0]);
        m_vecForwardLightBuffer.AddToTail(light.color[1]);
        m_vecForwardLightBuffer.AddToTail(light.color[2]);
        m_vecForwardLightBuffer.AddToTail(light.color[3]);
    }

    m_bForwardLightsDirty = false;
    return m_vecForwardLightBuffer.Base();
}

float* CDeferredExtension::GetForwardSpotlightData()
{
    if (!m_bForwardLightsDirty && m_vecForwardSpotLightBuffer.Count() > 0)
        return m_vecForwardSpotLightBuffer.Base();

    m_vecForwardSpotLightBuffer.RemoveAll(); 

    if (m_vecForwardSpotLights.Count() == 0)
        return NULL;

    // Each spotlight needs 8 floats (2 float4s)
    int floatsPerLight = 8;
    m_vecForwardSpotLightBuffer.EnsureCapacity(m_vecForwardSpotLights.Count() * floatsPerLight);

    for (int i = 0; i < m_vecForwardSpotLights.Count(); i++)
    {
        const ForwardSpotLightData& light = m_vecForwardSpotLights[i];

        // float4[0]: Direction (xyz) + inner cone (w)
        m_vecForwardSpotLightBuffer.AddToTail(light.direction[0]);
        m_vecForwardSpotLightBuffer.AddToTail(light.direction[1]);
        m_vecForwardSpotLightBuffer.AddToTail(light.direction[2]);
        m_vecForwardSpotLightBuffer.AddToTail(light.direction[3]);

        // float4[1]: unused (xyz) + outer cone (w)
        m_vecForwardSpotLightBuffer.AddToTail(light.attenuation[0]);
        m_vecForwardSpotLightBuffer.AddToTail(light.attenuation[1]);
        m_vecForwardSpotLightBuffer.AddToTail(light.attenuation[2]);
        m_vecForwardSpotLightBuffer.AddToTail(light.attenuation[3]);
    }

    m_bForwardLightsDirty = false;
    return m_vecForwardSpotLightBuffer.Base();
}

int CDeferredExtension::GetForwardLights_NumRows()
{
    // Each light uses 2 float4 rows (8 floats)
    return m_vecForwardLights.Count() * 2;
}

int CDeferredExtension::GetForwardSpotLights_NumRows()
{
    // Each spotlight uses 2 float4 rows (8 floats)
    return m_vecForwardSpotLights.Count() * 2;
}

int CDeferredExtension::GetNumActiveForwardLights()
{
    return m_vecForwardLights.Count();
}

void CDeferredExtension::FillDataForFramebuffer()
{
    //// Get light data
    //GetForwardLightData();
    //GetForwardSpotlightData();

    //pRenderContext->PopRenderTargetAndViewport();

    //pRenderContext->CopyRenderTargetToTextureEx(m_pForwardData, 0, NULL, NULL);

}