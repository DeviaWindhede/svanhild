#include "pch.h"
#include "ShaderCompiler.h"
#include "DX12.h"
#include <IWindow.h>
#include "StringHelper.h"

#if SHOULD_RECOMPILE_DURING_RUNTIME
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <chrono>

// Shared resources
std::unordered_set<std::string> filesToWatch;
std::mutex filesMutex;
std::atomic<bool> stopMonitoring = false;
std::filesystem::file_time_type lastModificationTime{};
#endif

ShaderCompiler* ShaderCompiler::instance = nullptr;

ShaderCompiler::ShaderCompiler(class DX12& aDx12)
#if SHOULD_RECOMPILE_DURING_RUNTIME
	: watcher(&ShaderCompiler::WatchFiles, this, IWindow::GetEngineShaderFullPath(L"")),
#else
	:
#endif
	dx12(aDx12)
{

}

ShaderCompiler::~ShaderCompiler()
{
#if SHOULD_RECOMPILE_DURING_RUNTIME
	stopMonitoring = true;
	if (watcher.joinable()) {
		watcher.join();
	}
#endif
}

#if SHOULD_RECOMPILE_DURING_RUNTIME
void ShaderCompiler::AddFileToWatch(const std::wstring& fileName)
{
	std::lock_guard<std::mutex> lock(filesMutex);
	std::string path = StringHelper::ws2s(IWindow::GetEngineShaderFullPath(fileName + L".hlsl"));
	if (filesToWatch.contains(path))
		return;
	filesToWatch.insert(path);
	std::cout << "Added file to watch: " << path << std::endl;
}

void ShaderCompiler::RemoveFileToWatch(const std::wstring& fileName)
{
	std::lock_guard<std::mutex> lock(filesMutex);
	std::string path = StringHelper::ws2s(IWindow::GetEngineShaderFullPath(fileName + L".hlsl"));
	filesToWatch.erase(path);
	std::cout << "Removed file from watch list: " << path << std::endl;
}

void ShaderCompiler::WatchFiles(const std::wstring& directory)
{
	HANDLE hDirectory = CreateFileW(directory.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (hDirectory == INVALID_HANDLE_VALUE)
		throw "Dir not found";

	char buffer[1024];
	DWORD bytesReturned;

	while (!stopMonitoring) {
		if (ReadDirectoryChangesW(hDirectory, buffer, sizeof(buffer), TRUE,
			FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
			&bytesReturned, NULL, NULL))
		{

			FILE_NOTIFY_INFORMATION* notification = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
			do
			{
				if (notification->Action == FILE_ACTION_RENAMED_OLD_NAME)
				{
					std::wstring fileNameW(notification->FileName, notification->FileNameLength / sizeof(wchar_t));
					std::string fileName(fileNameW.begin(), fileNameW.end());

					std::wcout << L"Change detected: " << fileNameW << std::endl;

					if (fileName.size() >= 5 && fileName.substr(fileName.size() - 5) == ".hlsl")
					{
						auto currentModificationTime = std::filesystem::last_write_time(directory + L"\\" + fileNameW);

						if (currentModificationTime != lastModificationTime)
						{
							std::wcout << L"File modified: " << fileNameW << std::endl;
							lastModificationTime = currentModificationTime;

							std::lock_guard<std::mutex> lock(instance->shaderAccessMutex);
							size_t index = GetShader(fileNameW.substr(0, fileNameW.size() - 5)).index;

							if (shadersToRecompile.size() > 0 && shadersToRecompile.back() == index)
								continue;

							shadersToRecompile.push(index);
						}
					}
				}

				if (notification->NextEntryOffset == 0) {
					break;
				}
				notification = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
					reinterpret_cast<BYTE*>(notification) + notification->NextEntryOffset);
			} while (true);
		}
		else {
			std::cerr << "Failed to read directory changes" << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(SHADER_POLL_TIME_MS));
	}

	CloseHandle(hDirectory);
}
#endif

HRESULT ShaderCompiler::CompileShader(std::wstring aPath, ShaderType aType, size_t& outShaderIndex)
{
	Shader shader{};
	HRESULT hr = CompileShader_Internal(aPath, aType, shader);

	if (FAILED(hr))
		return hr;

	shader.index = instance->shaders.size();
	instance->shaders.push_back(shader);
	instance->pathToIndex.insert({ shader.path, shader.index });
	outShaderIndex = instance->shaders.back().index;

	return hr;
}

HRESULT ShaderCompiler::RecompileShader(const Shader& aShader)
{
	instance->dx12.WaitForGPU();

	Shader shader{};

	//{
	//	std::wstring oldName = IWindow::GetCSOPath(aShader.path + L".cso");
	//	std::wstring newName = IWindow::GetCSOPath(aShader.path + L"_temp.cso");

	//	// Rename the file
	//	if (std::rename(StringHelper::ws2s(oldName).c_str(), StringHelper::ws2s(newName).c_str()) != 0) {
	//		throw "Unable to rename file";
	//	}
	//}

	HRESULT hr = CompileShader_Internal(aShader.path.substr(0, aShader.path.length() - 3), aShader.type, shader);

	if (FAILED(hr))
	{
		//std::wstring oldName = IWindow::GetCSOPath(aShader.path + L"_temp.cso");
		//std::wstring newName = IWindow::GetCSOPath(aShader.path + L".cso");

		//// Rename the file
		//if (std::rename(StringHelper::ws2s(oldName).c_str(), StringHelper::ws2s(newName).c_str()) != 0) {
		//	throw "Unable to rename file";
		//}

		return hr;
	}

	instance->dx12.WaitForGPU();

	shader.index = aShader.index;
	instance->shaders[aShader.index] = shader;

	for (size_t i = 0; i < instance->states.size(); i++)
	{
		PipelineState& pso = instance->states[i];
		bool comparison = false;
		switch (shader.type)
		{
		case ShaderType::Vertex:
			comparison = pso.indexVS == aShader.index;
			break;
		case ShaderType::Pixel:
			comparison = pso.indexPS == aShader.index;
			break;
		case ShaderType::Compute:
			//D3D12_COMPUTE_PIPELINE_STATE_DESC
			break;
		case ShaderType::Geometry:
			comparison = pso.indexGS == aShader.index;
			break;
		case ShaderType::Hull:
			comparison = pso.indexHS == aShader.index;
			break;
		case ShaderType::Domain:
			comparison = pso.indexDS == aShader.index;
			break;
		default:
			break;
		}

		if (comparison)
		{
			PipelineState newPso{};
			CreatePSO_Internal(newPso, pso.indexVS, pso.indexPS, pso.indexHS, pso.indexGS, pso.indexDS);
			newPso.index = pso.index;
			instance->states[pso.index] = newPso;
		}
	}

	//std::wstring oldName = IWindow::GetCSOPath(aShader.path + L"_temp.cso");
	//if (std::remove(StringHelper::ws2s(oldName).c_str()) != 0) {
	//	throw "Unable to rename file";
	//}

	return hr;
}

HRESULT ShaderCompiler::CompileShader_Internal(std::wstring aPath, ShaderType aType, Shader& outShader)
{
	outShader = {};

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

	outShader.path = aPath;

	target += "s_5_0";
	aPath += L".hlsl";

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	HRESULT hr = D3DCompileFromFile(
		IWindow::GetEngineShaderFullPath(aPath.c_str()).c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",//,
		target.c_str(),
		compileFlags,
		0,
		&outShader.blob,
		&errorBlob
	);

	if (FAILED(hr) && errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();

		//throw HrException(hr);
		return hr;
	}

	outShader.type = aType;

#if SHOULD_RECOMPILE_DURING_RUNTIME
	instance->AddFileToWatch(outShader.path);
#endif

	return hr;
}

void PipelineState::Set(DX12& aDx12) const
{
	aDx12.myCommandList->SetPipelineState(state.Get());
	aDx12.currentPSO = index;
}
