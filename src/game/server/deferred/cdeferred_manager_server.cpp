#include "cbase.h"

#include "deferred/deferred_shared_common.h"
#include "filesystem.h"
#include "bspfile.h"
#include "utlvector.h"
#include "KeyValues.h"
#include "tier1/strtools.h"
#include "mapentities_shared.h"
#include "networkstringtabledefs.h"

// Forward declarations for deferred lighting classes  
class CDeferredLightGlobal;
class CDeferredLightContainer;

// Declare the string table
extern INetworkStringTable* g_pStringTable_LightCookies;

ConVar r_deferred_autoenvlight_ambient_intensity_low("r_deferred_autoenvlight_ambient_intensity_low", "1.0", FCVAR_NONE, "Low ambient intensity for auto environment light");
ConVar r_deferred_autoenvlight_ambient_intensity_high("r_deferred_autoenvlight_ambient_intensity_high", "1.0", FCVAR_NONE, "High ambient intensity for auto environment light");
ConVar r_deferred_autoenvlight_diffuse_intensity("r_deferred_autoenvlight_diffuse_intensity", "1.0", FCVAR_NONE, "Diffuse intensity for auto environment light");

// Helper function to parse color strings into int arrays
static void UTIL_StringToIntArray(int* pVector, int count, const char* pString)
{
	char* pstr, * pfront, tempString[128];
	int	j;

	Q_strncpy(tempString, pString, sizeof(tempString));
	pstr = pfront = tempString;

	for (j = 0; j < count; j++)
	{
		pVector[j] = atoi(pfront);

		while (*pstr && *pstr != ' ')
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}
	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}

static CDeferredManagerServer g_DeferredManagerServer;

CDeferredManagerServer* GetDeferredManager()
{
	return &g_DeferredManagerServer;
}

CDeferredManagerServer::CDeferredManagerServer()
{
}

CDeferredManagerServer::~CDeferredManagerServer()
{
}

bool CDeferredManagerServer::Init()
{
	return true;
}

void CDeferredManagerServer::Shutdown()
{
}

const char* CDeferredManagerServer::MapEntity_ParseToken(const char* pEntData, char* token)
{
	return ::MapEntity_ParseToken(pEntData, token);
}

const char* CDeferredManagerServer::MapEntity_SkipToNextEntity(const char* pEntData, char* token)
{
	return ::MapEntity_SkipToNextEntity(pEntData, token);
}

float CDeferredManagerServer::ComputeLightRadius(const dworldlight_t& light)
{
	const float maxIntensity = MAX(MAX(light.intensity.x, light.intensity.y), light.intensity.z);
	const float radius = sqrt(maxIntensity * 1000.0f);
	return radius;
}

// ComputeLightRadius implementation for parameters
float CDeferredManagerServer::ComputeLightRadius(float distance, int brightness, float constant_attn, float linear_attn, float quadratic_attn)
{

	const float maxIntensity = brightness / 255.0f;
	const float threshold = maxIntensity / 256.0f;

	if (distance > 0)
		return distance;

	// Fallback calculation
	return sqrt(maxIntensity * 1000.0f);
}

void CDeferredManagerServer::LevelInitPreEntity()
{
	if (gpGlobals->eLoadType == MapLoad_LoadGame)
		return;

	if (CommandLine() && CommandLine()->FindParm("-nodeferred") != 0)
		return;

	const char* entStr = engine->GetMapEntitiesString();

	if (V_stristr(entStr, "light_deferred_global"))
		return;

	// Get the full map path
	char szMapPath[MAX_PATH];
	char szMapName[64];
	Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName));
	Q_snprintf(szMapPath, sizeof(szMapPath), "maps/%s.bsp", szMapName);

	FileHandle_t hFile = filesystem->Open(szMapPath, "rb", "GAME");
	if (!hFile)
		return;

	BSPHeader_t header;
	filesystem->Read(&header, sizeof(BSPHeader_t), hFile);

	const lump_t& lightLump = header.lumps[LUMP_WORLDLIGHTS];
	dworldlight_t* lights = NULL;
	int lightCount = 0;

	// ASW BSP doesn't use compressed lumps typically
	if (lightLump.filelen == 0)
	{
		filesystem->Close(hFile);
		return;
	}

	if (lightLump.filelen % sizeof(dworldlight_t))
	{
		filesystem->Close(hFile);
		return;
	}

	filesystem->Seek(hFile, lightLump.fileofs, FILESYSTEM_SEEK_HEAD);

	lightCount = lightLump.filelen / sizeof(dworldlight_t);
	lights = new dworldlight_t[lightCount];
	filesystem->Read(lights, lightLump.filelen, hFile);

	filesystem->Close(hFile);

	KeyValues* vmfFile = new KeyValues("vmf");

	char szTokenBuffer[MAPKEY_MAXLENGTH];
	const char* pEntStr = entStr;
	for (; true; pEntStr = MapEntity_SkipToNextEntity(pEntStr, szTokenBuffer))
	{
		char token[MAPKEY_MAXLENGTH];
		pEntStr = MapEntity_ParseToken(pEntStr, token);

		if (!pEntStr)
			break;

		if (token[0] != '{')
		{
			Error("MapEntity_ParseAllEntities: found %s when expecting {", token);
			continue;
		}

		// Calculate the size of this entity block
		const char* pBlockStart = pEntStr;
		int nBraceCount = 1;
		const char* pBlockEnd = pBlockStart;
		while (nBraceCount > 0 && *pBlockEnd)
		{
			if (*pBlockEnd == '{')
				nBraceCount++;
			else if (*pBlockEnd == '}')
				nBraceCount--;
			pBlockEnd++;
		}
		int nBlockSize = pBlockEnd - pBlockStart;

		CEntityMapData entData(const_cast<char*>(pBlockStart), nBlockSize);

		char className[MAPKEY_MAXLENGTH];
		if (!entData.ExtractValue("classname", className))
			continue;

		int iType = -1;
		if (FStrEq(className, "light"))
			iType = 0;
		else if (FStrEq(className, "light_spot"))
			iType = 1;
		else if (FStrEq(className, "point_spotlight"))
			iType = 2;
		else if (FStrEq(className, "light_environment"))
			iType = 3;
		else
			continue;

		char keyName[MAPKEY_MAXLENGTH];
		char value[MAPKEY_MAXLENGTH];
		KeyValues* pSubKey = new KeyValues(className);
		if (entData.GetFirstKey(keyName, value))
		{
			do
			{
				pSubKey->SetString(keyName, value);
			} while (entData.GetNextKey(keyName, value));
		}
		pSubKey->SetInt("light_type", iType);
		vmfFile->AddSubKey(pSubKey);
	}

	struct SpotLightPair_t
	{
		KeyValues* spotlight;
		KeyValues* light;
	};

	CUtlVector<SpotLightPair_t> spotLightPairs;

	// Find light_spot and point_spotlight entities at same place
	FOR_EACH_SUBKEY(vmfFile, entity1)
	{
		const int type1 = entity1->GetInt("light_type", -1);
		if (type1 != 1)
			continue;
		bool bSkip = false;
		const int numPairs = spotLightPairs.Count();
		for (int i = 0; i < numPairs; ++i)
		{
			const SpotLightPair_t& pair = spotLightPairs[i];
			if (pair.light == entity1)
			{
				bSkip = true;
				break;
			}
		}
		if (bSkip)
			continue;

		Vector pos1;
		QAngle rot1;
		UTIL_StringToVector(pos1.Base(), entity1->GetString("origin"));
		UTIL_StringToVector(rot1.Base(), entity1->GetString("angles"));

		FOR_EACH_SUBKEY(vmfFile, entity2)
		{
			const int type2 = entity2->GetInt("light_type", -1);
			if (type2 != 2)
				continue;

			if (entity1 == entity2)
				continue;

			bool bSkip2 = false;
			const int numPairs2 = spotLightPairs.Count();
			for (int i = 0; i < numPairs2; ++i)
			{
				const SpotLightPair_t& pair = spotLightPairs[i];
				if (pair.spotlight == entity2)
				{
					bSkip2 = true;
					break;
				}
			}
			if (bSkip2)
				continue;

			Vector pos2;
			QAngle rot2;
			UTIL_StringToVector(pos2.Base(), entity2->GetString("origin"));
			UTIL_StringToVector(rot2.Base(), entity2->GetString("angles"));

			// Fixed bug: was comparing pos1.z to pos1.z
			if (CloseEnough(pos1.x, pos2.x, 2.f) && CloseEnough(pos1.y, pos2.y, 2.f) &&
				CloseEnough(pos1.z, pos2.z, 2.f) && CloseEnough(rot1.y, rot2.y, 2.f) &&
				CloseEnough(rot1.z, rot2.z, 2.f))
			{
				DevMsg("Found matching lights at positions %.1f %.1f %.1f and %.1f %.1f %.1f of types %d and %d\n",
					pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, type1, type2);
				int idx = spotLightPairs.AddToTail();
				SpotLightPair_t& pair = spotLightPairs[idx];
				pair.light = type1 == 1 ? entity1 : entity2;
				pair.spotlight = type1 != 1 ? entity1 : entity2;
				break;
			}
		}
	}

#define COPY_LIGHT_DATA(name) pair.spotlight->SetString(name, pair.light->GetString(name))

	const int numPairs = spotLightPairs.Count();
	for (int i = 0; i < numPairs; ++i)
	{
		SpotLightPair_t& pair = spotLightPairs[i];
		vmfFile->RemoveSubKey(pair.light);
		COPY_LIGHT_DATA("_light");
		COPY_LIGHT_DATA("_exponent");
		COPY_LIGHT_DATA("_cone");
		COPY_LIGHT_DATA("_inner_cone");
		COPY_LIGHT_DATA("_constant_attn");
		COPY_LIGHT_DATA("_linear_attn");
		COPY_LIGHT_DATA("_quadratic_attn");
		pair.spotlight->SetInt("light_type", 5);
		pair.light->deleteThis();
	}

	bool bCreatedGlobalLight = false;

	FOR_EACH_SUBKEY(vmfFile, entity)
	{
		const int type = entity->GetInt("light_type", -1);
		if (type == -1 || (type == 3 && bCreatedGlobalLight))
			continue;

		Vector pos(0, 0, 0);
		QAngle rot(0, 0, 0);
		UTIL_StringToVector(pos.Base(), entity->GetString("origin"));

		KeyValues* angle = entity->FindKey("angles");
		if (angle)
		{
			UTIL_StringToVector(rot.Base(), angle->GetString());
			if (type == 1 && entity->GetInt("pitch") != 0)
				rot.x = -entity->GetInt("pitch");
		}

		if (type == 3)
		{
			rot.x = -entity->GetInt("pitch");
			CBaseEntity* pEnt = CreateEntityByName("light_deferred_global");
			CDeferredLightGlobal* lightEntity = dynamic_cast<CDeferredLightGlobal*>(pEnt);
			if (!lightEntity)
				break;

			lightEntity->SetAbsOrigin(pos);
			lightEntity->SetAbsAngles(rot);

			int color[4] = { 255, 255, 255, 255 };
			int ambient[4] = { 255, 255, 255, 255 };
			UTIL_StringToIntArray(color, 4, entity->GetString("_light"));
			UTIL_StringToIntArray(ambient, 4, entity->GetString("_ambient"));

			const float ds = r_deferred_autoenvlight_diffuse_intensity.GetFloat();
			const float asl = r_deferred_autoenvlight_ambient_intensity_low.GetFloat();
			const float ash = r_deferred_autoenvlight_ambient_intensity_high.GetFloat();

			char szValue[256];
			Q_snprintf(szValue, sizeof(szValue), "%d %d %d %f", color[0], color[1], color[2], color[3] * ds);
			lightEntity->KeyValue("diffuse", szValue);

			Q_snprintf(szValue, sizeof(szValue), "%d %d %d %f", ambient[0], ambient[1], ambient[2], ambient[3] * ash);
			lightEntity->KeyValue("ambient_high", szValue);

			Q_snprintf(szValue, sizeof(szValue), "%d %d %d %f", ambient[0], ambient[1], ambient[2], ambient[3] * asl);
			lightEntity->KeyValue("ambient_low", szValue);

			lightEntity->KeyValue("spawnflags", "3");

			DispatchSpawn(lightEntity);

			bCreatedGlobalLight = true;
			continue;
		}

		// Match with world lights
		for (int i = 0; i < lightCount; ++i)
		{
			const dworldlight_t& light = lights[i];
			if (light.type != emit_spotlight && light.type != emit_point)
				continue;

			if (light.type == emit_point)
			{
				entity->SetFloat("_distance", ComputeLightRadius(light));
				break;
			}

			QAngle ang;
			VectorAngles(light.normal, ang);
			if (CloseEnough(-rot.x, ang.x, 2.f) && CloseEnough(rot.y, ang.y, 2.f))
			{
				rot = ang;
				entity->SetFloat("_inner_cone", acos(light.stopdot) * 180.f / M_PI_F);
				entity->SetFloat("_cone", acos(light.stopdot2) * 180.f / M_PI_F);
				entity->SetFloat("_distance", ComputeLightRadius(light));
				break;
			}
		}

		CBaseEntity* pEnt = CreateEntityByName("light_deferred");
		CDeferredLight* lightEntity = dynamic_cast<CDeferredLight*>(pEnt);
		if (!lightEntity)
			break;

		lightEntity->SetAbsOrigin(pos);
		lightEntity->SetAbsAngles(rot);

		int color[4] = { 255, 255, 255, 255 };
		UTIL_StringToIntArray(color, 4, entity->GetString(type == 2 ? "rendercolor" : "_light"));
		if (type == 5)
			color[3] = 255;

		char szValue[256];
		Q_snprintf(szValue, sizeof(szValue), "%d %d %d %d", color[0], color[1], color[2], color[3]);
		lightEntity->KeyValue("def_diffuse", szValue);

		lightEntity->KeyValue("spawnflags", (type == 2 || type == 5) ? "11" : "3");

		if (type == 1 || type == 5)
		{
			lightEntity->KeyValue("def_lighttype", "1");
			Q_snprintf(szValue, sizeof(szValue), "%f", entity->GetFloat("_inner_cone"));
			lightEntity->KeyValue("def_spotcone_inner", szValue);
			Q_snprintf(szValue, sizeof(szValue), "%f", entity->GetFloat("_cone"));
			lightEntity->KeyValue("def_spotcone_outer", szValue);
			Q_snprintf(szValue, sizeof(szValue), "%f", entity->GetFloat("_exponent", 1.f));
			lightEntity->KeyValue("def_power", szValue);
			if (type == 5)
				lightEntity->KeyValue("def_volume_samples", "50");
		}
		else if (type == 2)
		{
			lightEntity->KeyValue("def_lighttype", "1");
			const float width = entity->GetFloat("spotlightwidth");
			Q_snprintf(szValue, sizeof(szValue), "%f", width);
			lightEntity->KeyValue("def_spotcone_inner", szValue);
			Q_snprintf(szValue, sizeof(szValue), "%f", width * 1.5f);
			lightEntity->KeyValue("def_spotcone_outer", szValue);
			lightEntity->KeyValue("def_power", "1");
			lightEntity->KeyValue("def_volume_samples", "50");
		}
		else
		{
			lightEntity->KeyValue("def_lighttype", "0");
			lightEntity->KeyValue("def_power", "1");
		}

		const float radius = type == 2 ? entity->GetFloat("spotlightlength") :
			ComputeLightRadius(entity->GetFloat("_distance"), color[3],
				entity->GetFloat("_constant_attn"), entity->GetFloat("_linear_attn"),
				entity->GetFloat("_quadratic_attn"));

		Q_snprintf(szValue, sizeof(szValue), "%f", radius);
		lightEntity->KeyValue("def_radius", szValue);
		Q_snprintf(szValue, sizeof(szValue), "%f", radius * 2);
		lightEntity->KeyValue("def_vis_dist", szValue);
		Q_snprintf(szValue, sizeof(szValue), "%f", radius * 1.25f);
		lightEntity->KeyValue("def_vis_range", szValue);
		Q_snprintf(szValue, sizeof(szValue), "%f", radius * (5.f / 6.f));
		lightEntity->KeyValue("def_shadow_dist", szValue);
		Q_snprintf(szValue, sizeof(szValue), "%f", radius * (2.f / 3.f));
		lightEntity->KeyValue("def_shadow_range", szValue);

		KeyValues* targetname = entity->FindKey("targetname");
		if (targetname)
			lightEntity->KeyValue("targetname", targetname->GetString());

		KeyValues* parent = entity->FindKey("parentname");
		if (parent)
			lightEntity->KeyValue("parentname", parent->GetString());

		DispatchSpawn(lightEntity);
	}

	delete[] lights;
	vmfFile->deleteThis();
}


int CDeferredManagerServer::AddCookieTexture(const char* pszCookie)
{
	Assert(g_pStringTable_LightCookies != NULL);
	return g_pStringTable_LightCookies->AddString(true, pszCookie);
}

void CDeferredManagerServer::AddWorldLight(CDeferredLight* l)
{
	CDeferredLightContainer* pC = FindAvailableContainer();

	if (!pC)
	{
		CBaseEntity* pEnt = CreateEntityByName("deferred_light_container");
		pC = dynamic_cast<CDeferredLightContainer*>(pEnt);
	}

	if (pC)
		pC->AddWorldLight(l);
}