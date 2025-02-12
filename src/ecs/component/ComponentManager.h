#pragma once
#include <vector>
#include <unordered_map>
#include <typeinfo>

#include "Component.h"
#include "ComponentContainer.h"
#include "../entity/Entity.h"

class IComponentContainer;
class World;

class ComponentManager
{
public:
	friend class World;

	ComponentManager()
	{
		myComponentContainers = {};
		myNextComponentId = 0;
	}

	void Reset()
	{
		for (auto& c : myComponentContainers)
		{
			c->Reset();
		}
		myEntitySignatureCollection.Reset();
	}

	template<typename T>
	void RegisterComponent(size_t aMaxSize)
	{
		T::componentId = myNextComponentId;
		myComponentContainers.push_back(new ComponentContainer<T>(aMaxSize));

		++myNextComponentId;
	}

	template<typename T>
	T& AddComponent(const Entity& aEntity)
	{
		return GetComponentContainer<T>()->AddComponent(aEntity, myEntitySignatureCollection);
	}

	template<typename T>
	void RemoveComponent(const Entity& aEntity)
	{
		GetComponentContainer<T>()->RemoveComponent(aEntity, myEntitySignatureCollection);
	}

	template<typename T>
	T& GetComponent(const Entity& aEntity) const
	{
		//assert(myEntitySignatureCollection.GetComponentSignature(aEntity).test(T::componentId) && "Entity does not exist in ComponentContainer");
		return GetComponentContainer<T>()->GetComponent(myEntitySignatureCollection.GetSignatureIndex(aEntity, T::componentId));
	}

	template<typename T>
	T* TryGetComponent(const Entity& aEntity) const
	{
		if (!myEntitySignatureCollection.GetComponentSignature(aEntity).test(T::componentId)) { return nullptr; }
		return &GetComponent<T>(aEntity);
	}

	void OnEntityDestroyed(const Entity& aEntity)
	{
		for (auto& c : myComponentContainers)
		{
			c->OnEntityDestroyed(aEntity, myEntitySignatureCollection);
		}
	}

	template<typename T>
	cid_t GetComponentSignatureID() const
	{
		return T::componentId;
	}

	template<typename T>
	bool HasComponent(const Entity& aEntity) const
	{
		return myEntitySignatureCollection.GetComponentSignature(aEntity)[GetComponentSignatureID<T>()];
	}

	template<typename T>
	void UpdateComponentSignature(const Entity& aEntity, bool aValue)
	{
		myEntitySignatureCollection.UpdateComponentSignature(aEntity, GetComponentSignatureID<T>(), aValue);
	}

	const ComponentSignature& GetEntitySignature(const Entity& aEntity) const
	{
		return myEntitySignatureCollection.GetComponentSignature(aEntity);
	}
private:
	std::vector<IComponentContainer*> myComponentContainers;
	EntitySignatureCollection myEntitySignatureCollection;
	cid_t myNextComponentId = 0;

	template<typename T>
	ComponentContainer<T>* GetComponentContainer() const
	{
		cid_t cid = T::componentId;

		assert(cid < myNextComponentId && "Component not registered before use.");

		return (ComponentContainer<T>*)myComponentContainers[cid];
	}
};

