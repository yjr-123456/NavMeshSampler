// NavMeshSampler Plugin - Runtime Module
// Sample points from NavMesh for object placement candidates

#include "NavMeshSamplerRuntime.h"

DEFINE_LOG_CATEGORY(LogNavMeshSampler);

class FNavMeshSamplerRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogNavMeshSampler, Log, TEXT("NavMeshSamplerRuntime module loaded"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogNavMeshSampler, Log, TEXT("NavMeshSamplerRuntime module unloaded"));
	}
};

IMPLEMENT_MODULE(FNavMeshSamplerRuntimeModule, NavMeshSamplerRuntime)