// NavMeshSamplerEditor.h
// Editor module for NavMesh sampling visualization

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNavMeshSamplerEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Spawn the sampler widget tab */
	TSharedRef<class SDockTab> SpawnNavMeshSamplerTab(const class FSpawnTabArgs& TabSpawnArgs);

private:
	void RegisterTabSpawners();
	void AddMenuExtension(class FMenuBuilder& Builder);
	void AddToolbarExtension(class FToolBarBuilder& Builder);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};