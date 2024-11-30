#pragma once

#include "IResource.h"
#include "DXHelper.h"

class Texture : public IResource
{
public:
	virtual void LoadToGPU(class DX12& aDx12) override;
    virtual void OnGPULoadComplete() override;
private:
    static const UINT TextureWidth = 256;
    static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

	std::vector<UINT8> GenerateTextureData();

    ComPtr<ID3D12Resource> m_texture;
};

