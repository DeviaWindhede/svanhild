#pragma once
#include <wrl/client.h>

#ifdef _EDITOR
#include <engine/graphics/StagingTexture.h>
#include <memory>
#endif

using Microsoft::WRL::ComPtr;
struct ID3D11ShaderResourceView;

class Texture;

enum class TextureInitializationType
{
	DDS,
	PNG
};

// https://stackoverflow.com/questions/3318410/pragma-pack-effect
#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t RGBBitCount;
    uint32_t RBitMask;
    uint32_t GBitMask;
    uint32_t BBitMask;
    uint32_t ABitMask;
};

struct DDS_HEADER {
    uint32_t           dwSize;
    uint32_t           dwFlags;
    uint32_t           dwHeight;
    uint32_t           dwWidth;
    uint32_t           dwPitchOrLinearSize;
    uint32_t           dwDepth;
    uint32_t           dwMipMapCount;
    uint32_t           dwReserved1[11];
    DDS_PIXELFORMAT    ddspf;
    uint32_t           dwCaps;
    uint32_t           dwCaps2;
    uint32_t           dwCaps3;
    uint32_t           dwCaps4;
    uint32_t           dwReserved2;
};
#pragma pack(pop)

class TextureFactory
{
public:
	TextureFactory() = delete;
	~TextureFactory() = delete;

//	static std::shared_ptr<Texture> CreateTexture(const std::string& aTexturePath, const bool& aUseRelative = true, bool aShouldPrintError = true);
//#ifdef _EDITOR
//    static std::shared_ptr<Texture> CreateDDSTextureWithCPUAccess(const std::string& aTexturePath, std::shared_ptr<StagingTexture>& outStagingTexture);
//    static void UpdateTextureWithCPUAccess(std::shared_ptr<Texture> aTexture, const unsigned char* aData);
//#endif
//    static bool CreateDDSTexture(const std::wstring& aPath, ComPtr<ID3D12Resource>& outTexture, ComPtr<ID3D11ShaderResourceView>& outShaderResourceView);
//	static bool CreateDDSTexture(const std::wstring& aPath, ComPtr<ID3D12ShaderResourceView>& outShaderResourceView);
//	static void WriteDDSToFile(const std::wstring& aPath, const char* aData, const int& aWidth, const int& aHeight);
//private:
//	static bool CreatePNGTexture(const std::string& aPath, std::shared_ptr<Texture>& outTexture);
};