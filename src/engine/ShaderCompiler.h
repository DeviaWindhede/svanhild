#pragma once
#include "DXHelper.h"

enum class ShaderType
{
	Vertex,
	Pixel,
	Compute,
	Geometry,
	Hull,
	Domain
};

class ShaderCompiler
{
public:
	static HRESULT Compile(std::wstring aPath, ShaderType aType, ComPtr<ID3DBlob>& aBlob);
};

