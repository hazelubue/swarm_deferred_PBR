#include "cbase.h"

#ifndef CDEFERRED_MANAGER_SERVER_H
#define CDEFERRED_MANAGER_SERVER_H

extern ConVar r_deferred_autoenvlight_ambient_intensity_low;
extern ConVar r_deferred_autoenvlight_ambient_intensity_high;
extern ConVar r_deferred_autoenvlight_diffuse_intensity;

class CDeferredLight;

class CDeferredManagerServer : public CBaseGameSystem
{
public:

	CDeferredManagerServer();
	~CDeferredManagerServer();

	const char* Name() { return "DeferredManagerServer"; }

	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPreEntity();

	const char* MapEntity_ParseToken(const char* pEntData, char* token);
	const char* MapEntity_SkipToNextEntity(const char* pEntData, char* token);

	float ComputeLightRadius(const dworldlight_t& light);
	float ComputeLightRadius(float distance, int brightness, float constant_attn, float linear_attn, float quadratic_attn);

	int AddCookieTexture(const char* pszCookie);
	void AddWorldLight(CDeferredLight* l);
};


extern CDeferredManagerServer* GetDeferredManager();

#endif