#pragma once
#include "IComponentContainer.h"
#include "../entity/EntitySignatureContainer.h"

#include <cassert>
#include <set>
#include <unordered_map>
#include <vector>

template<typename Derived>
struct Component;

template<typename T>
class ComponentContainer : public IComponentContainer
{
public:
	ComponentContainer(size_t aMaxSize) : myMaxSize(aMaxSize), myComponentToEntityMap()
	{
		// since we want segmented memory we block allocate an array, additionally the defai;t size is MAX_ENTITES since it is possible for every entity to have every component
		myComponents = new T[aMaxSize];
		myComponentToEntityMap = new eid_t[aMaxSize];
		mySize = 0;
	}

	~ComponentContainer() override
	{
		delete[] myComponents;
		delete[] myComponentToEntityMap;
		mySize = 0;
	}

	void Reset() override
	{
		T copy{};
		while (mySize > 0)
		{
			memcpy(&myComponents[mySize - 1], &copy, sizeof(T));  // resets the component
			--mySize;
		}
	}

	T& AddComponent(const Entity& aEntity, EntitySignatureCollection& aCollection)
	{
		assert(mySize < myMaxSize && "ComponentContainer is full");
		assert(!aCollection.GetComponentSignature(aEntity).test(T::componentId) && "Entity contains the component you are trying to add");

		aCollection.SetSignatureIndex(aEntity, T::componentId, static_cast<int>(mySize));
		aCollection.UpdateComponentSignature(aEntity, T::componentId, true);
		myComponentToEntityMap[mySize] = aEntity;
		++mySize;

		return myComponents[mySize - 1];
	}

	void RemoveComponent(const Entity& aEntity, EntitySignatureCollection& aCollection)
	{
		assert(aCollection.GetComponentSignature(aEntity).test(T::componentId) && "Entity does not exist in ComponentContainer");
		RemoveComponentInternal(aEntity, aCollection);
	}

	T& GetComponent(int aIndex) const
	{
		assert(aIndex < mySize && "Index out of range");
		return myComponents[aIndex];
	}

	void OnEntityDestroyed(const Entity& aEntity, EntitySignatureCollection& aCollection) override
	{
		// if the entity is not in the container, do nothing
		if (!aCollection.GetComponentSignature(aEntity).test(T::componentId)) { return; }

		RemoveComponent(aEntity, aCollection);
	}

	const size_t& GetMaxSize() const { return myMaxSize; }

//#ifndef _RELEASE
//	virtual void OnEntityDestroyed(EntityData& aEntity) override
//	{
//		// if the entity is not in the container, do nothing
//		if (std::find(myEntities.begin(), myEntities.end(), &aEntity) == myEntities.end()) { return; }
//
//		RemoveComponent(aEntity.id);
//		auto id = std::find(myEntities.begin(), myEntities.end(), &aEntity);
//		myEntities.at(id)->components.erase(std::remove(myEntities.at(id)->components.begin(), myEntities.at(id)->components.end(), typeid(T)), myEntities.at(id)->components.end());
//		myEntities.erase(id);
//	}
//#endif
private:
	void RemoveComponentInternal(const Entity& aEntity, EntitySignatureCollection& aCollection)
	{
		assert(mySize > 0 && "ComponentContainer is empty");

		size_t lastElementIndex = mySize - 1;
		int entityToRemoveIndex = aCollection.GetSignatureIndex(aEntity, T::componentId);

		if (entityToRemoveIndex == lastElementIndex)
		{
			T copy{};
			memcpy(&myComponents[lastElementIndex], &copy, sizeof(T));  // resets the component
			aCollection.UpdateComponentSignature(aEntity, T::componentId, false);
			--mySize;
			return;
		}

		eid_t lastEntity = myComponentToEntityMap[lastElementIndex];
		memcpy(&myComponents[entityToRemoveIndex], &myComponents[lastElementIndex], sizeof(myComponents[entityToRemoveIndex]));
		T copy{};
		memcpy(&myComponents[lastElementIndex], &copy, sizeof(T));  // resets the component
		myComponentToEntityMap[entityToRemoveIndex] = lastEntity;

		aCollection.SetSignatureIndex(lastEntity, T::componentId, entityToRemoveIndex);
		aCollection.UpdateComponentSignature(aEntity, T::componentId, false);
		--mySize;
	}

	const size_t myMaxSize;
	size_t mySize;
	T* myComponents;
	eid_t* myComponentToEntityMap;
	// TODO: optimize initialization of components if they contain too lot of data?
};

