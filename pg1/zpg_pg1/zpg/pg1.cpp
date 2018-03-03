#include "stdafx.h"
#include "ggx_distribution.h"


void rtc_error_function(const RTCError code, const char * str)
{
	printf("ERROR in Embree: %s\n", str);
	exit(1);
}

RTCError check_rtc_or_die(RTCDevice & device)
{
	const RTCError error = rtcDeviceGetError(device);

	if (error != RTC_NO_ERROR)
	{
		printf("ERROR in Embree: ");

		switch (error)
		{
		case RTC_UNKNOWN_ERROR:
			printf("An unknown error has occurred.");
			break;

		case RTC_INVALID_ARGUMENT:
			printf("An invalid argument was specified.");
			break;

		case RTC_INVALID_OPERATION:
			printf("The operation is not allowed for the specified object.");
			break;

		case RTC_OUT_OF_MEMORY:
			printf("There is not enough memory left to complete the operation.");
			break;

		case RTC_UNSUPPORTED_CPU:
			printf("The CPU is not supported as it does not support SSE2.");
			break;

		case RTC_CANCELLED:
			printf("The operation got cancelled by an Memory Monitor Callback or Progress Monitor Callback function.");
			break;
		}

		fflush(stdout);
		exit(1);
	}

	return error;
}

// struktury pro ukládání dat pro Embree
namespace embree_structs
{
	struct Vertex { float x, y, z, a; };
	typedef Vertex Normal;
	struct Triangle { int v0, v1, v2; };
};

void filter_intersection(void * user_ptr, Ray & ray)
{
	/*  All hit information inside the ray is valid.
	The filter function can reject a hit by setting the geomID member of the ray to
	RTC_INVALID_GEOMETRY_ID, otherwise the hit is accepted.The filter function is not
	allowed to modify the ray input data (org, dir, tnear, tfar), but can modify
	the hit data of the ray( u, v, Ng, geomID, primID ). */

	Surface * surface = reinterpret_cast<Surface *>(user_ptr);
	printf("intersection of: %s, ", surface->get_name().c_str());
	const Vector3 p = ray.eval(ray.tfar);
	printf("at: %0.3f (%0.3f, %0.3f, %0.3f)\n", ray.tfar, p.x, p.y, p.z);

	ray.geomID = RTC_INVALID_GEOMETRY_ID; // reject hit
}

void filter_occlusion(void * user_ptr, Ray & ray)
{
	/*  All hit information inside the ray is valid.
	The filter function can reject a hit by setting the geomID member of the ray to
	RTC_INVALID_GEOMETRY_ID, otherwise the hit is accepted.The filter function is not
	allowed to modify the ray input data (org, dir, tnear, tfar), but can modify
	the hit data of the ray( u, v, Ng, geomID, primID ). */

	Surface * surface = reinterpret_cast<Surface *>(user_ptr);
	printf("occlusion of: %s, ", surface->get_name().c_str());
	const Vector3 p = ray.eval(ray.tfar);
	printf("at: %0.3f (%0.3f, %0.3f, %0.3f)\n", ray.tfar, p.x, p.y, p.z);

	ray.geomID = RTC_INVALID_GEOMETRY_ID; // reject hit
}

#define SQR( x ) ( ( x ) * ( x ) )
#define DEG2RAD( x ) ( ( x ) * M_PI / 180.0 )
#define MY_MIN( X, Y ) ( ( X ) < ( Y ) ? ( X ) : ( Y ) )

#pragma region GPG

Vector3 sphere_S;
float sphere_r;

Vector3 IntersectSphere(Vector3 S, float r, RTCRay &ray)
{
	Vector3 u = Vector3(ray.dir);
	Vector3 A = Vector3(ray.org);

	u.Normalize();

	float a = SQR(u.x) + SQR(u.y) + SQR(u.z);
	float b = 2 * (A.x * u.x - S.x * u.x + A.y * u.y - S.y * u.y + A.z * u.z - S.z * u.z);
	float c = SQR(A.x - S.x) + SQR(A.y - S.y) + SQR(A.z - S.z) - SQR(r);


	float determinant = SQR(b) - 4 * a * c;


	if (determinant < 0)
	{
		ray.geomID = RTC_INVALID_GEOMETRY_ID;
		return Vector3(0,0,0);
	}
	else 
	{
		ray.geomID = 1;
		float t = (- b - sqrt(determinant)) / (2 * a);
		

		if (t < ray.tnear || t > ray.tfar)
		{
			if (determinant != 0)
			{
				t = (-b + sqrt(determinant)) / (2 * a);

				if (t < ray.tnear || t > ray.tfar)
				{
					ray.geomID = RTC_INVALID_GEOMETRY_ID;
					return Vector3(0, 0, 0);
				}
				
				//return Vector3(A.x + u.x * t, A.y + u.y * t, A.z + u.z * t);
			}
			else
			{
				ray.geomID = RTC_INVALID_GEOMETRY_ID;
				return Vector3(0, 0, 0);
			}
		}


		return Vector3(A.x + u.x * t, A.y + u.y * t, A.z + u.z * t);
		

	}

}


#pragma endregion


#pragma region old_projects

int test(RTCScene & scene, std::vector<Surface *> & surfaces)
{
	// --- test rtcIntersect -----
	Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
	//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
	//rtc_ray.tnear = 0.6f;
	//rtc_ray.tfar = 2.0f;
	rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
								  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
								  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

	if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		Surface * surface = surfaces[rtc_ray.geomID];
		Triangle & triangle = surface->get_triangle(rtc_ray.primID);
		//Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

		// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
		const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
		Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
		geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
		const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
		const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);


	}

	// --- test rtcOccluded -----
	rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
	//rtc_ray.tfar = 1.5;	
	rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
								 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
	if (rtc_ray.geomID == 0)
	{
		// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
	}

	return 0;
}


int renderOcluded(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Mat src_8uc3_img)
{
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			//rtc_ray.tfar = 2.0f;
			rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
										  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
										  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

			src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);

			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				//Surface * surface = surfaces[rtc_ray.geomID];
				//Triangle & triangle = surface->get_triangle(rtc_ray.primID);
				////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

				//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
				//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
				//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
				//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
				//const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
				//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

				src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);

				printf("");
			}

			// --- test rtcOccluded -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
			//rtc_ray.tfar = 1.5;	
			rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
										 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
			if (rtc_ray.geomID == 0)
			{
				src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
				// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
			}
			else
			{
				src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
			}

		}
	}

	cv::imshow("srcocluded", src_8uc3_img); // display image
	cv::waitKey(0);
	return 0;
}

int render(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Mat src_8uc3_img)
{
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			//rtc_ray.tfar = 2.0f;
			rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
										  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
										  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);

			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				//Surface * surface = surfaces[rtc_ray.geomID];
				//Triangle & triangle = surface->get_triangle(rtc_ray.primID);
				////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

				//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
				//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
				//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
				//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
				//const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
				//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 1, 0);


			}

			//// --- test rtcOccluded -----
			//rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
			////rtc_ray.tfar = 1.5;	
			//rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
			//							 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
			//if (rtc_ray.geomID == 0)
			//{
			//	//src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
			//	// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
			//}
		}
	}

	cv::imshow("SRC", src_8uc3_img); // display image
	cv::waitKey(0);
	return 0;
}

cv::Vec3f NormalShader(cv::Vec3f normal)
{
	return (normal / 2) + cv::Vec3f(0.5f, 0.5f, 0.5f);
}



int renderNormal(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Mat src_8uc3_img)
{
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			//rtc_ray.tfar = 2.0f;
			rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
										  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
										  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);

			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);
				////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

				//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
				//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
				//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
				//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
				const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
				//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

				cv::Vec3f v = NormalShader(cv::Vec3f(normal.x, normal.y, normal.z));

				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(v[2], v[1], v[0]);


			}

			//// --- test rtcOccluded -----
			//rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
			////rtc_ray.tfar = 1.5;	
			//rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
			//							 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
			//if (rtc_ray.geomID == 0)
			//{
			//	//src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
			//	// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
			//}
		}
	}

	cv::imshow("normal", src_8uc3_img); // display image
	return 0;
}

cv::Vec3f LambertShader(cv::Vec3f normal, cv::Vec3f lightPosition, cv::Vec3f point)
{
	cv::Vec3f lightDirection = (lightPosition - point);

	lightDirection = lightDirection / (sqrt(lightDirection[0] * lightDirection[0]) + sqrt(lightDirection[1] * lightDirection[1]) + sqrt(lightDirection[2] * lightDirection[2]));

	return (normal.dot(lightDirection) * cv::Vec3f(0.5f, 0.5f, 0.5f));




}

int renderLambert(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Mat src_8uc3_img, cv::Vec3f lightPosition)
{
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			//rtc_ray.tfar = 2.0f;
			rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
										  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
										  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);

			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);
				////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

				//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
				//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
				//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
				//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
				const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
				//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

				const Vector3 point = rtc_ray.eval(rtc_ray.tfar);

				src_8uc3_img.at<cv::Vec3f>(y, x) = LambertShader(cv::Vec3f(normal.x, normal.y, normal.z), lightPosition, cv::Vec3f(point.x, point.y, point.z));


			}

			//// --- test rtcOccluded -----
			//rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
			////rtc_ray.tfar = 1.5;	
			//rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
			//							 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
			//if (rtc_ray.geomID == 0)
			//{
			//	//src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
			//	// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
			//}
		}
	}

	cv::imshow("lambert", src_8uc3_img); // display image

	return 0;
}


int renderBackground(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, CubeMap & cubeMap, cv::Vec3f lightPosition)
{

	cv::Mat src_8uc3_img(480, 640, CV_32FC3);

	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			//rtc_ray.tfar = 2.0f;
			rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
										  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
										  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

										  //src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);



			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);
				////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

				//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
				//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
				//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
				//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
				const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
				//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

				const Vector3 point = rtc_ray.eval(rtc_ray.tfar);

				src_8uc3_img.at<cv::Vec3f>(y, x) = LambertShader(cv::Vec3f(normal.x, normal.y, normal.z), lightPosition, cv::Vec3f(point.x, point.y, point.z));


			}
			else
			{
				Color4 tex = cubeMap.GetTexel(Vector3(rtc_ray.dir[0], rtc_ray.dir[1], rtc_ray.dir[2]));
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(tex.b, tex.g, tex.r);

			}

			//// --- test rtcOccluded -----
			//rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
			////rtc_ray.tfar = 1.5;	
			//rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
			//							 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
			//if (rtc_ray.geomID == 0)
			//{
			//	//src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
			//	// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
			//}
		}
	}

	cv::imshow("background", src_8uc3_img); // display image

	return 0;
}

#pragma endregion

Vector3 reflect(Vector3 normal, Vector3 viewVec)
{
	viewVec.Normalize();

	return viewVec - 2 * normal * viewVec.DotProduct(normal);
	//return   2 * (normal.DotProduct(viewVec)) * normal - viewVec;
}

#pragma region renderovacirovnice 


double fRand(double fMin, double fMax)
{
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}


// Renderovaci rovnice
// L0(omega0) = Le(omega0) + integral(Li(omegai) * fr(omega_1, omega_2) * (normal dot omega0))dw0

//integral nahradit montekarlem
// = 1/N Suma(fi(xi)/pdf(xi))          suma jde od i=0 do N-1

//fr(omega_1, omega_2) .. material
//Pro testovani  fr_lambert(omega_1, omega_2) = albedo/pi         omega1 ani 2 nejsou vyuzity
//  albedo - nejaka odrazivost 0,5/pi

double rr_Fr(Vector3 omega_1, Vector3 omega_2)
{
	//return 1.0;
	return 0.5 / M_PI;

	//reflect()
}

//Udelat     podle kompendia
// normal -> omega_i
Vector3 rr_Omega_i(Vector3 normal)
{


	//double r1 = Random(0, 1);
	//double r2 = Random(0, 1);
	//double phi = 2 * M_PI * r1;

	//float sqrt_1r = std::sqrt(1 - r2 * r2);
	//Vector3 omega = Vector3(cos(phi) * sqrt_1r, sin(phi) * sqrt_1r, r2);
	//omega.Normalize();
	//return (omega.DotProduct(normal) < 0) ? -omega : omega;

	double r1 = Random(0, 1);
	double r2 = Random(0, 1);

	float x = cos(2 * M_PI * r1) * sqrt(1 - r2);
	float y = sin(2 * M_PI * r1) * sqrt(1 - r2);
	float z = sqrt(r2);

	//double phi = 2 * M_PI * r1;

	//float sqrt_1r = std::sqrt(1 - r2 * r2);
	//Vector3 omega = Vector3(cos(phi) * sqrt_1r, sin(phi) * sqrt_1r, r2);
	//omega.Normalize();
	Vector3 omega = Vector3(x, y, z);
	return (omega.DotProduct(normal) < 0) ? -omega : omega;



}


//double fRandom(double fMin, double fMax)
//{
//
//	double f = (double)rand() / RAND_MAX;
//	return fMin + f * (fMax - fMin);
//}

//Pro nas experiment
// Li(omegai) = 1

double rr_Li(Vector3 omega_i)
{
	return 1;
}


Vector3 renderingEquation(Vector3 P, Vector3 direction, RTCScene & scene, std::vector<Surface *> & surfaces, int depth, CubeMap cubeMap)
{
	Ray ray = Ray(P, direction);
	ray.tnear = .1f;                                                                //TNEAR
	rtcOccluded(scene, ray);    // Pro zrychleni
								//  rtcIntersect(scene, ray);

	Vector3 colorFromThisRay;
	if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		ray = Ray(P, direction);
		ray.tnear = .1f;                                                                //TNEAR
		rtcIntersect(scene, ray);

		Vector3 material = Vector3(0.5f, 0.5f, 0.5f);           //TODO: MATERIAL surface.getmaterial!!
		Surface * surface = surfaces[ray.geomID];
		Triangle & triangle = surface->get_triangle(ray.primID);

		// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
		const Vector3 p = ray.eval(ray.tfar);
		Vector3 geometry_normal = -Vector3(ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
		geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
		const Vector3 normal = triangle.normal(ray.u, ray.v);
		const Vector2 texture_coord = triangle.texture_coord(ray.u, ray.v);



		Vector3 materialSP = (surface->get_material())->specular;
		Vector3 material_DP = (surface->get_material())->diffuse;



		colorFromThisRay = Vector3(0, 0, 0);
		if (depth != 0)
		{
			Vector3 omega0 = -direction;
			Vector3 norm = normal;

			if (norm.DotProduct(omega0) < 0)
			{
				norm = -norm;
			}


			Vector3 Le = Vector3(0.0, 0.0, 0.0);    //Svetlo emtiovane z bodu p



			int N = 1;                              //Pocet paprsku pro integrovani Monte-Christo
			Vector3 Suma = Vector3(0.0, 0.0, 0.0);

			float R = surface->get_material()->ior == 1.5f ? 0.1f : 1.0f;

			for (int i = 0; i < N; i++)
			{
				

				Vector3 omegai = rr_Omega_i(norm);

				Vector3 Li = renderingEquation(p, omegai, scene, surfaces, depth - 1, cubeMap);

				Vector3 reflection = reflect(normal, omega0);

				/*reflection.x *= fRandom(0.99f, 1.01f);
				reflection.y *= fRandom(0.99f, 1.01f);
				reflection.z *= fRandom(0.99f, 1.01f);*/
				reflection.Normalize();

				Vector3 Li_reflect = renderingEquation(p, reflection, scene, surfaces, depth - 1, cubeMap);

				double Fr = rr_Fr(omega0, omegai);
				double pdf = omegai.DotProduct(norm) /(2* M_PI);// 1 / (2 * M_PI);           //Uniformni rozdeleni pdf?
				Suma += ((Li * Fr * (omegai.DotProduct(norm))) / pdf) * R// * material_DP
					+
					(Li_reflect * (1 - R));
				//Suma = Suma + (Li * Fr * 1) / (pdf*2.0);
			}

			colorFromThisRay = (Le + (1 / N) * (Suma)) * material_DP ;
			//          colorFromThisRay = colorFromThisRay * material_DP;

		}
		else // Maximalni zanoreni, Vracim cernou
		{
			return Vector3(0.0, 0.0, 0.0);
		}
	}
	else
	{
		//Cubemap (Vraci pouze Le)
		Color4 cubeMapColor = cubeMap.GetTexel(Vector3(ray.dir));
		colorFromThisRay = Vector3(cubeMapColor.r, cubeMapColor.g, cubeMapColor.b);
		return colorFromThisRay;// Vector3(1.0, 1.0, 1.0);
	}
	return colorFromThisRay;


}


int renderRenderingEqua(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap)
{
	//TODOH
	cv::Mat src_8uc3_img(480, 640, CV_32FC3);


	std::string str = "renderingEquation";

	//#pragma omp parallel for
	for (int x = 0; x < 640; x++)
	{
#pragma omp parallel for schedule(dynamic, 5) shared(scene, surfaces, src_8uc3_img, camera)
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);





			int numOfsamples = 50;
			Vector3 re;
			for (int sample = 0; sample < numOfsamples; sample++)
			{
				re += renderingEquation(rtc_ray.org, rtc_ray.dir, scene, surfaces, 5, cubeMap);

			}
			re = re / numOfsamples;


			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);


		}
		cv::imshow(str, src_8uc3_img); // display image
		cvMoveWindow(str.c_str(), 10, 10);
		cvWaitKey(1);
	}




	//cv::imshow("renderingEquation", src_8uc3_img); // display image
	//cvMoveWindow("renderingEquation", 10, 10);
	return 0;
}


#pragma endregion 

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



Vector3 orthogonal(const Vector3 & v)
{
	return (abs(v.x) > abs(v.z)) ? Vector3(-v.y, v.x, 0.0f) : Vector3(0.0f, -v.z, v.y);
}

Vector3 TransformToWS(Vector3 normal, Vector3 direction)
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



#pragma region Phong

bool isInShadow(Vector3 point, Vector3 lightPosition, RTCScene & scene)
{
	Vector3 sizeVector = (lightPosition - point);

	float tfar = sqrt(SQR(sizeVector.x) + SQR(sizeVector.y) + SQR(sizeVector.z));


	Ray rtc_ray = Ray(point, sizeVector, 0.01f, tfar);

	rtcOccluded(scene, rtc_ray);


	return rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID;
}

Vector3 PhongShader(Vector3 normal, Vector3 lightPosition, Vector3 point, Vector3 eyeDirection, Vector3 specularColor, RTCScene & scene, Material* material, Vector2 textureCoord, bool wantShadow)
{
	Vector3 L = (lightPosition - point);
	L.Normalize();
	Vector3 E = eyeDirection;

	Vector3 H = (E + L);
	H.Normalize();


	Vector3 retVector = Vector3(0.07f, 0.07f, 0.07f);  // puvodne 0.1f
	
	Vector3 diffuseColor = material->diffuse;


	Texture * diff_tex = material->get_texture(Material::kDiffuseMapSlot);


	if (diff_tex != NULL)
	{
		Color4 diff_texel = diff_tex->get_texel(textureCoord.x, textureCoord.y);
		diffuseColor = Vector3(diff_texel.r, diff_texel.g, diff_texel.b);
	}



	Vector3 color = L.DotProduct(normal) * diffuseColor + material->specular * std::pow(H.DotProduct(normal), 89);


	bool inShadow = isInShadow(point, lightPosition, scene);

	if (inShadow && wantShadow)
	{
		//	retVector
		color = Vector3(0, 0, 0);
	}

	retVector = retVector + color;

	return retVector;

	/*
	cv::Vec3f lightDirection = (lightPosition - point);

	lightDirection = lightDirection / (sqrt(lightDirection[0] * lightDirection[0]) + sqrt(lightDirection[1] * lightDirection[1]) + sqrt(lightDirection[2] * lightDirection[2]));

	return (normal.dot(lightDirection) * cv::Vec3f(0.5f, 0.5f, 0.5f));
	*/
}

Vector3 phongRecursion(int depth, Ray rtc_ray, Vector3 lightPosition, Vector3 eyePosition, RTCScene & scene, std::vector<Surface *> & surfaces, CubeMap cubeMap)
{
	

	//TODO  nahrazeni rtcIntersect
	//rtcIntersect(scene, rtc_ray);
	Vector3 p = IntersectSphere(sphere_S, sphere_r, rtc_ray);



	if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		//TODO OLD PG1
		/*Surface * surfa = surfaces[rtc_ray.geomID];
		Triangle & triangle = surfa->get_triangle(rtc_ray.primID);
		
		Vector3 n = triangle.normal(rtc_ray.u, rtc_ray.v);
		n.Normalize();


		Vector2 tuv = triangle.texture_coord(rtc_ray.u, rtc_ray.v);
		
		const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
		Material * mtl = surfa->get_material();
		*/
		
		Vector2 tuv(0, 0);

		Vector3 n = p - sphere_S;
		n.Normalize();

		Material *mtl = new Material();
		mtl->ior = 1.5f;
		

		if (depth == 10) // puvodne 10
		{
			//return Vector3(1,0,0);

			/*if (mtl->ior == 1)
			{
				Color4 col = cubeMap.GetTexel(Vector3(rtc_ray.dir));
				return Vector3(col.r, col.g, col.b);
			}
			else
			{*/
				return PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);
			//}
		}


		if (mtl->ior == 1)
		{

			Vector3 color = PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);
			Vector3 rayDir = -Vector3(rtc_ray.dir);

			//n = n.DotProduct(rf) >= 0 ? n : -n;

			rayDir.Normalize();
			Vector3 reflectedDir = 2 * (n.DotProduct(rayDir)) * n - rayDir;

			Ray reflected(p, reflectedDir, 0.01f);
			reflected.set_ior(rtc_ray.ior);
			////////////Vector3 retVector = Vector3(0.1f, 0.1f, 0.1f);

			////////////Vector3 isInShadowVector = isInShadow(point, lightPosition, scene, surfaces) ? Vector3(0.5, 0.5, 0.5) * cos(L.DotProduct(normal)) + specularColor * cos(H.DotProduct(normal)) : Vector3(1, 1, 1);
			////////////isInShadowVector.Normalize();




			//if (!isInShadow(p, lightPosition, scene))
			{
				Vector3 reflected_rec = phongRecursion(depth + 1, reflected, lightPosition, p, scene, surfaces, cubeMap);

			

				color += 0.45f * reflected_rec;// + T * retracted_rec; //// puvodne 0.3f
			}

			////////////retVector = retVector + isInShadowVector;

			////////////return cv::Vec3f(retVector.x, retVector.y, retVector.z);

			//return phongRecursion(depth + 1, normal, lightPosition, p, eyePosition, specularColor, scene, surfaces, cubeMap);
			return color;
			//return PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);
		}
		else // toto je sklo
		{
			Vector3 rd = Vector3(rtc_ray.dir);
			rd.Normalize();
			Vector3 rf = -rd;// eyePosition - p;
			rf.Normalize();
			//rf = -rf;

			n = n.DotProduct(rf) >= 0 ? n : -n;

			n.Normalize();


			//n = -n;

			float n1 = 1.0f;
			float n2 = 1.5f;
			if (rtc_ray.ior == 1.5f)
			{
				n1 = 2.5 - n1;
				n2 = 2.5 - n2;
			}
			////	mtl->ior;//;rtc_ray.ior;//
			////
			////= rtc_ray.ior;////mtl->ior;

			////////rd = Vector3(-0.364, -0.931, 0.027);
			////////n = Vector3(0.2f, 1, -0.3f);
			////////n.Normalize();
			//////////n = -n;
			////////n1 = 1.5f;
			////////n2 = 1.0f;

			//std::cout << n1 << "  " << n2 << std::endl;

			float cos_O2 = (-n).DotProduct(rd);
			float cos_O1_undersqrt = 1 - SQR(n1 / n2) * (1 - SQR(cos_O2));
			float cos_O1 = cos_O1_undersqrt < 0.0f ? 1.0f : sqrt(cos_O1_undersqrt);


			Vector3 rr = -(n1 / n2) * rd - ((n1 / n2) * cos_O2 + cos_O1) * n;
			rr.Normalize();
			Vector3 l = rr - (2 * (n.DotProduct(rr))) * n;
			//l.Normalize();

			Vector3 lr = -l;

			//////////std::cout << "cos_O2:=" << cos_O2<< std::endl;
			//////////std::cout << "cos_O1:=" << cos_O1 << std::endl;

			//////////std::cout << "rr:=(" << rr.x << ", " << rr.y << ", " << rr.z << ")" <<std::endl << std::endl;
			//////////std::cout << "l:=(" << l.x << ", " << l.y << ", " << l.z << ")" << std::endl;
			//////////std::cout << "lr:=(" << lr.x << ", " << lr.y << ", " << lr.z << ")" << std::endl;
			//Ray refracted(p,  p -eyePosition , 0.1f);

			Ray refracted(p, lr, 0.01f);
			refracted.set_ior(n2);


			//refracted.set_ior(mtl->ior);
			////Vector3 color = phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap);
			//PhongShader(n, lightPosition, p, -lr, mtl->specular, scene, mtl, tuv, false);// phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap);


			Vector3 reflectedDir = 2 * (n.DotProduct(rf)) * n - rf;

			Ray reflected(p, reflectedDir, 0.01f);
			reflected.set_ior(n1);

			

			float R = 1;
			float T = 0;
			float n1cosi = n1 * lr.DotProduct(-n);
			float n1cost = n1 * (rf).DotProduct(n);
			float n2cosi = n2 * lr.DotProduct(-n);
			float n2cost = n2 * (rf).DotProduct(n);
			float Rs = pow((n1cosi - n2cost) / (n1cosi + n2cost), 2);
			float Rp = pow((n1cost - n2cosi) / (n1cost + n2cosi), 2);
			R = (Rs + Rp) * 0.5f;
			T = 1 - R;
			

			return //PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false)*0.1f+ 
				phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap) * mtl->diffuse * T
				+ phongRecursion(depth + 1, reflected, lightPosition, p, scene, surfaces, cubeMap) *  mtl->diffuse *R;// *mtl->specular;




			//return //PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false)*0.1f+ 
			//	phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap) * mtl->diffuse;

			
		}

	}
	else
	{
		Color4 col = cubeMap.GetTexel(Vector3(rtc_ray.dir));
		return Vector3(col.r, col.g, col.b);
	}
}


int renderPhong(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap)
{
	
	cv::Mat src_8uc3_img(480, 640, CV_32FC3);

#pragma omp parallel for
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);
			rtc_ray.set_ior(1.0f);

		/*	Vector3 inter = IntersectSphere(sphere_S, sphere_r, rtc_ray);

			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);
			}
			else
			{
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(1, 1, 1);
			}
*/
			Vector3 re = phongRecursion(0, rtc_ray, Vector3(lightPosition[0], lightPosition[1], lightPosition[2]), camera.view_from(), scene, surfaces, cubeMap);
			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);


		}
	}

	cv::imshow("phong", src_8uc3_img); // display image
	cvMoveWindow("phong", 5, 10);
	return 0;
}

#pragma endregion 



#pragma region PROJEKT_UE4


#define SQR(x) ((x)* (x))
float HemmersleySigma(int i, int numSamples)
{
	float sum = 0;
	for (int ii = 0; ii < numSamples; ii++)
	{
		sum += i * powf(2, -(ii + 1));
	}
	return sum;
}


Vector2 Hammersley(int i, int numSamples)
{
	//return Vector2();
	return Vector2(1 / numSamples, HemmersleySigma(i, numSamples));
}



float G_Smith(Vector3 N, Vector3 H, Vector3 V, Vector3 L)
{
	return 0;
}

Vector3 ImportanceSampleCGX(Vector2 Xi, float roughness, Vector3 N)
{

	float a = SQR(roughness);

	float PHi = 2 * M_PI * Xi.x;

	float CosTheta = sqrtf((1 - Xi.y) / (1 + (SQR(a) - 1) * Xi.y));
	float SinTheta = sqrtf(1 - SQR(CosTheta));

	Vector3 H;

	H.x = SinTheta * cos(PHi);
	H.y = SinTheta * sin(PHi);
	H.z = CosTheta;

	Vector3 UpVector;

	if (abs(N.z) < 0.999)
	{
		UpVector = Vector3(0, 0, 1);
	}
	else
	{
		UpVector = Vector3(1, 0, 0);
	}

	Vector3 TangentX = (UpVector.CrossProduct(N));
	TangentX.Normalize();

	Vector3 TangentY = (N.CrossProduct(TangentX));

	return TangentX * H.x + TangentY * H.y + N * H.z;

}

Vector3 SpecularIBL(Vector3 specularColor, float roughness, Vector3 N, Vector3 V, CubeMap cubeMap, int NumSamples)
{
	Vector3 SpecularLighting = Vector3();

	//const int NumSamples = 100;
	//printf("pred forem");
	for (int i = 0; i < NumSamples; i++)
	{
		Vector2 Xi = Hammersley(i, NumSamples);

		Vector3 H = ImportanceSampleCGX(Xi, roughness, N);

		Vector3 L = 2 * V.DotProduct(H) * H - V;

		float NoV = saturate(N.DotProduct(V));
		float NoL = saturate(N.DotProduct(L));
		float NoH = saturate(N.DotProduct(H));
		float VoH = saturate(V.DotProduct(H));

		if (NoL > 0)
		{
			Color4 v4col = cubeMap.GetTexel(L);
			Vector3 SampleColor = Vector3(v4col.r, v4col.g, v4col.b);
			//SampleColor.Normalize();
			float G = 5.0f;// G_Smith(roughness, NoV, NoL); /TODOERROR
			float Fc = pow(1 - VoH, 5);

			Vector3 F = ((1 - Fc) * specularColor);// + Fc;

			SpecularLighting += SampleColor * F * G* VoH / (NoH * VoH);
		}
	}

	//printf("za forem");

	return SpecularLighting / NumSamples;
	//return Vector3();
}






int renderSpecularIBL(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, float roughness, CubeMap cubeMap, Vector3 viewFrom, int numSamples)
{
	cv::Mat src_8uc3_img(480, 640, CV_32FC3);

	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			//rtc_ray.tfar = 2.0f;
			rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
										  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
										  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);

			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);
				////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

				//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
				//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
				//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
				//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
				const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
				//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

				const Vector3 point = rtc_ray.eval(rtc_ray.tfar);


				Vector3 V = (viewFrom - point);

				V.Normalize();

				Vector3 res = SpecularIBL(Vector3(0.5f, 0.5f, 0.5f), roughness, normal, V, cubeMap, numSamples);

				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(res.x, res.y, res.z);
			}
			else
			{
				Color4 tex = cubeMap.GetTexel(Vector3(rtc_ray.dir[0], rtc_ray.dir[1], rtc_ray.dir[2]));
				src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(tex.b, tex.g, tex.r);
			}

			//// --- test rtcOccluded -----
			//rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(1.0f, 0.0f, 0.0f));
			////rtc_ray.tfar = 1.5;	
			//rtcOccluded(scene, rtc_ray); // determining if any hit between a ray segment and the scene exists
			//							 // po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
			//if (rtc_ray.geomID == 0)
			//{
			//	//src_8uc3_img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
			//	// neco jsme nekde na zadaném intervalu (tnear, tfar) trefili, ale nevime co ani kde
			//}
		}
	}

	cv::imshow("IBL", src_8uc3_img); // display image

	return 0;
}

#pragma endregion







CubeMap cubeMap = CubeMap::CubeMap("../../data/yokohama");
Camera cameraSPhere = Camera(640, 480, Vector3(2.0f, 2.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f));
ggx_distribution distr = ggx_distribution();

std::string strTest = "26";

void TestCountSamples()
{
	int countSamples = 50;
	//for (int countSamples = 10; countSamples <= 100; countSamples += 30)
	{
		std::string str = strTest;//"18";
		float roughness = -1.0f;
		distr.StartRender(cameraSPhere, cubeMap, countSamples, GOLD, str, -1, roughness);
			distr.StartRender(cameraSPhere, cubeMap, countSamples, IRON, str, -1, roughness);
		//distr.StartRender(cameraSPhere, cubeMap, countSamples, SILVER, str, -1, roughness);
		//distr.StartRender(cameraSPhere, cubeMap, countSamples, ALUMINIUM, str, -1, roughness);
		
	}
}

void TestRoughness(GGXColor col)
{
	
	for (float roughness = 0.1f; roughness <= 1.1f; roughness += 0.2)
	{
		distr.StartRender(cameraSPhere, cubeMap, 30, col, strTest + " RoughnesssTest", -1, roughness);
	}

	/*distr.StartRender(cameraSPhere, cubeMap, 50, col, "ROUGHNESSTest", -1, 0.1);
	distr.StartRender(cameraSPhere, cubeMap, 50, col, "ROUGHNESSTest", -1, 0.2);
	distr.StartRender(cameraSPhere, cubeMap, 50, col, "ROUGHNESSTest", -1, 0.3);
		distr.StartRender(cameraSPhere, cubeMap, 50, col, "ROUGHNESSTest", -1, 0.6);
		distr.StartRender(cameraSPhere, cubeMap, 50, col, "ROUGHNESSTest", -1, 0.8);
	distr.StartRender(cameraSPhere, cubeMap, 50, col, "ROUGHNESSTest", -1, 1.0);*/
}

void TestMetallic(GGXColor col)
{
	distr.StartRender(cameraSPhere, cubeMap, 50, col, "METALLICTest", -1, -1, 0.1);
	distr.StartRender(cameraSPhere, cubeMap, 50, col, "METALLICTest", -1, -1, 0.5);
	distr.StartRender(cameraSPhere, cubeMap, 50, col, "METALLICTest", -1, -1, 1.0);
}

int main(int argc, char * argv[])
{
	printf("PG1, (c)2011-2016 Tomas Fabian\n\n");

	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON); // Flush to Zero, Denormals are Zero mode of the MXCSR
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	RTCDevice device = rtcNewDevice(NULL); // musíme vytvořit alespoň jedno Embree zařízení		
	check_rtc_or_die(device); // ověření úspěšného vytvoření Embree zařízení
	rtcDeviceSetErrorFunction(device, rtc_error_function); // registrace call-back funkce pro zachytávání chyb v Embree	

	std::vector<Surface *> surfaces;
	std::vector<Material *> materials;

	// načtení geometrie
	/*if (LoadOBJ("../../data/6887_allied_avenger.obj", Vector3(0.5f, 0.5f, 0.5f), surfaces, materials) < 0)
	{
		return -1;
	}*/


	if (LoadOBJ("../../data/geosphere.obj", Vector3(0.5f, 0.5f, 0.5f), surfaces, materials) < 0)
	{
		return -1;
	}

	// vytvoření scény v rámci Embree
	RTCScene scene = rtcDeviceNewScene(device, RTC_SCENE_STATIC | RTC_SCENE_HIGH_QUALITY, RTC_INTERSECT1/* | RTC_INTERPOLATE*/);
	// RTC_INTERSECT1 = enables the rtcIntersect and rtcOccluded functions

	// nakopírování všech modelů do bufferů Embree
	for (std::vector<Surface *>::const_iterator iter = surfaces.begin();
	iter != surfaces.end(); ++iter)
	{
		Surface * surface = *iter;
		unsigned geom_id = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC,
			surface->no_triangles(), surface->no_vertices());

		//rtcSetUserData, rtcSetBoundsFunction, rtcSetIntersectFunction, rtcSetOccludedFunction,
		rtcSetUserData(scene, geom_id, surface);
		//rtcSetOcclusionFilterFunction, rtcSetIntersectionFilterFunction		
		//rtcSetOcclusionFilterFunction( scene, geom_id, reinterpret_cast< RTCFilterFunc >( &filter_occlusion ) );
		//rtcSetIntersectionFilterFunction( scene, geom_id, reinterpret_cast< RTCFilterFunc >( &filter_intersection ) );

		// kopírování samotných vertexů trojúhelníků
		embree_structs::Vertex * vertices = static_cast< embree_structs::Vertex * >(
			rtcMapBuffer(scene, geom_id, RTC_VERTEX_BUFFER));

		for (int t = 0; t < surface->no_triangles(); ++t)
		{
			for (int v = 0; v < 3; ++v)
			{
				embree_structs::Vertex & vertex = vertices[t * 3 + v];

				vertex.x = surface->get_triangles()[t].vertex(v).position.x;
				vertex.y = surface->get_triangles()[t].vertex(v).position.y;
				vertex.z = surface->get_triangles()[t].vertex(v).position.z;
			}
		}

		rtcUnmapBuffer(scene, geom_id, RTC_VERTEX_BUFFER);

		// vytváření indexů vrcholů pro jednotlivé trojúhelníky
		embree_structs::Triangle * triangles = static_cast< embree_structs::Triangle * >(
			rtcMapBuffer(scene, geom_id, RTC_INDEX_BUFFER));

		for (int t = 0, v = 0; t < surface->no_triangles(); ++t)
		{
			embree_structs::Triangle & triangle = triangles[t];

			triangle.v0 = v++;
			triangle.v1 = v++;
			triangle.v2 = v++;
		}

		rtcUnmapBuffer(scene, geom_id, RTC_INDEX_BUFFER);

		/*embree_structs::Normal * normals = static_cast< embree_structs::Normal * >(
		rtcMapBuffer( scene, geom_id, RTC_USER_VERTEX_BUFFER0 ) );
		rtcUnmapBuffer( scene, geom_id, RTC_USER_VERTEX_BUFFER0 );*/
	}

	rtcCommit(scene);


	//baseColor = Vector3(0.560, 0.570, 0.580); str = "IRON";// iron
	//baseColor = Vector3(0.972, 0.960, 0.915); str = "SILVER"; // silver
	//baseColor = Vector3(0.913, 0.921, 0.925); str = "ALUMINIUM"; // aluminium
	//baseColor = Vector3(1.000, 0.766, 0.336); str = "GOLD";// gold
	//baseColor = Vector3(0.550, 0.556, 0.554); str = "CHROMIUM"; // chromium

	//Camera camSphere = Camera(640, 480, Vector3(2.0f, 2.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f));


	//cubeMap = CubeMap::CubeMap("../../data/yokohama");
	//cameraSPhere = Camera(640, 480, Vector3(2.0f, 2.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f));


	distr = ggx_distribution(scene, surfaces, 10, Vector3(0, 0, 0), std::string("a"));

	//TestCountSamples(); // test na pocet snimku 10, 40, 50, 100 ve vsech barvach

	TestRoughness(SILVER); //test na meneni roughness 0.1 0.5 1.0

	//TestMetallic(GOLD); //test na meneni metallic 0.1 0.5 1.0


	/*distr.GenerateTestingSamples(0.01, cv::Vec3b(0, 255, 0), "0.01");
	distr.GenerateTestingSamples(0.25, cv::Vec3b(0, 255, 0), "0.25");
	distr.GenerateTestingSamples(0.5, cv::Vec3b(0, 255, 0), "0.5");
	distr.GenerateTestingSamples(0.75, cv::Vec3b(0, 255, 0), "0.75");
	distr.GenerateTestingSamples(0.99, cv::Vec3b(0, 255, 0), "0.99");*/

	cv::waitKey(0);

	//Camera camera = Camera(640, 480, Vector3(-20.f, 0.f, 0.f),
	//	Vector3(0.f, 0.f, 0.f), DEG2RAD(42.185f));
	//camera.Print();

	//Ray primaryRay = cameraNum.GenerateRay(640.f / 2.f - 0.5f, 480.f / 2.f - 0.5f);

	//Vector3 v3 = primaryRay.eval(752.147f);

	//printf("primaryRay.Eval = (x=%f , y = %f , z = %f)", v3.x, v3.y, v3.z);

	////cv::Mat src_8uc3_img(camera.width, camera.height, CV_8UC3);
	//cv::Mat src_8uc3_img(480, 640, CV_8UC3);

	////src_8uc3_img = 

	//render(scene, surfaces, camera, src_8uc3_img);

	////cv::imshow("SRC", src_8uc3_img); // display image

	////cv::Mat src_8uc3_img(camera.width, camera.height, CV_8UC3);
	//cv::Mat src_8uc3_img_ocluded(480, 640, CV_8UC3);

	////src_8uc3_img = 

	//renderOcluded(scene, surfaces, camera, src_8uc3_img);



	/////////SHADERS
	/*Camera cameraSPaceShip = Camera(640, 480, Vector3(-400.0f, -500.0f, 370.0f),
	Vector3(70.0f, -40.5f, 5.0f), DEG2RAD(42.185f));*/
	/*
	Camera cameraSPaceShip = Camera(640, 480, Vector3(-140.0f, -175.0f, 110.0f), Vector3(0.0f, 0.0f, 40.0f), DEG2RAD(42.185f));

	cameraSPaceShip.Print();

	//cv::Mat src_8uf3_spaceship(480, 640, CV_32FC3);

	//CubeMap cubeMap = CubeMap::CubeMap("../../data/tenerife");
	CubeMap cubeMap = CubeMap::CubeMap("../../data/yokohama");
	
	CubeMap specularCubeMap = CubeMap::CubeMap("../../data/yokohamaSpecular");

	//////Camera cameraBackground = Camera(640, 480, Vector3(0.0f, 0.0f, 0.0f),
	//////	Vector3(1,0,0), DEG2RAD(120.0f));
	//////renderBackground(scene, surfaces, cameraSPaceShip, cubeMap, cv::Vec3f(-400.0f, -500.0f, 370.0f));

	//renderNormal(scene, surfaces, cameraSPaceShip, src_8uf3_spaceship);
	//renderLambert(scene, surfaces, cameraSPaceShip, src_8uf3_spaceship, cv::Vec3f(-400.0f, -500.0f, 370.0f));

	Camera cameraSPhere = Camera(640, 480, Vector3(2.0f, 2.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f));
	*/
	//testSamplingOnSphere(scene, surfaces, cameraSPhere, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);
	//projRenderGGX_Distribution(scene, surfaces, cameraSPhere, cubeMap, specularCubeMap);





	/*sphere_S = Vector3(0, 0, 0);
	sphere_r = 1.0f;
	
	renderPhong(scene, surfaces, cameraSPhere, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);*/

	//renderRenderingEqua(scene, surfaces, cameraSPaceShip, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);

	//TODOH
	/////////ENDSHADERS


	//SEMPROJEKT
	/////*float roughness = 1;
	////int numSamples = 5;
	////renderSpecularIBL(scene, surfaces, cameraSPaceShip, roughness, cubeMap, Vector3(-400.0f, -500.0f, 370.0f), numSamples);*/


	//cv::waitKey(0);

	// TODO *** ray tracing ****
	test(scene, surfaces);

	rtcDeleteScene(scene); // zrušení Embree scény

	SafeDeleteVectorItems<Material *>(materials);
	SafeDeleteVectorItems<Surface *>(surfaces);

	rtcDeleteDevice(device); // Embree zařízení musíme také uvolnit před ukončením aplikace

	return 0;
}
