#pragma once
class CubeMap
{
private:
	Texture *_maps[6];
public:
	

	CubeMap(std::string path);
	Color4 GetTexel(Vector3 & direction);
	//~CubeMap();
};

