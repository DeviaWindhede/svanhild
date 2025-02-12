#pragma once
#include <deque>
#include <array>
#include <bitset>
#include <cassert>

#include "../component/Component.h"
#include "Entity.h"
#include "EntitySignature.h"

class EntitySignatureCollection
{
public:
	EntitySignatureCollection()
	{
		mySignatures = new EntitySignature[MAX_ENTITIES];
	}

	~EntitySignatureCollection()
	{
		delete[] mySignatures;
		mySignatures = nullptr;
	}

	void Reset()
	{
		for (size_t i = 0; i < MAX_ENTITIES; i++)
		{
			memset(mySignatures[i].indices, -1, sizeof(mySignatures[i].indices));
			mySignatures[i].signature.reset();
		}
	}

	void ResetComponentSignature(const Entity& aEntity)
	{
		assert(aEntity < MAX_ENTITIES && "Entity out of range.");

		mySignatures[aEntity].signature.reset();
	}

	void UpdateComponentSignature(const Entity& aEntity, cid_t aComponentId, bool aValue)
	{
		assert(aEntity < MAX_ENTITIES && "Entity out of range.");

		mySignatures[aEntity].signature[aComponentId] = aValue;
	}

	int GetSignatureIndex(const Entity& aEntity, cid_t aComponentId) const
	{
		//assert(aEntity < MAX_ENTITIES && "Entity out of range.");

		return mySignatures[aEntity].indices[aComponentId];
	}

	void SetSignatureIndex(const Entity& aEntity, cid_t aComponentId, int aIndex)
	{
		assert(aEntity < MAX_ENTITIES && "Entity out of range.");

		mySignatures[aEntity].indices[aComponentId] = aIndex;
	}

	const ComponentSignature& GetComponentSignature(const Entity& aEntity) const
	{
		assert(aEntity < MAX_ENTITIES && "Entity out of range.");

		return mySignatures[aEntity].signature;
	}
private:
	EntitySignature* mySignatures;
};
