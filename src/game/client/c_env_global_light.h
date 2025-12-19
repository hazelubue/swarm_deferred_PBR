#ifndef C_ENV_GLOBAL_LIGHT_H
#define C_ENV_GLOBAL_LIGHT_H

//------------------------------------------------------------------------------
// Purpose : Sunlights shadow control entity
//------------------------------------------------------------------------------
class C_GlobalLight : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_GlobalLight, C_BaseEntity);

	DECLARE_CLIENTCLASS();

	C_GlobalLight();
	virtual ~C_GlobalLight();

	// Static accessor - returns the active instance
	static C_GlobalLight* GetCSMLight();

	// Add these missing methods from your .cpp file
	bool IsCascadedShadowMappingEnabled() const;
	bool IsVolumetricsEnabled() const;
	bool IsDynamicSkyEnabled() const;
	bool UsesTimeForAngles() const;
	float DayNightTimescale() const;
	float CurrentTime() const;

	void OnDataChanged(DataUpdateType_t updateType);
	void Spawn();
	bool ShouldDraw();
	void ClientThink();

	// Public member variables
	bool m_bEnabled;

	// Add these public members that are used in your .cpp file
	Vector m_shadowDirection;
	Vector m_LinearFloatLightColor;
	Vector m_LinearFloatAmbientColor;
	QAngle m_angSunAngles;
	Vector m_vecLight;
	Vector m_vecAmbient;
	bool m_bCascadedShadowMappingEnabled;
	bool m_bEnableVolumetrics;
	bool m_bEnableDynamicSky;
	bool m_bEnableTimeAngles;
	float m_flDayNightTimescale;
	float m_fTime;
	float m_flColorTransitionTime;
	float m_flSunDistance;
	float m_flFOV;
	float m_flNearZ;
	float m_flNorthOffset;
	bool m_bEnableShadows;
	bool m_bOldEnableShadows;

	static C_GlobalLight* s_pCSMLight;
	static C_GlobalLight* m_pCachedCSMLight;
private:
	// Private member variables
	char m_TextureName[MAX_PATH];
	CTextureReference m_SpotlightTexture;
	color32	m_LightColor;
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;


	

	static ClientShadowHandle_t m_LocalFlashlightHandle;
};

extern C_GlobalLight* s_pCSMLight;
extern C_GlobalLight* m_pCachedCSMLight;

// Helper function for external access
float GetCSMCurrentTime();

#endif