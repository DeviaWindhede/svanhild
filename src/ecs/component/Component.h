#pragma once
#ifdef _EDITOR
#include "../entity/Entity.h"
#endif

typedef unsigned long int cid_t;

template<typename T>
class ComponentContainer;

// CRTP (Curiously Recurring Template Pattern) exploit >:3c
template <typename Derived>
struct Component
{
private:
	friend class ComponentManager;
	friend class ComponentContainer<Derived>;
	static cid_t componentId;
public:
	Component() = default;
#ifdef _EDITOR
	Entity myOwner = 0;
#endif
private:
	Component(const Component& aComponent) = default;
	Component& operator=(const Component& aComponent) = default;
};

// definition of the static variable
template <typename Derived>
cid_t Component<Derived>::componentId = 0;
