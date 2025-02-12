#pragma once
#include <deque>
#include <array>
#include <bitset>
#include <cassert>

#include "Entity.h"
#include "../component/Component.h"

class EntityManager
{
public:
	EntityManager() : myFrontIndex(0), myBackIndex(0), mySize(0)
	{
		myFreeEntities = new eid_t[MAX_ENTITIES];
		Reset();
	}

	~EntityManager()
	{
		delete[] myFreeEntities;
		mySize = 0;
		myFrontIndex = 0;
		myBackIndex = 0;
	}

	void Reset()
	{
		mySize = 0;
		myFrontIndex = 0;
		myBackIndex = 0;
		for (eid_t i = 0; i < MAX_ENTITIES; ++i)
		{
			DestroyEntity(i + INVALID_ENTITY + 1);
		}
	}

	Entity CreateEntity()
	{
		assert(mySize > 0 && "No entities to pop.");
		eid_t entity = myFreeEntities[myFrontIndex];
		myFrontIndex = (myFrontIndex + 1) % MAX_ENTITIES;
		--mySize;
		return entity;
	}

	void DestroyEntity(const Entity& aEntity)
	{
		assert(mySize < MAX_ENTITIES && "Too many entities in existence.");

		myFreeEntities[myBackIndex] = aEntity;
		myBackIndex = (myBackIndex + 1) % MAX_ENTITIES;
		++mySize;
	}
private:
	eid_t* myFreeEntities;
	size_t myFrontIndex;
	size_t myBackIndex;
	size_t mySize;
};
