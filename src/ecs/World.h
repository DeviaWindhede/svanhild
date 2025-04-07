#pragma once

#define SHOULD_TRACK_ENTITIES 1

#include "entity/Entity.h"
#include "entity/EntityManager.h"

#include "component/ComponentManager.h"
#include "system/SystemManager.h"
#include <cassert>

#ifdef _EDITOR
#include <vector>

#if SHOULD_TRACK_ENTITIES
#include <unordered_set>
#endif
#endif

// ACTS AS ADAPTER FOR ALL MANAGERS
class World
{
public:
	World();
	~World();

	void Reset();
	void Init();
	void Update(SceneUpdateContext& aDeltaTime);
	void Render();

	// Entity Adapter
	Entity CreateEntity();
	void DestroyEntity(const Entity& aEntity);

	void SetPlayerEntityID(const eid_t& aPlayerEntityID) { myPlayerEntityID = aPlayerEntityID; };
	eid_t GetPlayerEntityID() const
	{
		//Value will be INVALID_ENTITY if no player exists in scene
		return myPlayerEntityID;
	};

	// Component Adapter
	template<typename T>
	void RegisterComponent(size_t aMaxSize = MAX_ENTITIES);

	template<typename T>
	T& AddComponent(const Entity& aEntity);

	template<typename T>
	void RemoveComponent(const Entity& aEntity);

	template<typename T>
	T& GetComponent(const Entity& aEntity);

	template<typename... Components> requires (sizeof...(Components) != 1)
		decltype(auto) get(Entity aEntity) const
	{
		//static_assert(details::is_subset_of<std::tuple<std::remove_const_t<_Cs>...>, std::tuple<Cs...> >);
		//assert(contains(aEntity));

		//if constexpr (sizeof...(_Cs) == 0)
		//	return std::forward_as_tuple(std::get<component_pool<Cs>*>(pools)->get(aEntity)...);
		//else
		return std::forward_as_tuple(myComponentManager->GetComponent<Components>(aEntity) ...);
	}

	// System Adapter
	template<typename T, typename... Param>
	T* RegisterSystem(Param... aParams);

	template<typename T, typename... Param>
	T* RegisterSystem(const ComponentSignature& aSignature, Param... aParams);

	template<typename T>
	void SetSystemSignature(const ComponentSignature& aSignature);

	template<typename T>
	cid_t GetComponentSignatureID();

	template<typename T>
	T* GetSystem();

	void InitSystems();

	template<typename T>
	bool HasComponent(const Entity& aEntity) const
	{
		return myComponentManager->HasComponent<T>(aEntity);
	}

#ifdef _EDITOR
	template<typename T>
	T* TryGetComponent(const Entity& aEntity);
#endif

#ifdef _EDITOR
	std::vector<Entity>& GetEntities() { return myEntities; }
#endif

private:
	World(const World& aWorld) = delete;
	World& operator=(const World& aOther) = delete;

	template<typename T>
	void UpdateSignature(const Entity& aEntity, const bool& aValue);

	void MarkEntityForDeletion(const Entity& entity);
	void DeleteMarkedEntities();

	std::array<Entity, MAX_ENTITIES> myMarkedForDeletionEntities = {};
	size_t myMarkedDeleteCount = 0;

	EntityManager* myEntityManager = nullptr;
	SystemManager* mySystemManager = nullptr;
	ComponentManager* myComponentManager = nullptr;

	eid_t myPlayerEntityID = INVALID_ENTITY;

#ifdef _EDITOR
	std::vector<Entity> myEntities = {};
#if SHOULD_TRACK_ENTITIES
	std::unordered_set<Entity> myLifetimeEntityCount;
#endif
#endif
};

template<typename T>
void World::RegisterComponent(size_t aMaxSize)
{
	myComponentManager->RegisterComponent<T>(aMaxSize);
}

template<typename T>
T& World::AddComponent(const Entity& aEntity)
{
	T& component = myComponentManager->AddComponent<T>(aEntity);
	UpdateSignature<T>(aEntity, true);
	return component;
}

template<typename T>
void World::RemoveComponent(const Entity& aEntity)
{
	myComponentManager->RemoveComponent<T>(aEntity);
	UpdateSignature<T>(aEntity, false);
}

template<typename T>
inline T& World::GetComponent(const Entity& aEntity)
{
	return myComponentManager->GetComponent<T>(aEntity);
}

template<typename T, typename... Param>
T* World::RegisterSystem(Param... aParams)
{
	return mySystemManager->RegisterSystem<T>(aParams...);
}

template<typename T>
T* World::GetSystem()
{
	return mySystemManager->GetSystem<T>();
}

template<typename T, typename... Param>
inline T* World::RegisterSystem(const ComponentSignature& aSignature, Param... aParams)
{
	T* system = World::RegisterSystem<T>(aParams...);
	SetSystemSignature<T>(aSignature);
	return system;
}

template<typename T>
inline void World::SetSystemSignature(const ComponentSignature& aSignature)
{
	// TODO: set the entity container within the system to be equal
	// to the size of the smallest component in the signature
	mySystemManager->SetSignature<T>(aSignature);
}

template<typename T>
inline cid_t World::GetComponentSignatureID()
{
	return myComponentManager->GetComponentSignatureID<T>();
}

template<typename T>
void World::UpdateSignature(const Entity& aEntity, const bool& aValue)
{
	myComponentManager->UpdateComponentSignature<T>(aEntity, aValue);
	mySystemManager->OnEntitySignatureChanged(aEntity, myComponentManager->GetEntitySignature(aEntity));
}

#ifdef _EDITOR
template<typename T>
inline T* World::TryGetComponent(const Entity& aEntity)
{
	return myComponentManager->TryGetComponent<T>(aEntity);
}
#endif
