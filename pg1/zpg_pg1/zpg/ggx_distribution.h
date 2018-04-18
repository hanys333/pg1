#ifndef GGXDISTR_H_
#define GGXDISTR_H_

#include "camera.h"

class ggx_distribution
{
private:
	

public:

	RTCScene scene;
	std::vector<Surface *> surfaces;
	/*Camera cameraSPhere;
	CubeMap cubeMap;*/


	float saturate(float val)
	{
		if (val > 1)
		{
			return 1;
		}
		else if (val < 0)
		{
			return 0;
		}

		return val;
	}

	Vector3 reflect(Vector3 normal, Vector3 viewVec)
	{
		viewVec.Normalize();

		return viewVec - 2 * normal * viewVec.DotProduct(normal);
		//return   2 * (normal.DotProduct(viewVec)) * normal - viewVec;
	}
	Vector3 lerp(Vector3 v0, Vector3 v1, float t)
	{
		return (1 - t)*v0 + t*v1;
	}

	float chiGGX(float v)
	{
		return v > 0 ? 1 : 0;
	}


	//generovani samplu
	Vector3 orthogonal(const Vector3 & v);
	Vector3 TransformToWS(Vector3 normal, Vector3 direction);
	float Ph(float theta, float phi, float alpha);
	float GetTheta(float alpha);
	Vector3 GenerateGGXsampleVector(float roughness);


	Vector3 Fresnel_Schlick(float cosT, Vector3 F0);
	
	float GGX_PartialGeometryTerm(Vector3 v, Vector3 n, Vector3 h, float alpha);
	Vector3 GGX_Specular(CubeMap cubeMapSpecular, Vector3 normal, Vector3 rayDir, float roughness, Vector3 F0, Vector3 * kS, int SamplesCount);

	int StartRender(Camera cameraSPhere, CubeMap cubeMap, Vector3 lightDir, int SamplesCount, GGXColor col, std::string info, float _ior = -1.0f, float _roughness = -1.0f, float _metallic = -1.0f);

	int projRenderGGX_Distribution(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, CubeMap cubeMap, CubeMap specularCubeMap, Vector3 lightVector, int SamplesCount, Vector3 baseColor, float ior, float roughness, float metallic, std::string nameColor);

	int ggx_distribution::testGeometryTerm(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, CubeMap cubeMap, CubeMap specularCubeMap, Vector3 lightDir, int SamplesCount, Vector3 baseColor, float ior, float roughness, float metallic, std::string nameColor);

	//TESTS
	int testSamplingOnSphere(RTCScene & scene, std::vector<Surface*>& surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap);
	int GenerateTestingSamples(float roughness, cv::Vec3b color, char * name);
	
	ggx_distribution();
	ggx_distribution(RTCScene & scene, std::vector<Surface *> & surfaces);
	~ggx_distribution();

	Vector3 GetColorValue(GGXColor col)
	{
		switch (col)
		{
		case GOLD:
			return Vector3(1.000, 0.766, 0.336);
		case SILVER:
			return Vector3(0.972, 0.960, 0.915);
		case IRON:
			return Vector3(0.560, 0.570, 0.580);
		case ALUMINIUM:
			return Vector3(0.913, 0.921, 0.925);
		case CHROMIUM:
			return Vector3(0.550, 0.556, 0.554);
		default:
			break;
		}

		return Vector3(0, 1, 1);
	}

	std::string GetColorString(GGXColor col)
	{
		switch (col)
		{
		case GOLD:
			return "GOLD";
		case SILVER:
			return "SILVER";
		case IRON:
			return "IRON";
		case ALUMINIUM:
			return "ALUMINIUM";
		case CHROMIUM:
			return "CHROMIUM";
		default:
			break;
		}

		return "UNKNOWN";
	}
};

#endif
