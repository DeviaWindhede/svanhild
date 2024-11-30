#pragma once

#include <vector>
#include <DX12.h>

class ResourceLoader
{
public:
	ResourceLoader(DX12& aDx12);

	void Update();
	void LoadResource(class IResource* aResource) { resources.push_back(aResource); }
private:
	void PrepareLoad();
	void ExitLoad();

	std::vector<class IResource*> resources;
	DX12& dx12;
};

