//#include "stdafx.h"
//
//
//CubeMap::CubeMap(std::string path)
//{
//	this->_maps[0] = LoadTexture((path + "/posx.jpg").c_str());
//	this->_maps[1] = LoadTexture((path + "/posy.jpg").c_str());
//	this->_maps[2] = LoadTexture((path + "/posz.jpg").c_str());
//	this->_maps[3] = LoadTexture((path + "/negx.jpg").c_str());
//	this->_maps[4] = LoadTexture((path + "/negy.jpg").c_str());
//	this->_maps[5] = LoadTexture((path + "/negz.jpg").c_str());
//
//
//}
//
//Color4 CubeMap::GetTexel(Vector3 & direction)
//{
//	char indexLargest = direction.LargestComponent(true);
//	
//	int indexMap = 0;
//
//	if (indexLargest == 0) // Xtextura
//	{
//		indexMap = (direction.x > 0) ? indexMap = 0: indexMap = 3;
//	}
//	else if (indexLargest == 1) // ytextura
//	{
//		indexMap = (direction.y > 0) ? indexMap = 1 : indexMap = 4;
//	
//	}
//	else // ztextura
//	{
//		indexMap = (direction.z > 0) ? indexMap = 2 : indexMap = 5;
//	}
//
//	int u;
//	int v;
//
//	float tmp;
//
//	switch (indexMap)
//	{
//	case 0: // posX
//		//const float 
//			tmp = 1.0f / abs(direction.x);
//		u = (direction.y * tmp + 1) * 0.5f;
//		v = (direction.z * tmp + 1) * 0.5f;
//		break;
//	case 3: // negX
//			//const float 
//		tmp = 1.0f / abs(direction.x);
//		u = (direction.y * tmp - 1) * 0.5f;
//		v = (direction.z * tmp - 1) * 0.5f;
//		break;
//	case 1: // posY
//			//const float 
//		tmp = 1.0f / abs(direction.y);
//		u = (direction.x * tmp + 1) * 0.5f;
//		v = (direction.z * tmp + 1) * 0.5f;
//		break;
//	case 4: // negY
//			//const float 
//		tmp = 1.0f / abs(direction.y);
//		u = (direction.x * tmp - 1) * 0.5f;
//		v = (direction.z * tmp - 1) * 0.5f;
//		break;
//	case 2: // posZ
//			//const float 
//		tmp = 1.0f / abs(direction.z);
//		u = (direction.x * tmp - 1) * 0.5f;
//		v = (direction.y * tmp - 1) * 0.5f;
//		break;
//	case 5: // negZ
//			//const float 
//		tmp = 1.0f / abs(direction.z);
//		u = (direction.x * tmp + 1) * 0.5f;
//		v = (direction.y * tmp + 1) * 0.5f;
//		break;
//	default:
//		break;
//	}
//
//	//case POS_X:
//	//const float tmp = 1.0f / abs(direction.x);
//	//u = (direction.y * tmp + 1) * 0.5f;
//	//v = (direction.z * tmp + 1) * 0.5f;
//	//break;
//	//…
//		
//	Color4 texel = _maps[indexMap]->get_texel(u, v);
//	return texel;
//}
//
//
//CubeMap::~CubeMap()
//{
//	
//}



#include "stdafx.h"
//#include "CubeMap.h"






Color4 CubeMap::GetTexel(Vector3 & direction)
{
	Color4 retval = Color4(50, 50, 50, 50);

	int dIndex = direction.LargestComponent(true);
	float u = 0;
	float v = 0;

	switch (dIndex)
	{
	case 0:					//Osa X
		if (direction.x > 0)			//  POS X
		{
			const float tmp = 1.0f / abs(direction.x);
			u = (direction.y * tmp + 1) * 0.5f;
			v = ((direction.z * tmp + 1) * 0.5f);

			retval = (_maps[0])->get_texel(1 - u, v);
		}
		else							// NEG X
		{
			const float tmp = 1.0f / abs(direction.x);
			u = (direction.y * tmp + 1) * 0.5f;
			v = (direction.z * tmp + 1) * 0.5f;
			retval = (_maps[3])->get_texel(u, v);
		}
		break;
	case 1:					//Osa Y
		if (direction.y > 0)			//  POS Y
		{
			const float tmp = 1.0f / abs(direction.y);
			u = (direction.x * tmp + 1) * 0.5f;
			v = (direction.z * tmp + 1) * 0.5f;

			retval = _maps[1]->get_texel(u, v);

		}
		else							// NEG Y
		{
			const float tmp = 1.0f / abs(direction.y);
			u = (direction.x * tmp + 1) * 0.5f;
			v = (direction.z * tmp + 1) * 0.5f;
			retval = (_maps[4])->get_texel(1 - u, v);
		}
		break;
	case 2:					//Osa Z
		if (direction.z > 0)			//  POS Z
		{
			const float tmp = 1.0f / abs(direction.z);
			u = (direction.x * tmp + 1) * 0.5f;
			v = (direction.y * tmp + 1) * 0.5f;
			retval = (_maps[2])->get_texel(u, 1 - v);
		}
		else							// NEG Z
		{
			const float tmp = 1.0f / abs(direction.z);
			u = (direction.x * tmp + 1) * 0.5f;
			v = (direction.y * tmp + 1) * 0.5f;
			retval = (_maps[5])->get_texel(u, v);
		}
		break;
	}



	return retval;
}

CubeMap::CubeMap(std::string path)
{
	this->_maps[0] = LoadTexture((path + "/posx.jpg").c_str());
	this->_maps[1] = LoadTexture((path + "/posy.jpg").c_str());
	this->_maps[2] = LoadTexture((path + "/posz.jpg").c_str());
	this->_maps[3] = LoadTexture((path + "/negx.jpg").c_str());
	this->_maps[4] = LoadTexture((path + "/negy.jpg").c_str());
	this->_maps[5] = LoadTexture((path + "/negz.jpg").c_str());


}


