#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#include "TextureFactory.h"
#include <stb/stb_image.h>
#include <directxtex/DDSTextureLoader.h>


















//#include <engine/graphics/Texture.h>
//#include <engine/utility/Error.h>
//
//std::shared_ptr<Texture> TextureFactory::CreateTexture(const std::string& aTexturePath, const bool& aUseRelative, bool aShouldPrintError)
//{
//	aShouldPrintError;
//
//	std::string path = aTexturePath;
//	if (aUseRelative)
//	{
//		path = RELATIVE_ASSET_PATH + path;
//	}
//
//	if (StringHelper::GetFileExtension(path) == "png")
//	{
//		std::shared_ptr<Texture> texture = nullptr;
//		if (!CreatePNGTexture(path, texture))
//		{
//#ifndef _RELEASE
//			if (aShouldPrintError)
//			{
//				PrintPathError(path);
//			}
//#endif
//		}
//		return texture;
//	}
//
//	ComPtr<ID3D11Resource> texture;
//	ComPtr<ID3D11ShaderResourceView> shaderResourceView;
//	if (CreateDDSTexture(StringHelper::s2ws(path), texture, shaderResourceView))
//	{
//		ID3D11Texture2D* pTextureInterface = 0;
//		texture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
//
//		D3D11_TEXTURE2D_DESC desc;
//		pTextureInterface->GetDesc(&desc);
//		return std::make_shared<Texture>(shaderResourceView, desc.Width, desc.Height);
//	}
//
//#ifndef _RELEASE
//	if (aShouldPrintError)
//	{
//		PrintPathError(path);
//	}
//#endif
//	//// TODO: Proper error handling
//	//assert("Failed to init texture" && false);
//
//	return nullptr;
//}
//
//#ifdef _EDITOR
//#include <d3d11.h>
//#include <directxtex/DirectXTex.h>
//
//std::shared_ptr<Texture> TextureFactory::CreateDDSTextureWithCPUAccess(const std::string& aTexturePath, std::shared_ptr<StagingTexture>& outStagingTexture)
//{
//	std::wstring path = StringHelper::s2ws(aTexturePath);
//
//	ComPtr<ID3D11Resource> textureResource;
//	ComPtr<ID3D11ShaderResourceView> shaderResourceView;
//
//	HRESULT result = DirectX::CreateDDSTextureFromFileEx(
//		GraphicsEngine::GetInstance()->DX().GetDevice().Get(),
//		path.c_str(),
//		UINT_MAX,
//		D3D11_USAGE_DYNAMIC,
//		D3D11_BIND_SHADER_RESOURCE,
//		D3D11_CPU_ACCESS_WRITE,
//		0,
//		DirectX::DDS_LOADER_IGNORE_SRGB,
//		textureResource.GetAddressOf(),
//		shaderResourceView.GetAddressOf()
//	);
//
//	if (FAILED(result))
//	{
//		PrintE("[TextureFactory.cpp] Failed to load texture: " + aTexturePath);
//		return nullptr;
//	}
//
//
//	ComPtr<ID3D11Texture2D> texture;
//	textureResource->QueryInterface(__uuidof(ID3D11Texture2D), &texture);
//
//	D3D11_TEXTURE2D_DESC desc;
//	texture->GetDesc(&desc);
//
//	outStagingTexture = std::make_shared<StagingTexture>();
//	{
//		outStagingTexture->CreateStagingTexture(nullptr, texture, shaderResourceView);
//	}
//
//	return std::make_shared<Texture>(shaderResourceView, desc.Width, desc.Height, texture);
//}
//
//void TextureFactory::UpdateTextureWithCPUAccess(std::shared_ptr<Texture> aTexture, const unsigned char* aData)
//{
//	auto* ge = GraphicsEngine::GetInstance();
//
//	D3D11_MAPPED_SUBRESOURCE mappedResource;
//	HRESULT hr = ge->DX().GetContext()->Map(aTexture->myTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
//	if (FAILED(hr))
//	{
//		PrintE("[TextureFactory.cpp] Failed to map texture: ");
//		return;
//	}
//
//	memcpy(mappedResource.pData, aData, aTexture->myWidth * aTexture->myHeight * 4);
//
//	ge->DX().GetContext()->Unmap(aTexture->myTexture.Get(), 0);
//}
//#endif
//
//bool TextureFactory::CreatePNGTexture(const std::string& aPath, std::shared_ptr<Texture>& outTexture)
//{
//	bool useSrgb = true; // temp?
//	int width, height, channels;
//
//	unsigned char* rgbaPixels = stbi_load(aPath.c_str(), &width, &height, &channels, 0);
//	if (rgbaPixels == nullptr)
//		return false;
//
//	if (channels < 3 || channels > 4)
//		return false;
//
//	if (channels == 3)
//	{
//		std::vector<unsigned char> imageData(width * height * 4);
//		for (int i = 0; i < width * height; i++)
//		{
//			imageData[4 * i + 0] = rgbaPixels[3 * i + 0];
//			imageData[4 * i + 1] = rgbaPixels[3 * i + 1];
//			imageData[4 * i + 2] = rgbaPixels[3 * i + 2];
//			imageData[4 * i + 3] = 255;
//		}
//		rgbaPixels = imageData.data();
//	}
//
//	D3D11_TEXTURE2D_DESC desc = {};
//	desc.Width = width;
//	desc.Height = height;
//	desc.MipLevels = 0;
//	desc.ArraySize = 1;
//	desc.SampleDesc.Count = 1;
//	desc.SampleDesc.Quality = 0;
//	desc.Usage = D3D11_USAGE_DEFAULT;
//	if (useSrgb)
//		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//	else
//		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
//	desc.CPUAccessFlags = 0;
//	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
//
//	auto device = GraphicsEngine::GetInstance()->DX().GetDevice();
//
//	ID3D11Texture2D* myTexture;
//	HRESULT hra = device->CreateTexture2D(&desc, nullptr, &myTexture);
//	if (FAILED(hra))
//	{
//		return false;
//	}
//
//	ComPtr<ID3D11ShaderResourceView> myShaderResourceView;
//	HRESULT hr = device->CreateShaderResourceView(myTexture, NULL, &myShaderResourceView);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	GraphicsEngine::GetInstance()->DX().GetContext()->GenerateMips(myShaderResourceView.Get());
//	myTexture->Release();
//
//	outTexture = std::make_unique<Texture>(myShaderResourceView, width, height, rgbaPixels, myTexture);
//
//	return true;
//}
//
//bool TextureFactory::CreateDDSTexture(const std::wstring& aPath, ComPtr<ID3D11Resource>& outTexture, ComPtr<ID3D11ShaderResourceView>& outShaderResourceView)
//{
//	auto& device = GraphicsEngine::GetInstance()->DX().GetDevice();
//
//	//HRESULT result = DirectX::CreateDDSTextureFromFile(
//	//	device.Get(),
//	//	aPath.c_str(),
//	//	outTexture.GetAddressOf(),
//	//	outShaderResourceView.GetAddressOf()
//	//);
//
//	//DirectX::CreateDDSText
//
//	HRESULT result = DirectX::CreateDDSTextureFromFileEx(
//		device.Get(),
//		aPath.c_str(),
//		UINT_MAX,
//		D3D11_USAGE_DEFAULT,
//		D3D11_BIND_SHADER_RESOURCE,
//		0,
//		0,
//		DirectX::DDS_LOADER_IGNORE_SRGB,		
//		outTexture.GetAddressOf(),
//		outShaderResourceView.GetAddressOf()
//	);
//
//	return !FAILED(result);
//}
//
//bool TextureFactory::CreateDDSTexture(const std::wstring& aPath, ComPtr<ID3D11ShaderResourceView>& outShaderResourceView)
//{
//	ComPtr<ID3D11Resource> temp;
//	return CreateDDSTexture(aPath, temp, outShaderResourceView);
//}
//
//void TextureFactory::WriteDDSToFile(
//	const std::wstring& aPath,
//	const char* aData,
//	const int& aWidth,
//	const int& aHeight
//)
//{
//	std::ofstream file(aPath, std::ios::binary);
//	if (!file) {
//		PrintE("Failed to open file for writing: " + StringHelper::ws2s(aPath));
//		return;
//	}
//
//	DDS_HEADER header;
//	memset(&header, 0, sizeof(header));
//	header.dwSize = sizeof(header);
//	header.dwFlags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000;
//	header.dwHeight = aHeight;
//	header.dwWidth = aWidth;
//	header.dwPitchOrLinearSize = aWidth * 4;
//	header.dwDepth = 0;
//	header.dwMipMapCount = 0;
//	header.dwReserved2 = 0;
//	header.ddspf.size = sizeof(DDS_PIXELFORMAT);
//	header.ddspf.flags = 0x4;
//	header.ddspf.RGBBitCount = 32;
//	header.ddspf.fourCC = 0;
//	header.dwCaps = 0x1000 | 0x8;
//
//	file.write(reinterpret_cast<const char*>(&header), sizeof(header));
//	file.write(aData, aWidth * aHeight * 4);
//
//	file.close();
//}
