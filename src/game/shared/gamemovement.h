//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( GAMEMOVEMENT_H )
#define GAMEMOVEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamemovement.h"
#include "cmodel.h"
#include "tier0/vprof.h"

#define CTEXTURESMAX		512			// max number of textures loaded
#define CBTEXTURENAMEMAX	13			// only load first n chars of name

#define GAMEMOVEMENT_DUCK_TIME				1000		// ms
#define GAMEMOVEMENT_JUMP_TIME				2000			// ms approx - based on the 21 unit height jump
#define GAMEMOVEMENT_JUMP_HEIGHT			21.0f		// units
#define GAMEMOVEMENT_TIME_TO_UNDUCK_MSECS			( TIME_TO_UNDUCK_MSECS )		// ms
#define GAMEMOVEMENT_TIME_TO_UNDUCK_MSECS_INV		( GAMEMOVEMENT_DUCK_TIME - GAMEMOVEMENT_TIME_TO_UNDUCK_MSECS )

#define WALLRUN_MAX_Z 0.5

struct surfacedata_t;

class CBasePlayer;

//CGameMovement* plyr_r2;

class CGameMovement : public IGameMovement
{
public:
	DECLARE_CLASS_NOBASE( CGameMovement );
	
	CGameMovement( void );
	virtual			~CGameMovement( void );

	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove, CGameMovement* g_pMovement);
	virtual void	Reset( void );
	virtual void	StartTrackPredictionErrors( CBasePlayer *pPlayer );
	virtual void	FinishTrackPredictionErrors( CBasePlayer *pPlayer );
	virtual void	DiffPrint( char const *fmt, ... );
	virtual const Vector&	GetPlayerMins( bool ducked ) const;
	virtual const Vector&	GetPlayerMaxs( bool ducked ) const;
	virtual const Vector&	GetPlayerViewOffset( bool ducked ) const;
	virtual void SetupMovementBounds( CMoveData *pMove );

	virtual bool		IsMovingPlayerStuck( void ) const;
	virtual CBasePlayer *GetMovingPlayer( void ) const;
	virtual void		UnblockPusher( CBasePlayer *pPlayer, CBaseEntity *pPusher );

// For sanity checking getting stuck on CMoveData::SetAbsOrigin
	virtual void			TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );

	// wrapper around tracehull to allow tracelistdata optimizations
	void			GameMovementTraceHull( const Vector& start, const Vector& end, const Vector &mins, const Vector &maxs, unsigned int fMask, ITraceFilter *pFilter, trace_t *pTrace );

#define BRUSH_ONLY true
	virtual unsigned int PlayerSolidMask( bool brushOnly = false, CBasePlayer *testPlayer = NULL ) const;	///< returns the solid mask for the given player, so bots can have a more-restrictive set
	CBasePlayer		*player;
	CMoveData *GetMoveData() { return mv; }
protected:
	// Input/Output for this movement
	CMoveData		*mv;

	
	int				m_nOldWaterLevel;
	float			m_flWaterEntryTime;

	float			m_flTimeLastJumped;
	float			m_nLurchTimer;

	int				m_nOnLadder;

	Vector			m_vecForward;
	Vector			m_vecRight;
	Vector			m_vecUp;


	// Does most of the player movement logic.
	// Returns with origin, angles, and velocity modified in place.
	// were contacted during the move.
	virtual void	PlayerMove(CGameMovement* g_pMovement, CBasePlayer* g_pPlayer);

	// Set ground data, etc.
	void			FinishMove( void );

	virtual float	CalcRoll( const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed );

	virtual	void	DecayPunchAngle( void );

	virtual void	CheckWaterJump(void );

	virtual void	WaterMove( void );

	virtual void	WaterJump( void );

	// Handles both ground friction and water friction
	virtual void	Friction( void );
	// Special friction for powersliding
	
	void            PowerSlideFriction(void);

	virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	virtual void	AirMove(CGameMovement* pMovement, CBasePlayer* pPlayer);

	virtual void	PerformLurch();
	
	virtual bool	CanAccelerate();
	virtual void	Accelerate( Vector& wishdir, float wishspeed, float accel);

	// Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
	virtual void	WalkMove( void );

	// Try to keep a walking player on the ground when running down slopes etc
	virtual void	StayOnGround( void );

	// Check if only touching wall with head/upper body
	void            CheckFeetCanReachWall(void);

	// Handle MOVETYPE_WALK.
	virtual void	FullWalkMove(CGameMovement* g_pMovement, CBasePlayer* pPlayer);

	// Implement this if you want to know when the player collides during OnPlayerMove
	virtual void	OnTryPlayerMoveCollision( trace_t &tr ) {}

	virtual const Vector&	GetPlayerMins( void ) const; // uses local player
	virtual const Vector&	GetPlayerMaxs( void ) const; // uses local player

	typedef enum
	{
		GROUND = 0,
		STUCK,
		LADDER,
		LADDER_WEDGE
	} IntervalType_t;

	virtual int		GetCheckInterval( IntervalType_t type );

	// Useful for things that happen periodically. This lets things happen on the specified interval, but
	// spaces the events onto different frames for different players so they don't all hit their spikes
	// simultaneously.
	bool			CheckInterval( IntervalType_t type );


	// Decompoosed gravity
	virtual void	StartGravity( void );
	virtual void	FinishGravity( void );

	// Apply normal ( undecomposed ) gravity
	virtual void	AddGravity( void );

	// Handle movement in noclip mode.
	void			FullNoClipMove( float factor, float maxacceleration );

	// Returns true if he started a jump (ie: should he play the jump animation)?
	virtual bool	CheckJumpButton();	// Overridden by each game.

	// Check if player should powerslide
	// Called when we duck or land on the ground while ducked
	virtual void    CheckPowerSlide(void);

	// End powerslide - reset the vars, stop the sound
	virtual void    EndPowerSlide(void);

	virtual void    AnticipateWallRun(void);

	virtual bool    CheckForSteps(const Vector& startpos, const Vector& vel);

	// Get the yaw angle between the player and the wall normal
	virtual float   GetWallRunYaw(void);

	// Check if player should start wallrunning,
	// i.e. hit a suitable wall while airborn.
	virtual void    CheckWallRun(Vector& vecWallNormal, trace_t& pm);

	// Check if player can scramble up on top of obstacle
	virtual void    CheckWallRunScramble(bool& steps);

	// Calculate the wallrun view roll angle based on the 
	// yaw angle between the player and the wall
	virtual float   GetWallRunRollAngle(void);

	// Handle wallrun movement
	virtual void    WallRunMove(void);

	// Handle step-like bits of the wall when wallrunning
	// (basically step move except wallnorm instead of up)
	virtual void	WallRunAnticipateBump(void);

	// Try not to get stuck moving in to a corner that is small
	// and we could easily step around it.
	virtual void    WallRunEscapeCorner(Vector& wishdir);
	virtual bool    TryEscape(Vector& posD, float rotation, Vector move);

	// Handle end of wallrun - set vars, stop sound
	virtual void    EndWallRun(void);

	// Dead player flying through air., e.g.
	virtual void    FullTossMove( void );
	
	// Player is a Observer chasing another player
	void			FullObserverMove( void );

	// Handle movement when in MOVETYPE_LADDER mode.
	virtual void	FullLadderMove(CGameMovement* pMovement);

	// The basic solid body movement clip that slides along multiple planes
	virtual int		TryPlayerMove( Vector *pFirstDest=NULL, trace_t *pFirstTrace=NULL );
	
	virtual bool	LadderMove( void );
	virtual bool	OnLadder( trace_t &trace );
	virtual float	LadderDistance( void ) const { return 2.0f; }	///< Returns the distance a player can be from a ladder and still attach to it
	virtual unsigned int LadderMask( void ) const { return MASK_PLAYERSOLID; }
	virtual float	ClimbSpeed( void ) const { return MAX_CLIMB_SPEED; }
	virtual float	LadderLateralMultiplier( void ) const { return 1.0f; }

	// See if the player has a bogus velocity value.
	void			CheckVelocity( void );

	// Does not change the entities velocity at all
	void			PushEntity( Vector& push, trace_t *pTrace );

	// Slide off of the impacting object
	// returns the blocked flags:
	// 0x01 == floor
	// 0x02 == step / wall
	virtual int		ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce );

	// If pmove.origin is in a solid position,
	// try nudging slightly on all axis to
	// allow for the cut precision of the net coordinates

	int				CheckStuck( void );
	
	// Check if the point is in water.
	// Sets refWaterLevel and refWaterType appropriately.
	// If in water, applies current to baseVelocity, and returns true.
	virtual bool	CheckWater( void );
	virtual void	GetWaterCheckPosition( int waterLevel, Vector *pos );

	// Determine if player is in water, on ground, etc.
	virtual void CategorizePosition( void );

	virtual void	CheckParameters( void );

	virtual	void	ReduceTimers( void );

	virtual void	CheckFalling( void );

	virtual void	PlayerRoughLandingEffects( float fvol );

	void			PlayerWaterSounds( void );

	void ResetGetWaterContentsForPointCache();
	int GetWaterContentsForPointCached( const Vector &point, int slot );

	// Ducking
	virtual void	Duck( void );
	virtual void	HandleDuckingSpeedCrop();
	virtual void	FinishUnDuck( void );
	virtual void	FinishDuck( void );
	virtual bool	CanUnduck();
	virtual void	UpdateDuckJumpEyeOffset( void );
	virtual bool	CanUnDuckJump( trace_t &trace );
	virtual void	StartUnDuckJump( void );
	virtual void	FinishUnDuckJump( trace_t &trace );
	virtual void	SetDuckedEyeOffset( float duckFraction );
	virtual void	FixPlayerCrouchStuck( bool moveup );

	float			SplineFraction( float value, float scale );

	virtual void	CategorizeGroundSurface( trace_t &pm );

	virtual bool	InWater( void );

	// Commander view movement
	void			IsometricMove( void );

	// Traces the player bbox as it is swept from start to end
	virtual CBaseHandle		TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );

	// Checks to see if we should actually jump 
	void			PlaySwimSound();

	bool			IsDead( void ) const;

	// Figures out how the constraint should slow us down
	float			ComputeConstraintSpeedFactor( void );

	virtual void	SetGroundEntity( trace_t *pm );

	virtual void	StepMove( Vector &vecDestination, trace_t &trace );

protected:
	virtual ITraceFilter *LockTraceFilter( int collisionGroup );
	virtual void UnlockTraceFilter( ITraceFilter *&pFilter );

	// Performs the collision resolution for fliers.
	void			PerformFlyCollisionResolution( trace_t &pm, Vector &move );

	virtual bool	GameHasLadders() const;

	enum
	{
		// eyes, waist, feet points (since they are all deterministic
		MAX_PC_CACHE_SLOTS = 3,
	};

	// Cache used to remove redundant calls to GetPointContents() for water.
	int m_CachedGetPointContents[ MAX_PLAYERS ][ MAX_PC_CACHE_SLOTS ];
	Vector m_CachedGetPointContentsPoint[ MAX_PLAYERS ][ MAX_PC_CACHE_SLOTS ];	

//private:
	bool			m_bSpeedCropped;

	bool			m_bProcessingMovement;
	bool			m_bInStuckTest;

	float			m_flStuckCheckTime[MAX_PLAYERS+1][2]; // Last time we did a full test

	// special function for teleport-with-duck for episodic
#ifdef HL2_EPISODIC
public:
	void			ForceDuck( void );

#endif
	ITraceListData	*m_pTraceListData;

	int				m_nTraceCount;
};


//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
inline void CGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	++m_nTraceCount;
	VPROF( "CGameMovement::TracePlayerBBox" );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );
	ITraceFilter *pFilter = LockTraceFilter( collisionGroup );
	if ( m_pTraceListData && m_pTraceListData->CanTraceRay(ray) )
	{
		enginetrace->TraceRayAgainstLeafAndEntityList( ray, m_pTraceListData, fMask, pFilter, &pm );
	}
	else
	{
		enginetrace->TraceRay( ray, fMask, pFilter, &pm );
	}
	UnlockTraceFilter( pFilter );
}

inline void CGameMovement::GameMovementTraceHull( const Vector& start, const Vector& end, const Vector &mins, const Vector &maxs, unsigned int fMask, ITraceFilter *pFilter, trace_t *pTrace )
{
	++m_nTraceCount;
	Ray_t ray;
	ray.Init( start, end, mins, maxs );
	if ( m_pTraceListData && m_pTraceListData->CanTraceRay(ray) )
	{
		enginetrace->TraceRayAgainstLeafAndEntityList( ray, m_pTraceListData, fMask, pFilter, pTrace );
	}
	else
	{
		enginetrace->TraceRay( ray, fMask, pFilter, pTrace );
	}
}

#endif // GAMEMOVEMENT_H
