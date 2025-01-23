#pragma once
#include "DXHelper.h"
#include "DX12.h"
#include <vector>
#include <cassert>
#include <string>
#include <unordered_map>

enum class ShaderType
{
	Vertex,
	Pixel,
	Compute,
	Geometry,
	Hull,
	Domain
};

struct Shader
{
	std::wstring path;
	ComPtr<ID3DBlob> blob;
	size_t index;
	ShaderType type;
};

struct PipelineState
{
public:
	void Set(DX12& aDx12) const;

	size_t index	= SIZE_T_MAX;
	size_t indexVS	= SIZE_T_MAX;
	size_t indexPS	= SIZE_T_MAX;
	size_t indexGS	= SIZE_T_MAX;
	size_t indexHS	= SIZE_T_MAX;
	size_t indexDS	= SIZE_T_MAX;
	ComPtr<ID3D12PipelineState> state;
};

#define SHOULD_RECOMPILE_DURING_RUNTIME 1

#if SHOULD_RECOMPILE_DURING_RUNTIME
#include <unordered_set>
#include <thread>
#include <mutex>
#endif

class ShaderCompiler
{
public:
	static void CreateInstance(class DX12& aDx12)
	{
		if (instance)
			return;
		instance = new ShaderCompiler(aDx12);
	}

	static void DestroyInstance()
	{
		if (!instance)
			return;

		delete instance;
		instance = nullptr;
	}

	ShaderCompiler(class DX12& aDx12);
	~ShaderCompiler();

	template<class... ShaderIndicies>
	static HRESULT CreatePSO(ShaderIndicies... aIndicies);
	static HRESULT CompileShader(std::wstring aPath, ShaderType aType, size_t& outShaderIndex);
	static HRESULT RecompileShader(const Shader& aShader);

	static const PipelineState& GetPSO(size_t aIndex)
	{
		// TODO: Add validity
		assert(aIndex < instance->states.size() && "Out of range");
		return instance->states[aIndex];
	};

	static const Shader& GetShader(size_t aIndex)
	{
		// TODO: Add validity
		assert(aIndex < instance->shaders.size() && "Out of range");
		return instance->shaders[aIndex];
	};

	static const Shader& GetShader(const std::wstring& aPath)
	{
		assert(instance->pathToIndex.contains(aPath) && "Path does not exist!");
		return GetShader(instance->pathToIndex[aPath]);
	};

#if SHOULD_RECOMPILE_DURING_RUNTIME
	static std::queue<size_t>& GetShadersToRecompile() 
	{
		return instance->shadersToRecompile; 
	}
	static std::mutex& ShaderAccessMutex() { return instance->shaderAccessMutex; }
	
	void AddFileToWatch(const std::wstring& fileName);
	void RemoveFileToWatch(const std::wstring& fileName);
#endif
	static ShaderCompiler* instance;
private:
	template<class... ShaderIndicies>
	static HRESULT CreatePSO_Internal(PipelineState& outPSO, ShaderIndicies... aIndicies);
	static HRESULT CompileShader_Internal(std::wstring aPath, ShaderType aType, Shader& outShader);

#if SHOULD_RECOMPILE_DURING_RUNTIME
	static constexpr int SHADER_POLL_TIME_MS = 250;

	void WatchFiles(const std::wstring& directory);

	std::thread watcher;
	mutable std::mutex shaderAccessMutex;
	mutable std::queue<size_t> shadersToRecompile;
#endif
	std::vector<PipelineState> states;
	std::vector<Shader> shaders;
	std::unordered_map<std::wstring, size_t> pathToIndex;
	class DX12& dx12;
};

template<class ...ShaderIndicies>
inline HRESULT ShaderCompiler::CreatePSO(ShaderIndicies ...aIndicies)
{
	PipelineState pso{};
	HRESULT hr = CreatePSO_Internal(pso, aIndicies...);

	if (FAILED(hr))
		return hr;

	pso.index = instance->states.size();
	instance->states.push_back(pso);

	return hr;
}

template<class ...ShaderIndicies>
inline HRESULT ShaderCompiler::CreatePSO_Internal(PipelineState& outPSO, ShaderIndicies ...aIndicies)
{
	assert(instance->states.size() < 256 && "Too many PSOs! Duplicates might be present");
	outPSO = {};

	// TODO: Add custom desc as param
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WORLD",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "WORLD",  1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "WORLD",  2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL",  0, DXGI_FORMAT_R32_UINT,			1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	// TODO: Add custom desc as param
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = instance->dx12.myRootSignature.Get();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	//computePsoDesc.pRootSignature = instance->dx12.myComputeRootSignature.Get();

	bool hasCompute = false;
	int i = 0;
	([&] {
		if (aIndicies == SIZE_T_MAX)
			return;

		const Shader& shader = GetShader(aIndicies);
		CD3DX12_SHADER_BYTECODE byteCode = CD3DX12_SHADER_BYTECODE(shader.blob.Get());
		switch (shader.type)
		{
		case ShaderType::Vertex:
			psoDesc.VS = byteCode;
			outPSO.indexVS = shader.index;
			break;
		case ShaderType::Pixel:
			psoDesc.PS = byteCode;
			outPSO.indexPS = shader.index;
			break;
		case ShaderType::Compute:
			computePsoDesc.CS = byteCode;
			hasCompute = true;
			return;
		case ShaderType::Geometry:
			psoDesc.GS = byteCode;
			outPSO.indexGS = shader.index;
			break;
		case ShaderType::Hull:
			psoDesc.HS = byteCode;
			outPSO.indexHS = shader.index;
			break;
		case ShaderType::Domain:
			psoDesc.DS = byteCode;
			outPSO.indexDS = shader.index;
			break;
		default:
			break;
		}

		++i;
		} (), ...);

	if (hasCompute)
	{
		HRESULT result = instance->dx12.myDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&outPSO.state));

		if (SUCCEEDED(result))
			NAME_D3D12_OBJECT(outPSO.state);

		return result;
	}

	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Test: pass if the pixel is closer to the camera
	psoDesc.DepthStencilState.StencilEnable = FALSE;  // Disable stencil testing
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	HRESULT result = instance->dx12.myDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&outPSO.state));
	
	if (SUCCEEDED(result))
		NAME_D3D12_OBJECT(outPSO.state);

	return result;
}
