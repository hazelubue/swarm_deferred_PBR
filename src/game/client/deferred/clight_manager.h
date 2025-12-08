#ifndef C_LIGHT_MANAGER_H
#define C_LIGHT_MANAGER_H

#include "cbase.h"

class CViewSetup;
class CDeferredViewRender;

//struct ForwardLightData
//{
//	float position[4];      // xyz = position, w = radius
//	float color[4];         // xyz = color, w = intensity  
//	float direction[4];     // xyz = direction, w = type (0=point, 1=spot, 2=directional)
//	float attenuation[4];   // x = constant, y = linear, z = quadratic, w = spotCutoff
//};

class CLightingManager : public CAutoGameSystemPerFrame
{
	typedef CAutoGameSystemPerFrame BaseClass;
public:

	CLightingManager();
	~CLightingManager();

	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();

	virtual void Update( float ft );

	void SetRenderWorldLights( bool bRender );

	void LightSetup( const CViewSetup &setup );
	void LightTearDown();
	void SetRenderConstants( const VMatrix &ScreenToWorld,
		const CViewSetup &setup );

	void OnCookieStringReceived( const char *pszString, const int &index );
	void OnMaterialReload();

	void AddLight( def_light_t *l );
	bool RemoveLight( def_light_t *l );
	bool IsLightRendered( def_light_t *l );

	void ClearTmpLists();

	// update volatile data - internal xforms, final light col bleh
	void PrepareLights();

	// initialize the render list
	void CullLights();

	// determine which lights to draw fullscreen or per world projection
	void SortLights();

	// draw all lights set up for rendering
	void RenderLights( const CViewSetup &view, CDeferredViewRender *pCaller );

	// add volumes to scene after composition
	void RenderVolumetrics( const CViewSetup &view );

	// debugging crap
	void DoSceneDebug();
	void DebugLights_Draw_Boundingboxes();
#if DEBUG
	void DebugLights_Draw_DebugMeshes();
#endif

	void CollectForwardLights();
	void CommitForwardLightsToExtension();
	int GetNumForwardLights() const { return m_vecForwardLights.Count(); }
	const ForwardLightData* GetForwardLightData(int index) const;

	const CUtlVector<def_light_t*>& GetRenderLights() const { return m_hRenderLights; }
private:

	CUtlVector<ForwardLightData> m_vecForwardLights;
	CUtlVector<float> m_vecForwardLightBuffer;
	bool m_bForwardLightsDirty;

	bool m_bDrawWorldLights;

	enum
	{
		LSORT_POINT_WORLD = 0,
		LSORT_POINT_FULLSCREEN,
		LSORT_SPOT_WORLD,
		LSORT_SPOT_FULLSCREEN,

		LSORT_COUNT,
	};

	CUtlVector< def_light_t* > m_hDeferredLights;

	CUtlVector< def_light_t* > m_hRenderLights;
	CUtlVector< def_light_t* > m_hPreSortedLights[ LSORT_COUNT ];

	VMatrix m_matScreenToWorld;
	Vector m_vecViewOrigin;
	Vector m_vecForward;
	float m_flzNear;
	bool m_bDrawVolumetrics;

	FORCEINLINE float DoLightStyle( def_light_t *l );
	FORCEINLINE int WriteLight( def_light_t *l, float *pfl4 );
	FORCEINLINE void DrawVolumePrepass( bool bDoModelTransform, const CViewSetup &view, def_light_t *l );

#if DEBUG
	bool m_bVolatileLists;
#endif
};

extern CLightingManager *GetLightingManager();

#endif