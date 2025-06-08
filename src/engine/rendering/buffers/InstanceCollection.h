#pragma once
//#include <queue>
//
//enum class InstanceType : UINT8
//{
//	Static,
//	Stationary,
//	Movable
//};
//
//
//// Heap ordering is as follows:
//// Static - Stationary - Movable
//struct InstanceCollection
//{
//public:
//	void Reset(class DX12& aDx12);
//	void OnSceneLoadComplete();
//	void Add(const struct GPUTransform& aTransform, InstanceType aType);
//	void Remove(size_t aIndex);
//
//	GPUTransform& Get(size_t aIndex) const;
//
//	GPUTransform* heap;
//private:
//	void Resize(size_t aSize);
//
//	std::queue<size_t> availableStaticIndices;
//	std::queue<size_t> availableStationaryIndices;
//	std::queue<size_t> availableMovableIndices;
//
//	size_t staticSize		= 0;
//	size_t stationarySize	= 0;
//	size_t movableSize		= 0;
//	size_t totalSize		= 0;
//
//	bool locked = false;
//};