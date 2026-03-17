//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ACHIEVEMENTMGR_H
#define ACHIEVEMENTMGR_H
#ifdef _WIN32
#pragma once
#endif
#include "matchmaking/imatchframework.h"
#include "baseachievement.h"
#include "GameEventListener.h"
#include "iachievementmgr.h"
#include "utlmap.h"
#include "steam/steam_api.h"

typedef void* AsyncHandle_t;

class CAchievementMgr : public CAutoGameSystemPerFrame, public CGameEventListener, public IMatchEventsSink, public IAchievementMgr
{
public:
	CAchievementMgr();

	virtual bool Init();
	virtual void PostInit();
	virtual void Shutdown();
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPreEntity();
	virtual void InitializeAchievements( );
	virtual void Update( float frametime );
	virtual bool IsPerFrame( void ) { return true; }

	void OnMapEvent( const char *pchEventName, int nUserSlot );

	// Interfaces exported to other dlls for achievement list queries
	IAchievement* GetAchievementByIndex( int index, int nUserSlot );
	IAchievement* GetAchievementByDisplayOrder( int orderIndex, int nUserSlot );
	int GetAchievementCount();

	CBaseAchievement *GetAchievementByID( int iAchievementID, int nUserSlot );
	CUtlMap<int, CBaseAchievement *> &GetAchievements( int nUserSlot ) { return m_mapAchievement; }

	CBaseAchievement *GetAchievementByName( const char *pchName, int nUserSlot );

	void UploadUserData( int nUserSlot );
	void UserConnected( int nUserSlot );
	void UserDisconnected( int nUserSlot );
	bool IsUserConnected( int nUserSlot ) { return m_bUserSlotActive; }
	void ReadAchievementsFromTitleData( int iController, int iSlot );
	void SaveGlobalState();
	void SaveGlobalStateIfDirty();
	bool HasAchieved( const char *pchName, int nUserSlot );
	void AwardAchievement( int iAchievementID, int nUserSlot );
	void UpdateAchievement( int iAchievementID, int nData, int nUserSlot );
	void PreRestoreSavedGame();
	void PostRestoreSavedGame();
	void ResetAchievements();
	void ResetAchievement( int iAchievementID );
	void PrintAchievementStatus();
	float GetLastClassChangeTime( int nUserSlot ) { return m_flLastClassChangeTime; }
	float GetTeamplayStartTime( int nUserSlot ) { return m_flTeamplayStartTime; }
	int	  GetMiniroundsCompleted( int nUserSlot ) { return m_iMiniroundsCompleted; }
	const char *GetMapName() { return m_szMap; }
	void OnAchievementEvent( int iAchievementID, int nUserSlot );
	void SetDirty( bool bDirty, int nUserSlot ) { m_bDirty = bDirty; }
	bool CheckAchievementsEnabled();

	// IMatchEventsSink
	virtual void OnEvent( KeyValues *pEvent );

#ifdef CLIENT_DLL
	bool LoggedIntoSteam() { return ( steamapicontext->SteamUser() && steamapicontext->SteamUserStats() && steamapicontext->SteamUser()->BLoggedOn() ); }
#else
	bool LoggedIntoSteam() { return false; }
#endif

	float GetTimeLastUpload() { return m_flTimeLastUpload; }			// time we last uploaded to Steam
	bool WereCheatsEverOn( void ) { return m_bCheatsEverOn; }

#if !defined(NO_STEAM)
	STEAM_CALLBACK( CAchievementMgr, Steam_OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CAchievementMgr, Steam_OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored );
#endif
	const CUtlVector<int>& GetAchievedDuringCurrentGame( int nPlayerSlot );
	void ResetAchievedDuringCurrentGame( int nPlayerSlot );
	virtual void FireGameEvent( IGameEvent *event );

private:
	void OnKillEvent( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event );
	void ResetAchievement_Internal( CBaseAchievement *pAchievement );

	CUtlMap<int, CBaseAchievement *> m_mapAchievement;			// map of all achievements
	CUtlVector<CBaseAchievement *>	 m_vecAchievement ;			// vector of all achievements for accessing by index
	CUtlVector<CBaseAchievement *> m_vecKillEventListeners ;	// vector of achievements that are listening for kill events
	CUtlVector<CBaseAchievement *> m_vecMapEventListeners ;		// vector of achievements that are listening for map events
	CUtlVector<CBaseAchievement *> m_vecComponentListeners ;	// vector of achievements that are listening for components that make up an achievement
	CUtlVector<CBaseAchievement *>	 m_vecAchievementInOrder ;	// vector of all achievements for accessing by display order

	float m_flLevelInitTime ;
	float m_flLastClassChangeTime ;		// Time when player last changed class
	float m_flTeamplayStartTime ;		// Time when player joined a non-spectating team.  Not updated if she switches game teams; cleared if she joins spectator
	float m_iMiniroundsCompleted ;		// # of minirounds played since game start (for maps that have minirounds)
	char  m_szMap[MAX_PATH];										// file base of map name, cached since we access it frequently in this form
	bool  m_bDirty ;					// do we have interesting state that needs to be saved
	bool  m_bUserSlotActive ;
	bool  m_bCheatsEverOn;				// have cheats ever been turned on in this level
	float m_flTimeLastUpload;			// last time we uploaded to Steam
	float  m_flWaitingForStoreStatsCallback;
	bool  m_bCallStoreStatsAfterCallback;

#ifdef _X360 
	struct PendingAchievementInfo_t {
		int nAchievementID;					// Achievement we're waiting to check the status of
		int nUserSlot;						// Which user is being awarded the achievement
		AsyncHandle_t pOverlappedResult;	// Result we'll check again
	};

	CUtlVector<PendingAchievementInfo_t>	m_pendingAchievementState;
#endif

	CUtlVector<int> m_AchievementsAwarded;
	CUtlVector<int> m_AchievementsAwardedDuringCurrentGame;
	void ClearAchievementData( int nUserSlot );
};

// helper functions
const char *GetModelName( CBaseEntity *pBaseEntity );

#ifdef CLIENT_DLL
bool CalcPlayersOnFriendsList( int iMinPlayers );
bool CalcHasNumClanPlayers( int iClanTeammates );
int	CalcPlayerCount();
int	CalcTeammateCount();
#endif // CLIENT

extern ConVar	cc_achievement_debug;

#ifdef CLIENT_DLL
void MsgFunc_AchievementEvent( bf_read &msg );
#endif // CLIENT_DLL
#endif // ACHIEVEMENTMGR_H
