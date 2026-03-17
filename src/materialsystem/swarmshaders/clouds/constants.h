#if !defined INCLUDE_SKY_CLOUDS_CONSTANTS
#define INCLUDE_SKY_CLOUDS_CONSTANTS


// ====================================
// Cloud System Constants
// ====================================

static const float clouds_cumulus_radius = 6371000.0 + 1500.0;
static const float clouds_cumulus_top_radius = 6371000.0 + 3500.0;
static const float clouds_cumulus_thickness = 2000.0;
static const float clouds_cumulus_congestus_distance = 50000.0;
static const float clouds_altocumulus_radius = 6371000.0 + 4000.0;

static float world_age = 0.0;
static float3 cameraPosition = float3(0, 0, 0);
static const float CLOUDS_SCALE = 1.0;
static const float CLOUDS_CUMULUS_WIND_ANGLE = 45.0;
static const float CLOUDS_CUMULUS_WIND_SPEED = 10.0;
static const int CLOUDS_CUMULUS_PRIMARY_STEPS_H = 16;
static const int CLOUDS_CUMULUS_PRIMARY_STEPS_Z = 24;
static const int CLOUDS_CUMULUS_LIGHTING_STEPS = 3;
static const int CLOUDS_CUMULUS_AMBIENT_STEPS = 2;

static const float CLOUDS_CUMULUS_SIZE = 1.0;

// Global cloud parameters
static CloudParams clouds_params;
//static float3 sun_dir;
//static float3 moon_dir;
//static float3 sun_color;
//static float3 moon_color;
//static float3 sky_color;
//static float3 sunlight_color;

#endif // INCLUDE_SKY_CLOUDS_CONSTANTS
