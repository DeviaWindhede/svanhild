#include "pch.h"
#include "SceneBufferTypes.h"
#include "InstanceCollection.h"
#include "DX12.h"
//
//void InstanceCollection::Reset(class DX12& aDx12)
//{
//	aDx12.instanceBuffer.Create(&aDx12);
//	delete[] heap;
//
//	availableMovableIndices = {};
//	availableStationaryIndices = {};
//	stationaryStart = 0;
//	movableStart = 0;
//	size = 0;
//}
//
//void InstanceCollection::OnSceneLoadComplete()
//{
//	locked = true;
//}
//
//// TODO: MOVE TO MATH
//inline static size_t NextPowerOfTwo(size_t aValue)
//{
//	if (aValue <= 1)
//		return 1;
//	unsigned long index;
//	_BitScanReverse64(&index, aValue - 1);
//	return 1ull << (index + 1);
//}
//
//void InstanceCollection::Add(const GPUTransform& aTransform, InstanceType aType)
//{
//	bool shouldIncreaseSize = false;
//
//	size_t index = 0;
//
//	std::queue<size_t>* queue = nullptr;
//	size_t* size = nullptr;
//
//	switch (aType)
//	{
//	case InstanceType::Static:
//		if (locked)
//			return;
//		
//		queue = &availableStaticIndices;
//		size = &staticSize;
//		break;
//	case InstanceType::Stationary:
//		queue = &availableStationaryIndices;
//		size = &stationarySize;
//		break;
//	case InstanceType::Movable:
//		queue = &availableMovableIndices;
//		size = &movableSize;
//		break;
//	}
//
//	size_t newSize = NextPowerOfTwo(totalSize);
//	GPUTransform* newHeap = new GPUTransform[newSize];
//	
//
//	delete[] heap;
//
//	heap = newHeap;
//}
//
//void InstanceCollection::Remove(size_t aIndex)
//{
//	assert(aIndex >= stationaryStart && aIndex < size && "Trying to remove objects outside valid range");
//
//	if (aIndex < movableStart)
//		availableStationaryIndices.push(aIndex);
//}
//
//GPUTransform& InstanceCollection::Get(size_t aIndex) const
//{
//	assert(aIndex < size && "Trying to access element out of range");
//	return heap[aIndex];
//}
//
//
//void InstanceCollection::Resize(size_t aSize)
//{
//}
