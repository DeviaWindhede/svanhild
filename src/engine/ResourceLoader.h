#pragma once

#include <vector>

class IResource
{
	friend class ResourceLoader;
public:
	virtual void LoadToGPU() = 0;
	virtual void UnloadGPU() = 0;
protected:
	
};

class ResourceLoader
{
public:
	void LoadResources();
private:
	void PrepareLoad();
	void ExitLoad();

	std::vector<IResource*> resources;
};

