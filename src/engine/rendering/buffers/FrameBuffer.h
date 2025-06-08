#pragma once
#include "rendering/CbvResource.h"
#include "rendering/SceneBufferTypes.h"

class FrameBuffer : public CbvResource<FrameBufferConstantData>
{
public:
	void Init( 
		ID3D12Device* aDevice,
		D3D12_HEAP_TYPE aHeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD,
		const FrameBufferConstantData* aDefaultData = nullptr,
		size_t aDataSize = sizeof(FrameBufferData),
		bool aShouldUnmap = false
	) override;
    void Update(class DX12& aDx12, class Camera& aCamera, class ApplicationTimer& aTimer);
private:
	void ExtractFrustumPlanes(FrustumPlane outPlanes[6], const DirectX::XMMATRIX& viewProj, bool normalize = true);
};
