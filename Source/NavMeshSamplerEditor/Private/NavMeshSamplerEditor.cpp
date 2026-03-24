// NavMeshSamplerEditor.cpp
// Editor module implementation

#include "NavMeshSamplerEditor.h"
#include "NavMeshSamplerStyle.h"
#include "NavMeshSamplerCommands.h"
#include "SNavMeshSamplerWidget.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"

static const FName NavMeshSamplerTabName("NavMeshSampler");

#define LOCTEXT_NAMESPACE "FNavMeshSamplerEditorModule"

void FNavMeshSamplerEditorModule::StartupModule()
{
	FNavMeshSamplerStyle::Initialize();
	FNavMeshSamplerStyle::ReloadTextures();

	FNavMeshSamplerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FNavMeshSamplerCommands::Get().OpenSamplerPanel,
		FExecuteAction::CreateLambda([this]() {
			FGlobalTabmanager::Get()->TryInvokeTab(NavMeshSamplerTabName);
		})
	);

	RegisterTabSpawners();

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FNavMeshSamplerEditorModule::AddMenuExtension));
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FNavMeshSamplerEditorModule::ShutdownModule()
{
	FNavMeshSamplerStyle::Shutdown();
	FNavMeshSamplerCommands::Unregister();
}

void FNavMeshSamplerEditorModule::RegisterTabSpawners()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(NavMeshSamplerTabName, FOnSpawnTab::CreateRaw(this, &FNavMeshSamplerEditorModule::SpawnNavMeshSamplerTab))
		.SetDisplayName(LOCTEXT("NavMeshSamplerTabTitle", "NavMesh Sampler"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

TSharedRef<SDockTab> FNavMeshSamplerEditorModule::SpawnNavMeshSamplerTab(const FSpawnTabArgs& TabSpawnArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SNavMeshSamplerWidget)
		];
}

void FNavMeshSamplerEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FNavMeshSamplerCommands::Get().OpenSamplerPanel);
}

void FNavMeshSamplerEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FNavMeshSamplerCommands::Get().OpenSamplerPanel);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNavMeshSamplerEditorModule, NavMeshSamplerEditor)