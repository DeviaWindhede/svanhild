#pragma once

#include <DX12.h>
#include <vector>
#include <functional>
#include <memory>

class IFunction {
public:
	virtual ~IFunction() = default;
	virtual void execute(class IResource*) const = 0;
};

template <typename Ret>
class OnLoadedCallback : public IFunction {
public:
	typedef std::function<void(Ret*)> Callback;

	OnLoadedCallback(Callback func) : func_(func) {}

	void execute(class IResource* aOwner) const override {
		func_((Ret*)aOwner);
	}

private:
	std::function<void(Ret*)> func_;
};

struct PendingResource
{
	class IResource* resource = nullptr;
	IFunction* callback = nullptr;
};

class ResourceLoader
{
public:
	ResourceLoader(DX12& aDx12);
	~ResourceLoader();

	void Update();

	// TODO: Add proper mesh and texture loaders at integrate this function
	template<typename T = class IResource>
	void LoadResource(T* aResource, OnLoadedCallback<T>::Callback aOnLoadedCallback = nullptr)
	{ 
		resourcesToLoad.push_back(PendingResource{
			aResource,
			aOnLoadedCallback ? new OnLoadedCallback<T>(aOnLoadedCallback) : nullptr
		});
	}
private:
	void PrepareLoad();
	void ExitLoad();

	std::vector<PendingResource> resourcesToLoad;
	std::vector<class IResource*> activeResources;
	UINT resourceCounter = 0;
	// TODO: Custom heap allocator

	DX12& dx12;
};

