#include "World.h"

#include "entity/EntityManager.h"
#include "component/ComponentManager.h"
#include "system/SystemManager.h"

#ifdef _EDITOR
#if SHOULD_TRACK_ENTITIES
#include <Windows.h>
#include <iostream>

#define STRINGIZE(x) #x
#define PRINT_VALUE_TO_OUTPUT_WINDOW(x) \
    do { \
        OutputDebugStringA((STRINGIZE(x) " = " + std::to_string(x)).c_str()); \
    } while (0)

#include <string>
#endif
#endif

World::World()
{
#if SHOULD_TRACK_ENTITIES
#pragma message("[World.cpp] Tracking entities is enabled. This will have a performance impact.")
#endif
}

World::~World()
{
	delete myEntityManager;
	delete myComponentManager;
	delete mySystemManager;
#ifdef _EDITOR
#if SHOULD_TRACK_ENTITIES
	OutputDebugStringA("[World.cpp] ================== TRACKED ENTITY COUNT PER CURRENT SCENE: ");
	PRINT_VALUE_TO_OUTPUT_WINDOW(myLifetimeEntityCount.size());
	OutputDebugStringA(" ==================\n");
	myLifetimeEntityCount.clear();
#endif
#endif
}

void World::Reset()
{
	myComponentManager->Reset();
	myEntityManager->Reset();
	mySystemManager->Reset();

	myPlayerEntityID = INVALID_ENTITY;

#ifdef _EDITOR
	myEntities.clear();
#endif
}

void World::Init()
{
	//register all the components
  //REGISTER_COMPONENT(CTransform);
	myEntityManager = new EntityManager();
	mySystemManager = new SystemManager(this);
	myComponentManager = new ComponentManager();
}

void World::Update(SceneUpdateContext& aContext)
{
	mySystemManager->Update(aContext);
	DeleteMarkedEntities();
}

void World::Render()
{
	mySystemManager->Render();
}

Entity World::CreateEntity()
{
#ifdef _EDITOR
	eid_t entity = myEntityManager->CreateEntity();
	myEntities.push_back(entity);
#if SHOULD_TRACK_ENTITIES
	myLifetimeEntityCount.insert(entity);
#endif
	return entity;
#else
	return myEntityManager->CreateEntity();
#endif
}

void World::DestroyEntity(const Entity& aEntity)
{
	if (aEntity == INVALID_ENTITY) { return; }

	MarkEntityForDeletion(aEntity);
	//myEntityManager->DestroyEntity(entity);
	//myComponentManager->OnEntityDestroyed(entity);
	//mySystemManager->OnEntityDestroyed(entity);

#ifdef _EDITOR
	auto index = std::find(myEntities.begin(), myEntities.end(), aEntity);
	if (index == myEntities.end()) { return; }
	myEntities.erase(index);
#endif
}

void World::InitSystems()
{
	mySystemManager->Init();
}

void World::MarkEntityForDeletion(const Entity& aEntity)
{
	myMarkedForDeletionEntities[myMarkedDeleteCount] = aEntity;
	myMarkedDeleteCount++;
}

void World::DeleteMarkedEntities()
{
	for (size_t i = 0; i < myMarkedDeleteCount; i++)
	{
		myComponentManager->OnEntityDestroyed(myMarkedForDeletionEntities[i]);
		mySystemManager->OnEntityDestroyed(myMarkedForDeletionEntities[i]);
		myEntityManager->DestroyEntity(myMarkedForDeletionEntities[i]);
	}
	myMarkedDeleteCount = 0;
}
