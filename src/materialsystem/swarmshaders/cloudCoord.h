#ifndef INCLUDE_CLOUD_COORD
#define INCLUDE_CLOUD_COORD

const float cloudNarrowness = 0.07;

// Thanks to SixthSurge
float2 GetRoundedCloudCoord(float2 pos, float cloudRoundness) { // cloudRoundness is meant to be 0.125 for clouds and 0.35 for cloud shadows
    float2 coord = pos.xy + 0.5;
    float2 signCoord = sign(coord);
    coord = abs(coord) + 1.0;
    float2 i, f = modf(coord, i);
    f = smoothstep(0.5 - cloudRoundness, 0.5 + cloudRoundness, f);
    coord = i + f;
    return (coord - 0.5) * signCoord / 256.0;
}

float3 ModifyTracePos(float3 tracePos, int cloudAltitude) {
//#if CLOUD_SPEED_MULT == 100
//    float wind = syncedTime;
//#else
////#define CLOUD_SPEED_MULT_M CLOUD_SPEED_MULT * 0.01
////    float wind = frameTimeCounter * CLOUD_SPEED_MULT_M;
////#endif
    //tracePos.x += wind;
    tracePos.z += cloudAltitude * 64.0;
    tracePos.xz *= cloudNarrowness;
    return tracePos.xyz;
}

#endif