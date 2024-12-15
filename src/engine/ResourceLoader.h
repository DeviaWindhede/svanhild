#pragma once

#include <vector>
#include <DX12.h>
#include <functional>
#include <any>

typedef std::function<void(class IResource*)> OnLoadedCallback;

struct ResourceCallback
{
	OnLoadedCallback callback = nullptr;
	size_t index = 0;
};

class ResourceLoader
{
public:
	ResourceLoader(DX12& aDx12);

	void Update();
	void LoadResource(class IResource* aResource, OnLoadedCallback aOnLoadedCallback = nullptr)
	{ 
		if (aOnLoadedCallback)
			activeCallbacks[resources.size()] = aOnLoadedCallback;

		resources.push_back(aResource);
	}
private:
	void PrepareLoad();
	void ExitLoad();

	std::vector<class IResource*> resources;
	std::unordered_map<size_t, std::any> activeCallbacks;
	DX12& dx12;
};

