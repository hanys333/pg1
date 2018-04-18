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


CubeMap cubeMap = CubeMap::CubeMap("../../data/yokohama");
Camera camera = Camera(Camera(640, 480, Vector3(2.0f, 2.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f)));

ggx_distribution distr = ggx_distribution();

std::string strTest = "26";

void TestCountSamples()
{
	int countSamples = 50;

	std::string str = strTest;//"18";
	float roughness = -1.0f;
	distr.StartRender(camera, cubeMap, countSamples, GOLD, str, -1, roughness);
	distr.StartRender(camera, cubeMap, countSamples, IRON, str, -1, roughness);

}

void TestRoughness(GGXColor col)
{
	
	for (float roughness = 0.1f; roughness <= 1.1f; roughness += 0.2)
	{
		distr.StartRender(camera, cubeMap, 30, col, strTest + " RoughnesssTest", -1, roughness);
	}
}

void TestMetallic(GGXColor col)
{
	distr.StartRender(camera, cubeMap, 50, col, "METALLICTest", -1, -1, 0.1);
	distr.StartRender(camera, cubeMap, 50, col, "METALLICTest", -1, -1, 0.5);
	distr.StartRender(camera, cubeMap, 50, col, "METALLICTest", -1, -1, 1.0);
}

void JustTest(GGXColor col, int countSamples)
{
	distr.StartRender(camera, cubeMap, countSamples, col, "JustTest");
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


	//baseColor = Vector3(0.560, 0.570, 0.580); str = "IRON";// iron
	//baseColor = Vector3(0.972, 0.960, 0.915); str = "SILVER"; // silver
	//baseColor = Vector3(0.913, 0.921, 0.925); str = "ALUMINIUM"; // aluminium
	//baseColor = Vector3(1.000, 0.766, 0.336); str = "GOLD";// gold
	//baseColor = Vector3(0.550, 0.556, 0.554); str = "CHROMIUM"; // chromium

	cubeMap = CubeMap::CubeMap("../../data/yokohama");

	camera = Camera(640, 480, Vector3(-200.0f, -200.0f, 100.0f), Vector3(40, -40, 5), DEG2RAD(42.185f)); // camera on allien
	//camera = Camera(640, 480, Vector3(2.0f, 2.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), DEG2RAD(42.185f)); // camera on sphere

	distr = ggx_distribution(scene, surfaces, 10, Vector3(0, 0, 0), std::string("a"));

	/*TESTING BRDF********/

	//TestCountSamples(); // test na pocet snimku 10, 40, 50, 100 ve vsech barvach

	JustTest(GOLD, 50);
	JustTest(IRON, 50);


	//TestRoughness(GOLD); //test na meneni roughness 0.1 0.5 1.0
	//TestMetallic(GOLD); //test na meneni metallic 0.1 0.5 1.0


	/*distr.GenerateTestingSamples(0.01, cv::Vec3b(0, 255, 0), "0.01");
	distr.GenerateTestingSamples(0.25, cv::Vec3b(0, 255, 0), "0.25");
	distr.GenerateTestingSamples(0.5, cv::Vec3b(0, 255, 0), "0.5");
	distr.GenerateTestingSamples(0.75, cv::Vec3b(0, 255, 0), "0.75");
	distr.GenerateTestingSamples(0.99, cv::Vec3b(0, 255, 0), "0.99");*/

	cv::waitKey(0);

	rtcDeleteScene(scene); // zrušení Embree scény

	SafeDeleteVectorItems<Material *>(materials);
	SafeDeleteVectorItems<Surface *>(surfaces);

	rtcDeleteDevice(device); // Embree zařízení musíme také uvolnit před ukončením aplikace

	return 0;
}
