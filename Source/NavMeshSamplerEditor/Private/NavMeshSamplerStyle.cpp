// NavMeshSamplerStyle.cpp
// Style implementation

#include "NavMeshSamplerStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FNavMeshSamplerStyle::StyleInstance = nullptr;

void FNavMeshSamplerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FNavMeshSamplerStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

void FNavMeshSamplerStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FNavMeshSamplerStyle::Get()
{
	return *StyleInstance;
}

FName FNavMeshSamplerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("NavMeshSamplerStyle"));
	return StyleSetName;
}

TSharedRef<FSlateStyleSet> FNavMeshSamplerStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("NavMeshSamplerStyle"));

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("NavMeshSampler");
	if (Plugin.IsValid())
	{
		Style->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
	}

	return Style;
}