
#include "deferred_includes.h"

CDeferredExtension __g_defExt;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CDeferredExtension, IDeferredExtension, DEFERRED_EXTENSION_VERSION, __g_defExt );

CDeferredExtension::CDeferredExtension()
{
	m_bDefLightingEnabled = false;

    m_pIndexTexture = nullptr;
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

    m_bRegeneratorSet = false;
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

void CDeferredExtension::CommitMatrixData(float* data, const float& aspect, const float& fov, const Vector& origin, const float& zNear, const float& zFar,
    const QAngle& angles, VMatrix& m_matView, VMatrix& m_matProj, VMatrix& m_matViewInv,
    VMatrix& m_matProjInv, VMatrix& m_matLockedViewProjInv, VMatrix& matStaticView,
    VMatrix& matStaticViewInv, float viewportOffsetX, float viewportOffsetY)
{
    if (data)
    {
        memcpy(&m_commonData, data, sizeof(m_commonData));
    }
    else
    {
        m_commonData.aspect = aspect;
        m_commonData.fov = fov;
        m_commonData.vecOrigin = Vector4D(origin.x, origin.y, origin.z, 1.0f);
        m_commonData.flZDists[0] = zNear;
        m_commonData.flZDists[1] = zFar;
        m_commonData.angles = angles;
        m_commonData.matView = m_matView;
        m_commonData.matProj = m_matProj;
        m_commonData.matViewInv = m_matViewInv;
        m_commonData.matProjInv = m_matProjInv;
        //m_commonData.matLockedViewProjInv = m_matLockedViewProjInv;
        m_commonData.matStaticView = matStaticView;
        m_commonData.matStaticViewInv = matStaticViewInv;
        m_commonData.viewportOffsetX = viewportOffsetX;
        m_commonData.viewportOffsetY = viewportOffsetY;
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

    light.position[0] = pos.x;
    light.position[1] = pos.y;
    light.position[2] = pos.z;
    light.position[3] = radius;

    light.color[0] = color.x * intensity;
    light.color[1] = color.y * intensity;
    light.color[2] = color.z * intensity;
    light.color[3] = (float)type; 

    SpotLight.direction[0] = dir.x;
    SpotLight.direction[1] = dir.y;
    SpotLight.direction[2] = dir.z;
    SpotLight.direction[3] = constantAtt; 

    SpotLight.attenuation[0] = linearAtt;
    SpotLight.attenuation[1] = quadraticAtt;
    SpotLight.attenuation[2] = 0.0f;
    SpotLight.attenuation[3] = spotCutoff; 

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

int CDeferredExtension::GetLightBufferSize()
{
    return m_vecForwardLightBuffer.Count();
}

ITexture* CDeferredExtension::GetTexture_WaterNormals()
{
    return m_pTexWaterNormals;
}

#include "..\..\public\deferred\str_light_regenerator.h"

//class CDeferredLightDataRegenerator : public ITextureRegenerator {
//public:
//    float data[8];
//
//    void RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect) override {
//        CMatRenderContextPtr pRenderContext(materials);
//
//        // Set up the rendertarget
//        pRenderContext->BindRenderTarget(pTexture);
//        pRenderContext->ClearColor4f(0.0f, 0.0f, 0.0f, 1.0f);
//        pRenderContext->DrawRectangle(0, 0, pTexture->GetActualWidth(), pTexture->GetActualHeight());
//
//        // Write pixel data
//        CPixelWriter writer;
//        imageformat_t fmt = pTexture->GetFormat();
//        if (fmt == IMAGE_FORMAT_RGBA32323232F) {
//            writer.SetPixelMemory(fmt, pTexture->GetBuffer(), pTexture->GetBufferSize());
//        }
//
//        // Write pixel data based on stored data
//        for (int i = 0; i < 2; ++i) {
//            writer.Seek(i, 0);
//            writer.WritePixel(
//                (uint8)(clamp(data[i * 4 + 0], 0, 1) * 255),
//                (uint8)(clamp(data[i * 4 + 1], 0, 1) * 255),
//                (uint8)(clamp(data[i * 4 + 2], 0, 1) * 255),
//                (uint8)(clamp(data[i * 4 + 3], 0, 1) * 255)
//            );
//        }
//    }
//
//    void Release() override { delete this; }
//
//    ITexture* m_pTex;
//};

uint8 CDeferredExtension::LightType(uint8 type)
{
    return type;
}

std::array<float, 8> CDeferredExtension::IndexTextureData(def_light_t* l)
{
    Vector pos = l->pos;
    Vector col_diffuse = l->col_diffuse;
    float radius = l->flRadius;
    uint8 type = l->iLighttype;

    std::array<float, 8> data = {
        pos.x, pos.y, pos.z,
        col_diffuse.x, col_diffuse.y, col_diffuse.z,
        radius,
        static_cast<float>(type)
    };

    return data;
}

void CDeferredExtension::InitIndexTexture()
{
    CDeferredLightDataRegenerator* m_pRegen = nullptr;
    m_pRegen = new CDeferredLightDataRegenerator();
    // potentially getting garbage data from writing to this then trying to read from it when regenerating directly below?
    m_pRegen->m_pTex = GetTexture_ForwardData();

    m_pRegen->RegenerateTextureBits(m_pRegen->m_pTex, nullptr, nullptr);
    
    //m_pIndexTexture->SetTextureRegenerator(m_pRegen);
    if (m_pIndexTexture != nullptr) {
        m_pIndexTexture->SetTextureRegenerator(m_pRegen);
    }
}

void CDeferredExtension::UpdateIndexTexture(def_light_t* l)
{
    if (!m_pIndexTexture || !m_pRegen)
        return;

    auto data = IndexTextureData(l);
    // !!
    memcpy(m_pRegen->data, data.data(), sizeof(float) * 8);

    /*int height = m_pIndexTexture->GetActualHeight();
    int width = m_pIndexTexture->GetActualWidth();

    Rect_t* rect = nullptr;
    rect = new Rect_t();

    rect->x = 0;
    rect->y = 0;
    rect->height = height;
    rect->width = width;*/

    m_pIndexTexture->Download();
}

ITexture* CDeferredExtension::IndexTexture()
{
    return m_pIndexTexture;
}

//struct Rect_t
//{
//    int x, y;
//    int width, height;
//};
//
//// This will be called when the texture bits need to be regenerated.
//// Use the VTFTexture interface, which has been set up with the
//// appropriate texture size + format
//// The rect specifies which part of the texture needs to be updated
//// You can choose to update all of the bits if you prefer
////virtual void RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect) = 0;
//
//// This will be called when the regenerator needs to be deleted
//// which will happen when the texture is destroyed
//virtual void Release() = 0;
//
//// Used to modify the texture bits (procedural textures only)
//virtual void SetTextureRegenerator(ITextureRegenerator* pTextureRegen, bool releaseExisting = true) = 0;
//
//// Reconstruct the texture bits in HW memory
//
//// If rect is not specified, reconstruct all bits, otherwise just
//// reconstruct a subrect.
//virtual void Download(Rect_t* pRect = 0) = 0;