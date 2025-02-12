#pragma once
#include "../component/ComponentSignature.h"

struct EntitySignature
{
	int indices[MAX_COMPONENTS] = {};
	ComponentSignature signature = {};
	const ComponentSignature& GetSignature() const { return signature; }
};