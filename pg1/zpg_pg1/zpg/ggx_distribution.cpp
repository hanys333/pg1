#include "stdafx.h"
#include "ggx_distribution.h"

std::string path = "D:\\T4J\\VSB-pg1\\result\\";
const int SamplesCount = 10;

Vector3 ggx_distribution::orthogonal(const Vector3 & v)
{
	return (abs(v.x) > abs(v.z)) ? Vector3(-v.y, v.x, 0.0f) : Vector3(0.0f, -v.z, v.y);
}

Vector3 ggx_distribution::TransformToWS(Vector3 normal, Vector3 direction)
{
	direction.Normalize();
	// normal je osa z 
	//normal.z = (1-roughness) * normal.z;
	normal.Normalize();
	Vector3 o1 = orthogonal(normal); // o1 je pomocna osa x 
	o1.Normalize();
	Vector3 o2 = o1.CrossProduct(normal); // o2 je pomocna osa y 
	o2.Normalize();

	Vector3 direction_ws = Vector3(
		o1.x * direction.x + o2.x * direction.y + normal.x * direction.z,
		(o1.y * direction.x + o2.y * direction.y + normal.y * direction.z),
		(o1.z * direction.x + o2.z * direction.y + normal.z * direction.z)
	); // direction je vstupni vektor, ktery chcete "posadit" do ws 
	direction_ws.Normalize();

	return direction_ws;
}


float ggx_distribution::GGX_PartialGeometryTerm(Vector3 v, Vector3 n, Vector3 h, float alpha)
{
	h.Normalize();
	v.Normalize();
	n.Normalize();

	float VoH = saturate(v.DotProduct(h));
	float VoN = saturate(v.DotProduct(n));
	float chi = chiGGX(VoH / VoN);

	float VoH2 = VoH * VoH;
	float tan2 = (1 - VoH2) / VoH2;
	return (chi * 2) / (1 + sqrt(1 + alpha * alpha * tan2));
}


Vector3 ggx_distribution::Fresnel_Schlick(float cosT, Vector3 F0)
{
	return F0 + (Vector3(1, 1, 1) - F0) * pow(1 - cosT, 5);
}

float ggx_distribution::Ph(float theta, float phi, float alpha)
{
	//return (SQR(alpha) * cos(theta) * sin(theta)) / M_PI * SQR((SQR(alpha) - 1) * SQR(cos(theta)) + 1);
	return (2 * SQR(alpha) * cos(theta) * sin(theta)) / SQR((SQR(alpha) - 1) * SQR(cos(theta)) + 1);
}

float ggx_distribution::GetTheta(float alpha)
{
	float epsilon = Random();
	return acos(sqrt((1 - epsilon) / (epsilon * (SQR(alpha) - 1) + 1)));
	//return atan(alpha * sqrt(epsilon / (1 - epsilon)));
}


Vector3 ggx_distribution::GenerateGGXsampleVector(int i, float roughness)//, Vector3 normal)
{
	int ii = 0;

	while (true) //for (size_t i = 0; i < 5; i++)
	{
		float theta = GetTheta(roughness);
		float phi = Random(0, 2 * M_PI);
		float random = Random();

		float ph = Ph(theta, phi, roughness) / M_PI * 2;
		//printf("Pokus %d cyklus(%d) ph=%f  phi=%f\n", i, ii, ph, theta);

		if (ph > 1 - roughness)
		{
			return Vector3::GetFromSpherical(theta, phi);
		}
		else
		{
			ii++;
			continue;
		}
	}


}

Vector3 ggx_distribution::GGX_Specular(CubeMap cubeMapSpecular, Vector3 normal, Vector3 rayDir, float roughness, Vector3 F0, Vector3 *kS, int SamplesCount)
{
	
	Vector3 radiance = Vector3(0, 0, 0);
	float  NoV = saturate(normal.DotProduct(rayDir));



	for (int i = 0; i < SamplesCount; i++)
	{
		// Generate a sample vector in some local space
		Vector3 sampleVector = GenerateGGXsampleVector(i, roughness);

		////
		sampleVector.Normalize();
		////// Convert the vector in world space
		sampleVector = TransformToWS(normal, sampleVector);// worldFrame * sampleVector;

		sampleVector.Normalize();

		// Calculate the half vector
		Vector3 halfVector = sampleVector + rayDir;
		halfVector.Normalize();

		float cosT = saturate(sampleVector.DotProduct(normal));
		float sinT = sqrt(1 - cosT * cosT);

		// Calculate fresnel
		Vector3 fresnel = Fresnel_Schlick(saturate(halfVector.DotProduct(rayDir)), F0);
		// Geometry term
		float geometry = GGX_PartialGeometryTerm(rayDir, normal, halfVector, roughness) * GGX_PartialGeometryTerm(sampleVector, normal, halfVector, roughness);
		// Calculate the Cook-Torrance denominator
		float denominator = saturate(4 * (NoV * saturate(halfVector.DotProduct(normal)) + 0.05));
		*kS += fresnel;
		// Accumulate the radiance
		radiance += Vector3(cubeMapSpecular.GetTexel(sampleVector).data) * geometry * fresnel * sinT / denominator;
	}

	// Scale back for the samples count
	*kS = *kS / SamplesCount;
	(*kS).x = saturate((*kS).x);
	(*kS).y = saturate((*kS).y);
	(*kS).z = saturate((*kS).z);

	return radiance / SamplesCount;
}








int ggx_distribution::StartRender(Camera cameraSPhere, CubeMap cubeMap, int SamplesCount, Vector3 baseColor, std::string nameColor)
{
	projRenderGGX_Distribution(scene, surfaces, cameraSPhere, cubeMap, cubeMap, SamplesCount, baseColor, nameColor);
	return 0;
}


int ggx_distribution::projRenderGGX_Distribution(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, CubeMap cubeMap, CubeMap specularCubeMap, int SamplesCount, Vector3 baseColor, std::string nameColor)
{
	cv::Mat src_8uc3_img(480, 640, CV_32FC3);

	/*float roughness;
	float ior;
	float metallic;
	ior = 1.5f;


	roughness = 0.01f;
	metallic = 0.9f;*/
	//Vector3 baseColor;
	std::string str;
	//baseColor = Vector3(0.560, 0.570, 0.580); str = "IRON";// iron
	//baseColor = Vector3(0.972, 0.960, 0.915); str = "SILVER"; // silver
	//baseColor = Vector3(0.913, 0.921, 0.925); str = "ALUMINIUM"; // aluminium
	//baseColor = Vector3(1.000, 0.766, 0.336); str = "GOLD";// gold
	//baseColor = Vector3(0.550, 0.556, 0.554); str = "CHROMIUM"; // chromium
	//baseColor = Vector3(0.61f, 0.606f, 0.958f);
	

	float ior = 1 + baseColor.x;
	float roughness = saturate(baseColor.y - EPSILON) + EPSILON;
	float metallic = baseColor.z;

	/*ior = 1 + baseColor.x;
	roughness = saturate(baseColor.y - alpha) + alpha;
	roughness = alpha;
	metallic = baseColor.z;*/
	
	//metallic = 1;

	str = nameColor + "_" + std::to_string(SamplesCount) + " metallic_" + std::to_string(metallic) + " roughness_" + std::to_string(roughness) + " " + "ior_" + std::to_string(ior);
	

	cv::namedWindow(str, 1);

	for (int x = 0; x < 640; x++)
	{
#pragma omp parallel for schedule(dynamic, 5) shared(scene, surfaces, src_8uc3_img, camera)
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			//Vector3 re = phongRecursion(0, rtc_ray, Vector3(lightPosition[0], lightPosition[1], lightPosition[2]), camera.view_from(), scene, surfaces, cubeMap);




			rtcIntersect(scene, rtc_ray);



			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{


				Vector3 ret;
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);

				Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);

				Vector3 rayDir = Vector3(rtc_ray.dir);
				rayDir.Normalize();


				normal = normal.DotProduct(rayDir) < 0 ? normal : -normal;
				normal.Normalize();

				Vector3 h = rayDir + normal;
				h.Normalize();


				float F0_f = saturate((1.0 - ior) / (1.0 + ior));
				Vector3 F0 = Vector3(F0_f, F0_f, F0_f);
				F0 = F0 * F0;
				
				F0 = lerp(F0, baseColor, metallic);

				

				Vector3 ks = Vector3(0, 0, 0);
				Vector3 specular = GGX_Specular(specularCubeMap, normal, rayDir, roughness, F0, &ks, SamplesCount);
				Vector3 kd = (Vector3(1, 1, 1) - ks) * (1 - metallic);

				Vector3 irradiance = Vector3(cubeMap.GetTexel(normal).data);

				Vector3 diffuse = baseColor * irradiance;

				ret = kd * diffuse + specular;

				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(ret.z, ret.y, ret.x);


			}
			else
			{
				Color4 col = Color4(0.5f, 0.5f, 0.5f, 1.0f); //cubeMap.GetTexel(Vector3(rtc_ray.dir));
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(col.b, col.g, col.r);

			}



		}

		cv::imshow(str, src_8uc3_img); // display image

		cv::moveWindow(str, 10, 50);

		

		cvWaitKey(1);
	}

	cv::Mat finalImage;

	cv::convertScaleAbs(src_8uc3_img, finalImage, 255.0f);


	std::string resultPath = path + str + ".png";
	cv::imwrite(resultPath, finalImage);
	std::cout << resultPath << std::endl;
	//cvSaveImage("D:\\" + str + ".jpg", src_8uc3_img);
	//cvWaitKey(0);
	//std::string str = "GGX_Distribution metallic(" + std::to_string(metallic) + ")  roughness(" + std::to_string(roughness) + ")";


	return 0;
}


//#pragma endregion

int ggx_distribution::testSamplingOnSphere(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap)
{

	cv::Mat src_8uc3_img(480, 640, CV_32FC3);
	float roughness = 0.5f;

	int SamplesCount = 100;
	std::string str = "testSamplingOnSphere roughness(" + std::to_string(roughness) + ")";


	for (int x = 0; x < 640; x++)
	{
#pragma omp parallel for schedule(dynamic, 5) shared(scene, surfaces, src_8uc3_img, camera)
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			//Vector3 re = phongRecursion(0, rtc_ray, Vector3(lightPosition[0], lightPosition[1], lightPosition[2]), camera.view_from(), scene, surfaces, cubeMap);




			rtcIntersect(scene, rtc_ray);



			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{


				Vector3 ret;
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);

				Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);

				Vector3 rayDir = Vector3(rtc_ray.dir);
				rayDir.Normalize();



				normal = normal.DotProduct(rayDir) < 0 ? normal : -normal;
				normal.Normalize();

				/*normal += Vector3(1, 1, 1);
				normal *= 0.5;
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(normal.z, normal.y, normal.x);
				continue;
				*/
				Vector3 reflectionVector = reflect(normal, rayDir);

				//reflectionVector += Vector3(1, 1, 1);
				//reflectionVector *= 0.5;

				//src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(reflectionVector.z, reflectionVector.y, reflectionVector.x);
				//continue;

				Vector3 irradiance = Vector3(0, 0, 0);

				for (int i = 0; i < SamplesCount; i++)
				{
					// Generate a sample vector in some local space
					Vector3 sampleVector = GenerateGGXsampleVector(i, roughness);
					//sampleVector = Vector3(0,0,1);
					////
					sampleVector.Normalize();
					////// Convert the vector in world space
					sampleVector = TransformToWS(normal, sampleVector);// worldFrame * sampleVector;
					sampleVector.Normalize();



					irradiance += Vector3(cubeMap.GetTexel(sampleVector).data);
				}

				irradiance = irradiance / SamplesCount;

				ret = Vector3(irradiance);

				/*ret += Vector3(1, 1, 1);
				ret *= 0.5;*/


				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(ret.z, ret.y, ret.x);

				//kd * diffuse + ks * specular



			}
			else
			{
				Color4 col = cubeMap.GetTexel(Vector3(rtc_ray.dir)); //Color4(0, 0, 0, 0); //
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(col.b, col.g, col.r);

			}



			/*src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);*/


		}

		cv::imshow(str, src_8uc3_img); // display image

		cv::moveWindow(str, 10, 50);
		cvWaitKey(1);
	}

	//std::string str = "GGX_Distribution metallic(" + std::to_string(metallic) + ")  roughness(" + std::to_string(roughness) + ")";


	return 0;
}

int ggx_distribution::GenerateTestingSamples(float roughness, cv::Vec3b color, char* name)
{
	int size = 200;
	cv::Mat dst_resultTest(size, size, CV_8UC3);


	//printf("Roughness:%f", roughness);

	for (int x = 0; x < size; x++)
	{
		for (int y = 0; y < size; y++)
		{
			dst_resultTest.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
		}
	}
	for (int i = 0; i < 500; i++)
	{




		//printf("Zacatek Pokus:%d\n", i);
		Vector3 vec = GenerateGGXsampleVector(i, roughness);

		dst_resultTest.at<cv::Vec3b>(size - (int)(vec.z * size), (int)(vec.x * size)) = color;


		//printf("Pokus:%d   %f, %f, %f\n", i, vec.x, vec.y, vec.z);
	}

	cv::imshow(name, dst_resultTest);
	//cvWaitKey(0);
	return 1;
}



ggx_distribution::ggx_distribution(RTCScene & _scene, std::vector<Surface *> & _surfaces, int SamplesCount, Vector3 baseColor, std::string nameColor)
{

	/**/
	scene = _scene;
	surfaces = _surfaces;
	
	//projRenderGGX_Distribution(scene, surfaces, cameraSPhere, cubeMap, cubeMap, SamplesCount, baseColor, nameColor);

	//TEST - sampling on sphere
	//testSamplingOnSphere(scene, surfaces, cameraSPhere, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);
}

ggx_distribution::~ggx_distribution()
{
}





