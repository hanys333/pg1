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
	Vector3 GenerateGGXsampleVector(int i, float roughness);


	Vector3 Fresnel_Schlick(float cosT, Vector3 F0);
	
	float GGX_PartialGeometryTerm(Vector3 v, Vector3 n, Vector3 h, float alpha);
	Vector3 GGX_Specular(CubeMap cubeMapSpecular, Vector3 normal, Vector3 rayDir, float roughness, Vector3 F0, Vector3 * kS, int SamplesCount);
	
	//int StartRender(Camera cameraSPhere, CubeMap cubeMap, int SamplesCount, Vector3 baseColor, std::string nameColor);

	int StartRender(Camera cameraSPhere, CubeMap cubeMap, int SamplesCount, Vector3 baseColor, std::string nameColor, float _ior = -1.0f, float _roughness = -1.0f, float _metallic = -1.0f);

	int projRenderGGX_Distribution(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, CubeMap cubeMap, CubeMap specularCubeMap, int SamplesCount, Vector3 baseColor, float ior, float roughness, float metallic, std::string nameColor);

	//TESTS
	int testSamplingOnSphere(RTCScene & scene, std::vector<Surface*>& surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap);
	int GenerateTestingSamples(float roughness, cv::Vec3b color, char * name);
	
	
	ggx_distribution(RTCScene & scene, std::vector<Surface *> & surfaces, int SamplesCount, Vector3 baseColor, std::string nameColor);
	~ggx_distribution();
};

#endif
