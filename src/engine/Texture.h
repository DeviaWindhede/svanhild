#pragma once

#include "IResource.h"
#include "DXHelper.h"
#include "DescriptorHeapHandle.h"

class Texture : public IResource
{
public:
    virtual ~Texture() override;
	virtual void LoadToGPU(class DX12& aDx12) override;
    virtual void OnGPULoadComplete(class DX12& aDx12) override;
    virtual void UnloadCPU(class DX12& aDx12) override;

    bool Bind(UINT aSlot, class DX12& aDx12);
private:
    static const UINT TextureWidth = 256;
    static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

    DescriptorHeapHandle descriptorHandle;

	std::vector<UINT8> GenerateTextureData();
};

