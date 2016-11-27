#include "stdafx.h"

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

Vector3 phongRecursion(int depth, Ray rtc_ray, Vector3 lightPosition, Vector3 eyePosition, RTCScene & scene, std::vector<Surface *> & surfaces, CubeMap cubeMap)
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


		if (mtl->ior == 1)
		{

			//Vector3 color = PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, true);
			//Vector3 rayDir = -Vector3(rtc_ray.dir);
			//rayDir.Normalize();
			//Vector3 reflectedDir = 2 * (n.DotProduct(rayDir)) * n - rayDir;

			//Ray reflected(p, reflectedDir, 0.01f);

			//////////////Vector3 retVector = Vector3(0.1f, 0.1f, 0.1f);

			//////////////Vector3 isInShadowVector = isInShadow(point, lightPosition, scene, surfaces) ? Vector3(0.5, 0.5, 0.5) * cos(L.DotProduct(normal)) + specularColor * cos(H.DotProduct(normal)) : Vector3(1, 1, 1);
			//////////////isInShadowVector.Normalize();




			//if (!isInShadow(p, lightPosition, scene))
			//{
			//	Vector3 reflected_rec = phongRecursion(depth + 1, reflected, lightPosition, p, scene, surfaces, cubeMap);

			//

			//	color += 0.3f * reflected_rec;// + T * retracted_rec;
			//}

			//////////////retVector = retVector + isInShadowVector;

			//////////////return cv::Vec3f(retVector.x, retVector.y, retVector.z);

			////return phongRecursion(depth + 1, normal, lightPosition, p, eyePosition, specularColor, scene, surfaces, cubeMap);
			//return color;
			return PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);
		}
		else // toto je sklo
		{
			Vector3 rf = eyePosition - p;



			n = n.DotProduct(rf) >= 0 ? n : -n;



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

			return //PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false)*0.1f+ 
				phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap) * mtl->diffuse;

			//dle wiki
			//float r = rtc_ray.ior / mtl->ior;

			////float r = mtl->ior / rtc_ray.ior;
			////Vector3 l =   eyePosition - p;
			////float c = (-n).DotProduct(l);


			////


			////Vector3 v_refract = r * l + (r * c - (sqrt(1 - SQR(r) * (1 - SQR(c))))) * n;
			////Ray refracted(p, v_refract, 0.1f);
			////refracted.set_ior(mtl->ior);
			////return //PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false)*0.1f+
			////	//0.8f*
			////	phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap);

			////// Ray(p, p - eyePosition, 0.1f) 

			//return PhongShader(n, lightPosition, p, eyePosition - p, mtl->specular, scene, mtl, tuv, false);// phongRecursion(depth + 1, refracted, lightPosition, p, scene, surfaces, cubeMap);

			//return color;

			//return Vector3(1, 0, 0);
			//Vector3 retracted_rec = Vector3(0, 0, 0);
			//float R = 1;
			//float T = 0;
			//if (mtl->ior != -1)// {
			////if(mtl->ior > 0)
			//{
			//	Vector3 l = eyePosition-p ;
			//	float c = abs(l.DotProduct(-n));
			//	float r = (mtl->ior / rtc_ray.ior);
			//	float cos_O2 = sqrt(1 - r*r * (1 - c*c));
			//	//Vector3 retractDir = r * l + (r * c - sqrt(1 - r*r * (1 - c*c))) * normal;
			//	Vector3 retractDir = r * l + (c - r * cos_O2) * n;

			//	Ray retracted (p , retractDir, 0.5f);

			//	retracted_rec = phongRecursion(depth + 1, retracted, lightPosition, p, scene, surfaces, cubeMap);
			//		// TracePhong(Ray(GetPoint(ray, false), retractDir), deep + 1);

			//		float n1cosi = abs(rtc_ray.ior * c);
			//	float n1cost = abs(rtc_ray.ior * retractDir.DotProduct(n));
			//	float n2cosi = abs(mtl->ior * c);
			//	float n2cost = abs(mtl->ior * retractDir.DotProduct(n));

			//	float Rs = pow((n1cosi - n2cost) / (n1cosi + n2cost), 2);
			//	float Rp = pow((n1cost - n2cosi) / (n1cost + n2cosi), 2);
			//	R = (Rs + Rp) * 0.5f;
			//	T = 1 - R;
			//}
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


			// --- test rtcIntersect -----
			//Ray rtc_ray = Ray(Vector3(-1.0f, 0.1f, 0.2f), Vector3(2.0f, 0.0f, 0.0f), 0.0f);
			//Ray rtc_ray = Ray( Vector3( 4.0f, 0.1f, 0.2f ), Vector3( -1.0f, 0.0f, 0.0f ) );
			//rtc_ray.tnear = 0.6f;
			////rtc_ray.tfar = 2.0f;
			//rtcIntersect(scene, rtc_ray); // find the closest hit of a ray segment with the scene
			//							  // pri filtrovani funkce rtcIntersect jsou pruseciky prochazeny od nejblizsiho k nejvzdalenejsimu
			//							  // u funkce rtcOccluded jsou nalezene pruseciky vraceny v libovolnem poradi

			//							  //src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(0, 0, 0);



			//if (rtc_ray.geomID != RTC_INVALID_GEOMETRY_ID)
			//{
			//	Surface * surface = surfaces[rtc_ray.geomID];
			//	Triangle & triangle = surface->get_triangle(rtc_ray.primID);
			//	////Triangle * triangle2 = &( surface->get_triangles()[rtc_ray.primID] );

			//	//// získání souřadnic průsečíku, normál, texturovacích souřadnic atd.
			//	//const Vector3 p = rtc_ray.eval(rtc_ray.tfar);
			//	//Vector3 geometry_normal = -Vector3(rtc_ray.Ng); // Ng je nenormalizovaná normála zasaženého trojúhelníka vypočtená nesouhlasně s pravidlem pravé ruky o závitu
			//	//geometry_normal.Normalize(); // normála zasaženého trojúhelníka vypočtená souhlasně s pravidlem pravé ruky o závitu
			//	const Vector3 normal = triangle.normal(rtc_ray.u, rtc_ray.v);
			//	//const Vector2 texture_coord = triangle.texture_coord(rtc_ray.u, rtc_ray.v);

			//	const Vector3 point = rtc_ray.eval(rtc_ray.tfar);

			//	Vector3 re = phongRecursion(0, normal, Vector3(lightPosition[0], lightPosition[1], lightPosition[2]), point, camera.view_from(), Vector3(0.5f, 0.5f, 0.5f), scene, surfaces, cubeMap);


			//	

			//	
			//}
			//else
			//{
			//	Color4 tex = cubeMap.GetTexel(Vector3(rtc_ray.dir[0], rtc_ray.dir[1], rtc_ray.dir[2]));
			//	src_8uc3_img.at<cv::Vec3f>(y, x) = cv::Vec3f(tex.b, tex.g, tex.r);

			//}

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

float Saturate(float val)
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

		float NoV = Saturate(N.DotProduct(V));
		float NoL = Saturate(N.DotProduct(L));
		float NoH = Saturate(N.DotProduct(H));
		float VoH = Saturate(V.DotProduct(H));

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


/*
Seznam úkolů:

1, Doplnit TODO v souboru tracing.cpp.
*/

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
	if (LoadOBJ("../../data/6887_allied_avenger.obj", Vector3(0.5f, 0.5f, 0.5f), surfaces, materials) < 0)
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

	//////Camera cameraBackground = Camera(640, 480, Vector3(0.0f, 0.0f, 0.0f),
	//////	Vector3(1,0,0), DEG2RAD(120.0f));
	//////renderBackground(scene, surfaces, cameraSPaceShip, cubeMap, cv::Vec3f(-400.0f, -500.0f, 370.0f));

	//renderNormal(scene, surfaces, cameraSPaceShip, src_8uf3_spaceship);
	//renderLambert(scene, surfaces, cameraSPaceShip, src_8uf3_spaceship, cv::Vec3f(-400.0f, -500.0f, 370.0f));

	renderPhong(scene, surfaces, cameraSPaceShip, cv::Vec3f(-400.0f, -500, 370.0f), cubeMap);
	//TODOH
	/////////ENDSHADERS


	//SEMPROJEKT
	/////*float roughness = 1;
	////int numSamples = 5;
	////renderSpecularIBL(scene, surfaces, cameraSPaceShip, roughness, cubeMap, Vector3(-400.0f, -500.0f, 370.0f), numSamples);*/


	cv::waitKey(0);

	// TODO *** ray tracing ****
	test(scene, surfaces);

	rtcDeleteScene(scene); // zrušení Embree scény

	SafeDeleteVectorItems<Material *>(materials);
	SafeDeleteVectorItems<Surface *>(surfaces);

	rtcDeleteDevice(device); // Embree zařízení musíme také uvolnit před ukončením aplikace

	return 0;
}
