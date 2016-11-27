#include "stdafx.h"

void rtc_error_function( const RTCError code, const char * str )
{
	printf( "ERROR in Embree: %s\n", str );
	exit( 1 );
}

RTCError check_rtc_or_die( RTCDevice & device )
{
	const RTCError error = rtcDeviceGetError( device );

	if ( error != RTC_NO_ERROR )
	{
		printf( "ERROR in Embree: " );

		switch ( error )
		{
		case RTC_UNKNOWN_ERROR:


			printf( "An unknown error has occurred." );
			break;

		case RTC_INVALID_ARGUMENT:
			printf( "An invalid argument was specified." );
			break;

		case RTC_INVALID_OPERATION:
			printf( "The operation is not allowed for the specified object." );
			break;

		case RTC_OUT_OF_MEMORY:
			printf( "There is not enough memory left to complete the operation." );
			break;

		case RTC_UNSUPPORTED_CPU:
			printf( "The CPU is not supported as it does not support SSE2." );
			break;

		case RTC_CANCELLED:
			printf( "The operation got cancelled by an Memory Monitor Callback or Progress Monitor Callback function." );
			break;
		}

		fflush( stdout );
		exit( 1 );
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

void filter_intersection( void * user_ptr, Ray & ray )
{
	/*  All hit information inside the ray is valid.
		The filter function can reject a hit by setting the geomID member of the ray to
	    RTC_INVALID_GEOMETRY_ID, otherwise the hit is accepted.The filter function is not
		allowed to modify the ray input data (org, dir, tnear, tfar), but can modify
		the hit data of the ray( u, v, Ng, geomID, primID ). */

	Surface * surface = reinterpret_cast<Surface *>( user_ptr );	
	printf( "intersection of: %s, ", surface->get_name().c_str() );
	const Vector3 p = ray.eval( ray.tfar );
	printf( "at: %0.3f (%0.3f, %0.3f, %0.3f)\n", ray.tfar, p.x, p.y, p.z );
	
	ray.geomID = RTC_INVALID_GEOMETRY_ID; // reject hit
}

void filter_occlusion( void * user_ptr, Ray & ray )
{
	/*  All hit information inside the ray is valid.
	The filter function can reject a hit by setting the geomID member of the ray to
	RTC_INVALID_GEOMETRY_ID, otherwise the hit is accepted.The filter function is not
	allowed to modify the ray input data (org, dir, tnear, tfar), but can modify
	the hit data of the ray( u, v, Ng, geomID, primID ). */

	Surface * surface = reinterpret_cast<Surface *>( user_ptr );	
	printf( "occlusion of: %s, ", surface->get_name().c_str() );
	const Vector3 p = ray.eval( ray.tfar );
	printf( "at: %0.3f (%0.3f, %0.3f, %0.3f)\n", ray.tfar, p.x, p.y, p.z );

	ray.geomID = RTC_INVALID_GEOMETRY_ID; // reject hit
}

#pragma region StareCviceni
int test( RTCScene & scene, std::vector<Surface *> & surfaces )
{
	// --- test rtcIntersect -----
	Ray rtc_ray = Ray( Vector3( -1.0f, 0.1f, 0.2f ), Vector3( 2.0f, 0.0f, 0.0f ), 0.0f );		
	//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
	//rtc_ray.tnear = 0.6f;
	//rtc_ray.tfar = 2.0f;
	rtcIntersect( scene, rtc_ray ); // find the closest hit of a ray segment with the scene
	// pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
	// u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

	if ( rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID )
	{
		Surface * surface = surfaces[rtc_ray.geomID];
		Triangle & triangle = surface->get_triangle( rtc_ray.primID );
		//Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

		// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
		const Vector3 p = rtc_ray.eval( rtc_ray.tfar );
		Vector3 geometry_normal = -Vector3( rtc_ray.Ng ); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
		geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
		const Vector3 normal = triangle.normal( rtc_ray.u, rtc_ray.v );
		const Vector2 texture_coord = triangle.texture_coord( rtc_ray.u, rtc_ray.v );

		
	}

	// --- test rtcOccluded -----
	rtc_ray = Ray( Vector3( -1.0f, 0.1f, 0.2f ), Vector3( 1.0f, 0.0f, 0.0f ) );
	//rtc_ray.tfar = 1.5;	
	rtcOccluded( scene, rtc_ray ); // determining if any hit between a ray segment and the scene exists
	// po volání rtcOccluded je nastavena pouze hodnota geomID, ostatni jsou nezměněny
	if ( rtc_ray.geomID == 0 )
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

				src_8uc3_img.at<cv::Vec3f>(y, x) =cv::Vec3f(v[2], v[1], v[0]);


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

				src_8uc3_img.at<cv::Vec3f>(y, x) = LambertShader(cv::Vec3f(normal.x, normal.y, normal.z), lightPosition,cv::Vec3f(point.x, point.y, point.z));


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

#pragma region Phong

bool isInShadow(Vector3 point, Vector3 lightPosition, RTCScene & scene)
{
	Vector3 sizeVector = (lightPosition - point);

	float tfar = sqrt(SQR(sizeVector.x) + SQR(sizeVector.y) + SQR(sizeVector.z));


	Ray rtc_ray = Ray(point, sizeVector, 0.01f, tfar);

	rtcOccluded(scene, rtc_ray);


	return rtc_ray.geomID == RTC_INVALID_GEOMETRY_ID;
}

Vector3 PhongShader(Vector3 normal, Vector3 lightPosition, Vector3 point, Vector3 eyeDirection, Vector3 specularColor, RTCScene & scene, Material* material, Vector2 textureCoord, bool wantShadow)
{
	Vector3 L = (lightPosition - point);
	L.Normalize();
	Vector3 E = eyeDirection;

	Vector3 H = (E + L);
	H.Normalize();


	Vector3 retVector = Vector3(0.1f, 0.1f, 0.1f);
	bool inShadow = isInShadow(point, lightPosition, scene);
	Vector3 diffuseColor = material->diffuse;

	
	Texture * diff_tex = material->get_texture(Material::kDiffuseMapSlot);


	if (diff_tex != NULL)
	{
		Color4 diff_texel = diff_tex->get_texel(textureCoord.x, textureCoord.y);
		diffuseColor = Vector3(diff_texel.r, diff_texel.g, diff_texel.b);
	}



	Vector3 color = L.DotProduct(normal) * diffuseColor + material->specular * std::pow(H.DotProduct(normal), 4);

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

Vector3 phongRecursion(int depth, Ray rtc_ray, Vector3 lightPosition, Vector3 eyePosition,RTCScene & scene, std::vector<Surface *> & surfaces, CubeMap cubeMap)
{
	//if (depth == 4) // konec rekurze
	//{
	//	Surface * surfad = surfaces[rtc_ray.geomID];
	//	Triangle & triangle = surfa->get_triangle(rtc_ray.primID);
	//	return PhongShader(n, lightPosition, rtc_ray.eval(rtc_ray.tfar), eyePosition - (rtc_ray.eval(rtc_ray.tfar)), surfaces[0]->get_material->specular, scene, surfaces[0]->get_material, triangle.normal(rtc_ray.u, rtc_ray.v), true);
	//	//Vector3 v = Vector3(1, 0, 0); //surfaces. Vector3(1, 1, 1);
	//	//return v;
	//}

	rtcIntersect(scene, rtc_ray);

	


	if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		Surface * surfa = surfaces[rtc_ray.geomID];
		Triangle & triangle = surfa->get_triangle(rtc_ray.primID);
		////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

		//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
		//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
		//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
		//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu

		Vector3 n = triangle.normal(rtc_ray.u, rtc_ray.v);
		n.Normalize();
		


		Vector2 tuv = triangle.texture_coord(rtc_ray.u, rtc_ray.v);


		const Vector3 p = rtc_ray.eval(rtc_ray.tfar);

		////////////Vector3 L = (lightPosition - point);
		////////////L.Normalize();
		////////////Vector3 E = eyeDirection;

		////////////Vector3 H = (E + L);
		////////////H.Normalize();

		Material * mtl = surfa->get_material();

		if (depth == 5)
		{
			return PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);
			//Color4 col = cubeMap.GetTexel(Vector3(rtc_ray.dir));
			//return Vector3(col.r, col.g, col.b);
		}


		if (false)//(mtl->ior == 1)
		{

			Vector3 color = PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, true);
			Vector3 rayDir = -Vector3(rtc_ray.dir);
			rayDir.Normalize();
			Vector3 reflectedDir = 2 * (n.DotProduct(rayDir)) * n - rayDir;

			Ray reflected(p, reflectedDir, 0.01f);

			////////////Vector3 retVector = Vector3(0.1f, 0.1f, 0.1f);

			////////////Vector3 isInShadowVector = isInShadow(point, lightPosition, scene, surfaces) ? Vector3(0.5, 0.5, 0.5) * cos(L.DotProduct(normal)) + specularColor * cos(H.DotProduct(normal)) : Vector3(1, 1, 1);
			////////////isInShadowVector.Normalize();


			

			if (!isInShadow(p, lightPosition, scene) && rtc_ray.ior == 1)
			{
				Vector3 reflected_rec = phongRecursion(depth + 1, reflected, lightPosition, p, scene, surfaces, cubeMap);

			

				color += 0.3f * reflected_rec;// + T * retracted_rec;
			}

			////////////retVector = retVector + isInShadowVector;

			////////////return cv::Vec3f(retVector.x, retVector.y, retVector.z);

			//return phongRecursion(depth + 1, normal, lightPosition, p, eyePosition, specularColor, scene, surfaces, cubeMap);
			return color;
			//return PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);
		}
		else // toto je sklo
		{
			Vector3 rf = eyePosition -p ;
		


			n = n.DotProduct(rf) >= 0 ? n: -n;
			
			
			
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

			float cos_O2 = n.DotProduct(rf);
			float cos_O1 = sqrt(1 - SQR(n1 / n2) * (1 - SQR(cos_O2)));


			Vector3 rr = (n1 / n2) * rf - ((n1 / n2) * cos_O2 + cos_O1) * n;
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

			rf.Normalize();

			float R = 1;
			float T = 0;
			float n1cosi = n1 * abs( lr.DotProduct(n));
			float n1cost = n1 * abs((rf).DotProduct(n));
			float n2cosi = n2 * abs(lr.DotProduct(n));
			float n2cost = n2 * abs((rf).DotProduct(n));
			float Rs = pow((n1cosi - n2cost) / (n1cosi + n2cost), 2);
			float Rp = pow((n1cost - n2cosi) / (n1cost + n2cosi), 2);
			R = (Rs + Rp) * 0.5f;
			T = 1 - R;


		/*	float R_0 = pow((n1 - n2) / (n1 + n2), 5);

			float cos_fi;
			cos_fi = n.DotProduct(rf);
			if (cos_fi < 0)
			{
				cos_fi = (-n).DotProduct(rf);
			}


			float R_fi = R_0 + (1 - R_0) * pow(1 - cos_fi, 5);

			R = R_fi;

			T = 1 - R_fi;*/

			return //PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false)*0.1f+ 
				phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap) * mtl->diffuse
				;//+ phongRecursion(depth + 1, reflected, lightPosition, p, scene, surfaces, cubeMap) * R;

		
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
	//TODOH
	cv::Mat src_8uc3_img(480, 640, CV_32FC3);

#pragma omp parallel for
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);

			Vector3 re = phongRecursion(0, rtc_ray, Vector3(lightPosition[0], lightPosition[1], lightPosition[2]), camera.view_from(), scene, surfaces, cubeMap);
			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);
			

		}
	}

	cv::imshow("phong", src_8uc3_img); // display image
	cvMoveWindow("phong", 10, 10);
	return 0;
}

#pragma endregion

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
	return 1.0;
	//return 0.5 / M_PI;
}

//Udelat     podle kompendia
// normal -> omega_i
Vector3 rr_Omega_i(Vector3 normal)
{
	bool notInNormalHemishpere = true;
	Vector3 retval = Vector3(0, 0, 0);

	double r1 = fRand(0.0, 1.0);
	double r2 = fRand(-1.0, 1.0);

	double fi = 2 * M_PI * r1;
	double tau = acos(r2);


	retval.x = cos(2 * M_PI*r1) * sqrt(1 - SQR(r2));
	retval.y = cos(2 * M_PI*r1) * sqrt(1 - SQR(r2));
	retval.z = r2;


	if (normal.DotProduct(retval) < 0)
	{
		retval = -retval;
	}

	retval.Normalize();
	return retval;
}

//Pro nas experiment
// Li(omegai) = 1

double rr_Li(Vector3 omega_i)
{
	return 1;
}


Vector3 renderingEquation(Vector3 P, Vector3 direction, RTCScene & scene, std::vector<Surface *> & surfaces, int depth, CubeMap cubeMap)
{
	Ray ray = Ray(P, direction);
	ray.tnear = .1f;                         
	rtcOccluded(scene, ray);   

	Vector3 colorFromThisRay;
	if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		ray = Ray(P, direction);
		ray.tnear = .1f;                                                       
		rtcIntersect(scene, ray);

		
		Surface * surface = surfaces[ray.geomID];

		Vector3 material = surface->get_material()->diffuse;//Vector3(0.5f, 0.5f, 0.5f);           //TODOH - material surface.getmaterial!!

		Triangle & triangle = surface->get_triangle(ray.primID);

		// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
		const Vector3 p = ray.eval(ray.tfar);
		Vector3 geometry_normal = -Vector3(ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
		geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
		Vector3 normal = triangle.normal(ray.u, ray.v);

		if (normal.DotProduct(direction) < 0)
		{
			normal = -normal;
		}

		const Vector2 texture_coord = triangle.texture_coord(ray.u, ray.v);



		Vector3 materialSP = (surface->get_material())->specular;
		Vector3 material_DP = (surface->get_material())->diffuse;


		colorFromThisRay = Vector3(0, 0, 0);
		if (depth != 0)
		{
			Vector3 omega0 = -direction;
			Vector3 norm = normal;

			Vector3 Le = Vector3(0.0, 0.0, 0.0);    //Svetlo emtiovane z bodu p



			int N = 3;                              //Pocet paprsku pro integrovani Monte-Carlo
			Vector3 Suma = Vector3(0.0, 0.0, 0.0);
			for (int i = 0; i < N; i++)
			{
				Vector3 omegai = rr_Omega_i(norm);
				Vector3 Li = renderingEquation(p, omegai, scene, surfaces, depth - 1, cubeMap);
				double Fr = rr_Fr(omega0, omegai);
				double pdf = 1;         //Uniformni rozdeleni pdf
				Suma = Suma + (Li * Fr * (omegai.DotProduct(norm))) / pdf;
			}

			colorFromThisRay = Le + 1 / N * (Suma);
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
		Color4 cubeMapColor = cubeMap.GetTexel(Vector3(ray.dir[0], ray.dir[1], ray.dir[2]));
		colorFromThisRay = Vector3(cubeMapColor.b, cubeMapColor.g, cubeMapColor.r);
		return Vector3(1.0, 1.0, 1.0);
	}
	return colorFromThisRay;


}


int renderRenderingEqua(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap)
{
	//TODOH
	cv::Mat src_8uc3_img(480, 640, CV_32FC3);

#pragma omp parallel for
	for (int x = 0; x < 640; x++)
	{
		for (int y = 0; y < 480; y++)
		{

			Ray rtc_ray = camera.GenerateRay(x, y);



			

			int numOfsamples = 100;
			Vector3 re;
			for (int sample = 0; sample < numOfsamples; sample++)
			{
				re += renderingEquation(rtc_ray.org, rtc_ray.dir, scene, surfaces, 20, cubeMap);

			}
			re = re / numOfsamples;

			
			src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);
			

		}
	}




	cv::imshow("renderingEquation", src_8uc3_img); // display image
	cvMoveWindow("renderingEquation", 10, 10);
	return 0;
}


#pragma endregion  


#pragma region Projekt UE4


 

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

float G_Smith(Vector3 N, Vector3 H, Vector3 V, Vector3 L)
{

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

				Vector3 res = SpecularIBL(Vector3(0.5f,0.5f,0.5f), roughness, normal, V, cubeMap, numSamples);

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

#pragma region 

double fRandom(double fMin, double fMax)
{
	
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}


Vector3 Get_Omega_i(Vector3 normal, float alpha)
{
	
	//bool notInNormalHemishpere = true;
	//Vector3 retval = Vector3(0, 0, 0);

	//double r1 = fRandom(0.0f, 1 - roughness);
	//double r2 = fRandom(0.0f, 1 - roughness);

	//double fi = 2 * M_PI * r1;
	//double tau = acos(r2);


	//retval.x = fi;//cos(2 * M_PI * r1) * sqrt(1 - SQR(r2));
	//retval.y = tau;//cos(2 * M_PI*r1) * sqrt(1 - SQR(r2));
	//retval.z = r2;

	//if (normal.DotProduct(retval) < 0)
	//{
	//	retval = -retval;
	//}

	//retval.Normalize();
	//return retval;


	float eps1 = fRandom(0.0f, 1.0f);
	float eps2 = fRandom(0.0f, 1.0f);



	float r = 1;
	float theta = atan((alpha* sqrt(eps1)) / sqrt(1 - eps1));
	float fi = 2 * M_PI * eps2;

	Vector3 sampleVector = Vector3(0, 0, 0);

	sampleVector.x = r * cos(theta) * sin(fi);
	sampleVector.y = r * sin(theta) * sin(fi);
	sampleVector.z = r * cos(fi);

	return sampleVector;

}

float chiGGX(float v)
{
	return v > 0 ? 1 : 0;
}

float GGX_Distribution(Vector3 n, Vector3 h, float alpha)
{
	h.Normalize();

	float NoH = n.DotProduct(h);
	float alpha2 = alpha * alpha;
	float NoH2 = NoH * NoH;
	float den = NoH2 * (alpha2 + ((1 - NoH2)/ NoH2));
	return (chiGGX(NoH) * alpha2) / (M_PI * den * den);
}

float GGX_PartialGeometryTerm(Vector3 v, Vector3 n, Vector3 h, float alpha)
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


Vector3 Fresnel_Schlick(float cosT, Vector3 F0)
{
	return F0 + (Vector3(1,1,1) - F0) * pow(1 - cosT, 5);
}

Vector3 lerp(Vector3 v0, Vector3 v1, float t) {
	//Vector3 vdiff = v1 - v0;
	//vdiff = t * vdiff;
	//return vdiff + v0;
	return (1 - t)*v0 + t*v1;
}
Vector3 reflect(Vector3 normal, Vector3 viewVec)
{
	return 2 * (normal.DotProduct(viewVec)) * normal - viewVec;
}

Matrix4x4 GenerateFrame(Vector3 input)
{
	Vector3 x;
	Vector3 y;
	if (abs(input.x) > 0.99) x = Vector3(0, 1, 0); else x = Vector3(1, 0, 0);
	y = (input.CrossProduct(x));
	y.Normalize();

	x = y.CrossProduct(input);
	return Matrix4x4(
		x.x, y.x, input.x, 0,
		x.y, y.y, input.y, 0,
		x.z, y.z, input.z, 0,
		0, 0, 0, 0);
}

Vector3 GenerateGGXsampleVector(int i, int SamplesCount, float roughness, Vector3 normal)
{
	//TODOLAST
	return Get_Omega_i(normal, roughness);
}

Vector3 GGX_Specular(CubeMap cubeMap, Vector3 normal, Vector3 rayDir, float roughness, Vector3 F0, Vector3 *kS)
{
	Vector3 reflectionVector = reflect(-rayDir, normal);
	Matrix4x4 worldFrame = GenerateFrame(reflectionVector);
	Vector3 radiance = Vector3(0,0,0);
	float  NoV = saturate(normal.DotProduct(rayDir));
	
	int SamplesCount = 100;

	for (int i = 0; i < SamplesCount; i++)
	{
		// Generate a sample vector in some local space
		Vector3 sampleVector = GenerateGGXsampleVector(i, SamplesCount, roughness, normal);
		// Convert the vector in world space
		sampleVector = worldFrame * sampleVector;

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
		radiance += Vector3(cubeMap.GetTexel(sampleVector).data) * geometry * fresnel * sinT / denominator;
	}

	// Scale back for the samples count
	*kS = *kS / SamplesCount;
	(*kS).x = saturate((*kS).x);
	(*kS).y = saturate((*kS).y);
	(*kS).z = saturate((*kS).z);

	return radiance / SamplesCount;
}


#define SQR( x ) ( ( x ) * ( x ) )
#define DEG2RAD( x ) ( ( x ) * M_PI / 180.0 )
#define MY_MIN( X, Y ) ( ( X ) < ( Y ) ? ( X ) : ( Y ) )


void geom_dist(IplImage* src, IplImage* dst, bool bili, double K1 = 1.0, double K2 = 1.0)
{
	double cu = src->width * 0.5;
	double cv = src->height * 0.5;
	double R = sqrt(cu*cu + cv*cv);


	for (int yn = 0; yn < src->height; yn++) {
		uchar* ptr_dst = (uchar*)(dst->imageData + (int)yn * dst->widthStep);
		for (int xn = 0; xn < src->width; xn++) {

			double x = (xn - cu) / R;
			double y = (yn - cv) / R;
			double sqrR = x*x + y*y;
			double fi_sqrR = 1 + K1 * sqrR + K2 * sqrR*sqrR;

			double xd = (xn - cu) * (1 / fi_sqrR) + cu;
			double yd = (yn - cv) * (1 / fi_sqrR) + cv;

			uchar* ptr_src = (uchar*)(src->imageData + (int)yd * src->widthStep);


			ptr_dst[3 * (int)xn + 0] = ptr_src[3 * (int)xd + 0]; //Set B (BGR format)
			ptr_dst[3 * (int)xn + 1] = ptr_src[3 * (int)xd + 1]; //Set G (BGR format)
			ptr_dst[3 * (int)xn + 2] = ptr_src[3 * (int)xd + 2]; //Set R (BGR format)
		}
	}
}


//int K1 = 3.0, K2 = 1.0;
RTCScene scene = rtcDeviceNewScene(RTCDevice(), RTC_SCENE_STATIC | RTC_SCENE_HIGH_QUALITY, RTC_INTERSECT1/* | RTC_INTERPOLATE*/);
std::vector<Surface *> & surfaces = std::vector<Surface *>();
Camera & camera = Camera(10,10, Vector3(0,0,0), Vector3(0, 0, 0), 0);
cv::Vec3f lightPosition = cv::Vec3f();
CubeMap cubeMap = CubeMap("a");
IplImage *img_geom;
int _roughness;
int _metallic;
int _ior;




int projRenderGGX_Distribution_geom(IplImage * img_geom, float roughness, float metallic, float ior)
{


#pragma omp parallel for

	for (int y = 0; y < img_geom->height; y++)
	{
		uchar* ptr_dst = (uchar*)(img_geom->imageData + (int)y * img_geom->widthStep);
		for (int x = 0; x < img_geom->width; x++)
		{
			Vector3 ret;

			Ray rtc_ray = camera.GenerateRay(x, y);

			//Vector3 re = phongRecursion(0, rtc_ray, Vector3(lightPosition[0], lightPosition[1], lightPosition[2]), camera.view_from(), scene, surfaces, cubeMap);




			rtcIntersect(scene, rtc_ray);



			if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				Surface * surface = surfaces[rtc_ray.geomID];
				Triangle & triangle = surface->get_triangle(rtc_ray.primID);

				Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);

				Vector3 rayDir = Vector3(rtc_ray.dir[0], rtc_ray.dir[1], rtc_ray.dir[2]);
				rayDir.Normalize();

				Vector3 h = rayDir + normal;
				h.Normalize();


				//if (normal.DotProduct(rayDir) < 0)
				//{
				//	normal = -normal;
				//}

				// F = Fresnel_Schlick(abs(rayDir.DotProduct(normal)), F0_v);
				// D = GGX_Distribution(normal, h, alpha) ;
				// G = GGX_PartialGeometryTerm(rayDir, normal, h, alpha);

				Material *mtl = surface->get_material();


				//Vector3 surfa = mtl->shininess;//cubeMap.GetTexel(rayDir);


				ior = 1;//mtl->ior;//1 + mtl->ior;
				roughness = 0.5f;
				//float roughness = saturate(mtl->shininess - EPSILON) + EPSILON;
				metallic = 0.1f;//1 - alpha;



				float F0_f = abs((1.0 - ior) / (1.0 + ior));
				Vector3 F0 = Vector3(F0_f, F0_f, F0_f);
				F0 = F0 * F0;
				F0.Normalize();
				F0 = lerp(F0, mtl->diffuse, metallic);


				Vector3 ks = Vector3(0, 0, 0);
				Vector3 specular = GGX_Specular(cubeMap, normal, rayDir, roughness, F0, &ks);
				Vector3 kd = (Vector3(1, 1, 1) - ks) * (1 - metallic);

				Vector3 irradiance = Vector3(cubeMap.GetTexel(normal).data);
				Vector3 diffuse = mtl->diffuse * irradiance;

				ret = Vector3(kd * diffuse + ks * specular);



				//float D = GGX_Distribution(normal, h, alpha);
				//float G = GGX_PartialGeometryTerm(rayDir, normal, h, alpha);

				//int numSamples = 500;

				//Vector3 ret;
				//Vector3 omega_i;
				//Vector3 omega_o = rayDir;
				//float OIoN;
				//float OOoN = abs(omega_o.DotProduct(h));
				//

				//for (int i = 0; i < numSamples; i++)
				//{
				//	omega_i = rr_Omega_i(normal);
				//	OIoN = abs(omega_i.DotProduct(h));

				//	ret += (F * D * G) / (4 * OIoN * OOoN);
				//}

				//
				//ret = ret / numSamples;




				//ret = (F * D * G) / (4 * OIoN * OOoN); // GGX_Distribution(normal, h, alpha) ;




				//ret = 1 ret;




			}
			else
			{
				Color4 col = cubeMap.GetTexel(Vector3(rtc_ray.dir));
				ret = Vector3(col.data);
			}

			ptr_dst[3 * (int)x + 0] = ret.x; // ptr_src[3 * (int)xd + 0]; //Set B (BGR format)
			ptr_dst[3 * (int)x + 1] = ret.y;//ptr_src[3 * (int)xd + 1]; //Set G (BGR format)
			ptr_dst[3 * (int)x + 2] = ret.z;//ptr_src[3 * (int)xd + 2]; //Set R (BGR format)

			
		}
	}

		//for (int x = 0; x < 640; x++)
		//{
		//	for (int y = 0; y < 480; y++)
		//	{

		//		



		//		/*src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);*/


		//	}
		//}

		//std::string str = "GGX_Distribution metallic(" + std::to_string(metallic) + ")  roughness(" + std::to_string(alpha) + ")";
		//cv::imshow(str, src_8uc3_img); // display image

		//cv::moveWindow(str, 10, 50);
	
	return 0;
}

void on_change(int id)
{
	projRenderGGX_Distribution_geom(img_geom, ((float)_roughness) / 100.0f, ((float)_metallic) / 100.0f, ((float)_ior) / 100.0f);//, false, K1 / 100.0, K2 / 100.0);
	cvShowImage("Geom Dist", img_geom);
	cvMoveWindow("Geom Dist", 10, 50);

}

int renderGeomDist(RTCScene & _scene, std::vector<Surface *> & _surfaces, Camera & _camera, cv::Vec3f _lightPosition, CubeMap _cubeMap)
{
	/*img = cvLoadImage("images/distorted_window.jpg", CV_LOAD_IMAGE_COLOR);
	if (!img)
	{
	printf("Unable to load image!\n");
	exit(-1);
	}*/

	/*cvNamedWindow("Original Image");
	cvShowImage("Original Image", img);*/

	scene = _scene;
	surfaces = _surfaces;
	camera = _camera;
	lightPosition = _lightPosition;
	cubeMap = _cubeMap;

	_roughness = 1.0f;
	_metallic = 0.5f;
	_ior = 1.0f;

	img_geom = cvCreateImage(cvSize(640, 480), 32, 3);

	//cvSet(img_geom, cvScalar(0, 0, 0));

	projRenderGGX_Distribution_geom(img_geom,((float)_roughness) / 100.0f, ((float)_metallic) / 100.0f, ((float)_ior) / 100.0f);//, false, K1 / 100.0, K2 / 100.0);

	cvNamedWindow("Geom Dist");
	cvShowImage("Geom Dist", img_geom);
	cvMoveWindow("Geom Dist", 10, 50);

	cvCreateTrackbar("roughness", "Geom Dist", &_roughness, 100, on_change);
	cvCreateTrackbar("metallic", "Geom Dist", &_metallic, 100, on_change);
	cvCreateTrackbar("ior", "Geom Dist", &_ior, 2000, on_change);


	cvWaitKey(0);

	return 0;
}

int projRenderGGX_Distribution(RTCScene & scene, std::vector<Surface *> & surfaces, Camera & camera, cv::Vec3f lightPosition, CubeMap cubeMap)
{
	//TODOH
	//float alpha = 0.5f;

	for (float alpha = 0.1f; alpha <= 1.1f; alpha += 0.2f)
	{
		alpha = saturate(alpha);
		cv::Mat src_8uc3_img(480, 640, CV_32FC3);

		float roughness = 0.5f;
		float ior = 1;
		float metallic = alpha;


#pragma omp parallel for
		for (int x = 0; x < 640; x++)
		{
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

					Vector3 rayDir = Vector3(rtc_ray.dir[0], rtc_ray.dir[1], rtc_ray.dir[2]);
					rayDir.Normalize();

					Vector3 h = rayDir + normal;
					h.Normalize();


					//if (normal.DotProduct(rayDir) < 0)
					//{
					//	normal = -normal;
					//}

					// F = Fresnel_Schlick(abs(rayDir.DotProduct(normal)), F0_v);
					/*float D = GGX_Distribution(normal, h, alpha) ;
					ret = Vector3(D, D, D);*/
					// G = GGX_PartialGeometryTerm(rayDir, normal, h, alpha);

					Material *mtl = surface->get_material();


					//Vector3 surfa = mtl->shininess;//cubeMap.GetTexel(rayDir);

					
					//float ior = 1;//mtl->ior;//1 + mtl->ior;
					//roughness = alpha;
					//float roughness = saturate(mtl->shininess - EPSILON) + EPSILON;
					//metallic = 0.1f;//1 - alpha;



					float F0_f = abs((1.0 - ior) / (1.0 + ior));
					Vector3 F0 = Vector3(F0_f, F0_f, F0_f);
					F0 = F0 * F0;
					F0.Normalize();
					F0 = lerp(F0, mtl->diffuse, metallic);

					
					Vector3 ks = Vector3(0,0,0);
					Vector3 specular = GGX_Specular(cubeMap, normal, rayDir, roughness, F0, &ks);
					Vector3 kd = (Vector3(1,1,1) - ks) * (1 - metallic);
					
					Vector3 irradiance =  Vector3( cubeMap.GetTexel(normal).data);
					Vector3 diffuse = mtl->diffuse * irradiance;

					ret = Vector3(kd * diffuse + specular);

					src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(ret.z, ret.y, ret.x);

					
					


				}
				else
				{
					Color4 col = cubeMap.GetTexel(Vector3(rtc_ray.dir));
					src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(col.b, col.g, col.r);
					
				}



				/*src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(re.z, re.y, re.x);*/


			}
		}

		std::string str = "GGX_Distribution metallic(" + std::to_string(metallic) + ")  roughness(" + std::to_string(roughness) + ")";
		cv::imshow(str, src_8uc3_img); // display image

		cv::moveWindow(str, 10, 50);
	}
	return 0;
}


/*
Seznam úkolů:

1, Doplnit TODO v souboru tracing.cpp.
*/

int main( int argc, char * argv[] )
{
	printf( "PG1, (c)2011-2016 Tomas Fabian\n\n" );	

	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON ); // Flush to Zero, Denormals are Zero mode of the MXCSR
	_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
	RTCDevice device = rtcNewDevice( NULL ); // musíme vytvořit alespoň jedno Embree zařízení		
	check_rtc_or_die( device ); // ověření úspěšného vytvoření Embree zařízení
	rtcDeviceSetErrorFunction( device, rtc_error_function ); // registrace call-back funkce pro zachytávání chyb v Embree	

	std::vector<Surface *> surfaces;
	std::vector<Material *> materials;

	// načtení geometrie
	//if ( LoadOBJ( "../../data/6887_allied_avenger.obj", Vector3( 0.5f, 0.5f, 0.5f ), surfaces, materials ) < 0 )
	//{
	//	return -1;
	//}

	if (LoadOBJ("../../data/geosphere.obj", Vector3(0.5f, 0.5f, 0.5f), surfaces, materials) < 0)
	{
		return -1;
	}
	
	// vytvoření scény v rámci Embree
	RTCScene scene = rtcDeviceNewScene( device, RTC_SCENE_STATIC | RTC_SCENE_HIGH_QUALITY, RTC_INTERSECT1/* | RTC_INTERPOLATE*/ );
	// RTC_INTERSECT1 = enables the rtcIntersect and rtcOccluded functions

	// nakopírování všech modelů do bufferů Embree
	for ( std::vector<Surface *>::const_iterator iter = surfaces.begin();
		iter != surfaces.end(); ++iter )
	{
		Surface * surface = *iter;
		unsigned geom_id = rtcNewTriangleMesh( scene, RTC_GEOMETRY_STATIC,
			surface->no_triangles(), surface->no_vertices() );

		//rtcSetUserData, rtcSetBoundsFunction, rtcSetIntersectFunction, rtcSetOccludedFunction,
		rtcSetUserData( scene, geom_id, surface );
		//rtcSetOcclusionFilterFunction, rtcSetIntersectionFilterFunction		
		//rtcSetOcclusionFilterFunction( scene, geom_id, reinterpret_cast< RTCFilterFunc >( &filter_occlusion ) );
		//rtcSetIntersectionFilterFunction( scene, geom_id, reinterpret_cast< RTCFilterFunc >( &filter_intersection ) );

		// kopírování samotných vertexů trojúhelníků
		embree_structs::Vertex * vertices = static_cast< embree_structs::Vertex * >(
			rtcMapBuffer( scene, geom_id, RTC_VERTEX_BUFFER ) );

		for ( int t = 0; t < surface->no_triangles(); ++t )
		{
			for ( int v = 0; v < 3; ++v )
			{
				embree_structs::Vertex & vertex = vertices[t * 3 + v];

				vertex.x = surface->get_triangles()[t].vertex( v ).position.x;
				vertex.y = surface->get_triangles()[t].vertex( v ).position.y;
				vertex.z = surface->get_triangles()[t].vertex( v ).position.z;
			}
		}

		rtcUnmapBuffer( scene, geom_id, RTC_VERTEX_BUFFER );

		// vytváření indexů vrcholů pro jednotlivé trojúhelníky
		embree_structs::Triangle * triangles = static_cast< embree_structs::Triangle * >(
			rtcMapBuffer( scene, geom_id, RTC_INDEX_BUFFER ) );

		for ( int t = 0, v = 0; t < surface->no_triangles(); ++t )
		{
			embree_structs::Triangle & triangle = triangles[t];

			triangle.v0 = v++;
			triangle.v1 = v++;
			triangle.v2 = v++;
		}

		rtcUnmapBuffer( scene, geom_id, RTC_INDEX_BUFFER );

		/*embree_structs::Normal * normals = static_cast< embree_structs::Normal * >(
			rtcMapBuffer( scene, geom_id, RTC_USER_VERTEX_BUFFER0 ) );
			rtcUnmapBuffer( scene, geom_id, RTC_USER_VERTEX_BUFFER0 );*/
	}

	rtcCommit( scene );	

	// vytvoření kamery
	//Camera camera = Camera( 640, 480, Vector3( -1.5f, -3.0f, 2.0f )*0.8f,
	//	Vector3( 0.0f, 0.5f, 0.5f ), DEG2RAD( 40.0f ) );
	//camera.Print();

	//Camera cameraNum = Camera(640, 480, Vector3(-400.0f, -500.0f, 370.0f),
	//	Vector3(70.0f, -40.f, 5.0f), DEG2RAD(42.185f));

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

	Camera cameraSPaceShip = Camera(640, 480, Vector3(-140.0f, -175.0f, 110.0f), Vector3(0.0f, 0.0f, 40.0f), DEG2RAD(42.185f));

	cameraSPaceShip.Print();

	//cv::Mat src_8uf3_spaceship(480, 640, CV_32FC3);

	CubeMap cubeMap = CubeMap::CubeMap("../../data/tenerife");
	//CubeMap cubeMap = CubeMap::CubeMap("../../data/yokohama3");

	//////Camera cameraBackground = Camera(640, 480, Vector3(0.0f, 0.0f, 0.0f),
	//////	Vector3(1,0,0), DEG2RAD(120.0f));
	//////renderBackground(scene, surfaces, cameraSPaceShip, cubeMap, cv::Vec3f(-400.0f, -500.0f, 370.0f));

	//renderNormal(scene, surfaces, cameraSPaceShip, src_8uf3_spaceship);
	//renderLambert(scene, surfaces, cameraSPaceShip, src_8uf3_spaceship, cv::Vec3f(-400.0f, -500.0f, 370.0f));

	Camera cameraSPhere = Camera(640, 480, Vector3(3.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f));

	renderPhong(scene, surfaces, cameraSPhere, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);
	//renderRenderingEqua(scene, surfaces, cameraSPaceShip, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);

	//renderGeomDist(scene, surfaces, cameraSPaceShip, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);
	//projRenderGGX_Distribution(scene, surfaces, cameraSPhere, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);
	//TODOH
	/////////ENDSHADERS
	
	
	//SEMPROJEKT
	/////*float roughness = 1;
	////int numSamples = 5;
	////renderSpecularIBL(scene, surfaces, cameraSPaceShip, roughness, cubeMap, Vector3(-400.0f, -500.0f, 370.0f), numSamples);*/


	cv::waitKey(0);

	// TODO *** ray tracing ****
	//test( scene, surfaces );

	rtcDeleteScene( scene ); // zrušení Embree scény

	SafeDeleteVectorItems<Material *>( materials );
	SafeDeleteVectorItems<Surface *>( surfaces );

	rtcDeleteDevice( device ); // Embree zařízení musíme také uvolnit před ukončením aplikace

	return 0;
}
