#include "pch.h"
#include "ShaderCompiler.h"
#include <IWindow.h>

HRESULT ShaderCompiler::Compile(std::wstring aPath, ShaderType aType, ComPtr<ID3DBlob>& aBlob)
{
	ID3DBlob* errorBlob = nullptr;

	std::string target = "";
	switch (aType)
	{
	case ShaderType::Vertex:
		target = "v";
		aPath += L"_VS";
		break;
	case ShaderType::Pixel:
		target = "p";
		aPath += L"_PS";
		break;
	case ShaderType::Compute:
		target = "c";
		aPath += L"_CS";
		break;
	case ShaderType::Geometry:
		target = "g";
		aPath += L"_GS";
		break;
	case ShaderType::Hull:
		target = "h";
		aPath += L"_HS";
		break;
	case ShaderType::Domain:
		target = "d";
		aPath += L"_DS";
		break;
	default:
		break;
	}
	target += "s_5_0";
	aPath += L".hlsl";

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	HRESULT result = D3DCompileFromFile(
		IWindow::GetEngineShaderFullPath(aPath.c_str()).c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",//,
		target.c_str(),
		compileFlags,
		0,
		&aBlob,
		&errorBlob
	);

	if (FAILED(result) && errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();

		throw HrException(result);
	}

    return result;
}
