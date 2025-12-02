
#ifndef LIGHTING_PASS_BASIC_H
#define LIGHTING_PASS_BASIC_H

struct lightPassParms
{
	lightPassParms()
	{
		Q_memset(this, 0xFF, sizeof(lightPassParms));
	};

	int iLightTypeVar;
	int iWorldProjection;

	int bIsLightingPass;

	int m_nBaseTexture;
	int m_nAlbedo;

	int m_nWorldUVSampler;

	int m_nRoughness; 
	int m_nMetallicblend;
	int m_nRoughnessblend;
	int m_nMetallic;
	int m_nAO;
	int m_nEmissive;

	int Cubemap;

	int m_nMRAO;
	int m_nMRAO2;
	int m_nUseSmoothness;
	int m_nSpecularTexture;

	int m_nBRDF;

	int iUVScale;
	int iUVRotation;

	int m_nTransparency;
	int	m_nTransparencyFactor;

	int bModel;
};

struct GuassianPassParams
{
	GuassianPassParams()
	{
		Q_memset(this, 0xFF, sizeof(GuassianPassParams));
	};
	
	int ISVERTICAL;

	int BLUR;
	
};




void InitParmsLightPass( const lightPassParms &info, CBaseVSShader *pShader, IMaterialVar **params );
void InitPassLightPass( const lightPassParms &info, CBaseVSShader *pShader, IMaterialVar **params );

void DrawPassLightPass( const lightPassParms &info, CBaseVSShader *pShader, IMaterialVar **params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression);

void InitParmsGuassian(const GuassianPassParams& info, CBaseVSShader* pShader, IMaterialVar** params);
void InitPassGuassian(const GuassianPassParams& info, CBaseVSShader* pShader, IMaterialVar** params);

void DrawPassGuassian(const GuassianPassParams& info, CBaseVSShader* pShader, IMaterialVar** params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression);



#endif