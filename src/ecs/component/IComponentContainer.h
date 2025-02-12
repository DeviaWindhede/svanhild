#pragma once
#include "../entity/Entity.h"

class EntitySignatureCollection;

// interface in order to be stored in one vector within ComponentContainer.h
class IComponentContainer
{
public:
	virtual ~IComponentContainer() {}

	virtual void Reset() = 0;
	virtual void OnEntityDestroyed(const Entity& aEntity, EntitySignatureCollection& aCollection) = 0;
//#ifndef _RELEASE
//	virtual void OnEntityDestroyed(const EntityData& aEntity) = 0;
//#endif
};

