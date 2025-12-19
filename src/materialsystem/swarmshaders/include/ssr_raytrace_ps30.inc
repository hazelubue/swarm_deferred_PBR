#pragma once
#include "shaderlib/cshader.h"
class ssr_raytrace_ps30_Static_Index
{
public:
	ssr_raytrace_ps30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_ssr_raytrace_ps30 1


class ssr_raytrace_ps30_Dynamic_Index
{
public:
	ssr_raytrace_ps30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_ssr_raytrace_ps30 1

