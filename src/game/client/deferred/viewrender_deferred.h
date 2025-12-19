#ifndef VIEWRENDER_DEFERRED_H
#define VIEWRENDER_DEFERRED_H

#include "viewrender.h"

#include "deferred/deferred_shared_common.h"

extern void SetupCurrentView(const Vector& vecOrigin, const QAngle& angles, view_id_t viewID, bool bDrawWorldNormal = false, bool bCullFrontFaces = false);

class CDeferredViewRender : public CViewRender
{
	DECLARE_CLASS( CDeferredViewRender, CViewRender );

public:
					CDeferredViewRender();
	virtual			~CDeferredViewRender( void ) {}

	virtual void	Init( void );
	virtual void	Shutdown( void );

	virtual void	RenderView( const CViewSetup &view, const CViewSetup &hudViewSetup, int nClearFlags, int whatToDraw );

	CMaterialReference	m_SkydomeMaterial;
	CMaterialReference m_ForwardData;
	CDeferredViewRender* m_pDeferredView;

public:

	void			LevelInit( void );
	void			LevelShutdown( void );

	void			ResetCascadeDelay();

	void			ViewDrawSceneDeferred( const CViewSetup &view, int nClearFlags, view_id_t viewID,
		bool bDrawViewModel );

	void			ViewDrawForward(const CViewSetup& view, bool& bDrew3dSkybox,
		SkyboxVisibility_t& nSkyboxVisible, bool bDrawViewModel);

	void			ViewDrawGBuffer( const CViewSetup &view, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible,
		bool bDrawViewModel);

	void			ViewDrawGBufferWater(const CViewSetup& view, bool& bDrew3dSkybox, SkyboxVisibility_t& nSkyboxVisible,
		bool bDrawViewModel);

	void			ViewDrawGBufferTranslucent(const CViewSetup& view, bool& bDrew3dSkybox, SkyboxVisibility_t& nSkyboxVisible,
		bool bDrawViewModel);

	void			ViewDrawComposite( const CViewSetup &view, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible,
		int nClearFlags, view_id_t viewID, bool bDrawViewModel );

	void			DrawSkyboxComposite( const CViewSetup &view, const bool &bDrew3dSkybox );
	void			DrawWorldComposite( const CViewSetup &view, int nClearFlags, bool bDrawSkybox );

	void			DrawLightShadowView( const CViewSetup &view, int iDesiredShadowmap, def_light_t *l );

	void			DrawSky(const CViewSetup& view);
	void			SendForwardData();


	void ProcessDeferredGlobals(const CViewSetup& view);
	void ProcessGlobalMatrixData(const CViewSetup& view);
	
	void SetReflectionViewData(const Vector& origin, const QAngle& angles,
		float waterHeight)
	{
		m_vecReflectionViewOrigin = origin;
		m_angReflectionViewAngles = angles;
		m_flReflectionWaterHeight = waterHeight;
	}

	const Vector& GetReflectionViewOrigin() const { return m_vecReflectionViewOrigin; }
	const QAngle& GetReflectionViewAngles() const { return m_angReflectionViewAngles; }
	float GetReflectionWaterHeight() const { return m_flReflectionWaterHeight; }

	bool m_bReflectionDataValid;
protected:

	void			DrawViewModels( const CViewSetup &view, bool drawViewmodel, bool bGBuffer );

private:

	Vector m_vecReflectionViewOrigin;
	QAngle m_angReflectionViewAngles;
	float m_flReflectionWaterHeight;
	

	VMatrix GetViewProjMatrix(const CViewSetup& viewSetup);
	VMatrix GetViewMatrix(const Vector& pos, const QAngle& ang);
	VMatrix GetProjMatrix(const CViewSetup& viewSetup);

	void PerformLighting( const CViewSetup &view );

	void RenderCascadedShadows( const CViewSetup &view );

	float m_flRenderDelay[SHADOW_NUM_CASCADES];
};




#endif