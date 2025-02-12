#pragma once

#include "../entity/Entity.h"

#include <set>

typedef unsigned long int sid_t;

class World;
struct SceneUpdateContext;

class ISystem
{
public:
	friend class SystemManager;
	virtual void Init() { __noop; }
	virtual void Update(SceneUpdateContext&) { __noop; }
	virtual void Render() { __noop; }
	virtual void Reset() { __noop; }
protected:
	virtual sid_t GetSystemId() const { return 0; }
	virtual void EraseEntity(Entity) { __noop; }
	virtual void InsertEntity(Entity) { __noop; }
};

// TODO: Maybe it would be a good idea to include a OnEntityAdded and OnEntityRemoved function?

template <typename Derived>
class System : public ISystem
{
private:
	static sid_t systemId;
public:
	friend class SystemManager;

	System(World* aWorld) { myWorld = aWorld; }
	virtual ~System()
	{
		myEntities.clear();
		myWorld = nullptr;
	}

	void Reset() override
	{
		myEntities.clear();
	}

	//void AddEntity(Entity entity);
	//void RemoveEntity(Entity entity);

	const std::set<Entity>& GetEntities() const { return myEntities; }
protected:
	sid_t GetSystemId() const override { return System<Derived>::systemId; }

	void EraseEntity(Entity aEntity) override
	{
		myEntities.erase(aEntity);
	}

	void InsertEntity(Entity aEntity) override
	{
		myEntities.insert(aEntity);
	}

	std::set<Entity> myEntities;
	World* myWorld;
};

template <typename Derived>
sid_t System<Derived>::systemId = 0;
