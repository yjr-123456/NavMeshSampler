// NavMeshSamplerCommands.h
// UI commands for NavMesh Sampler plugin

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "NavMeshSamplerStyle.h"

class FNavMeshSamplerCommands : public TCommands<FNavMeshSamplerCommands>
{
public:
	FNavMeshSamplerCommands()
		: TCommands<FNavMeshSamplerCommands>(
			TEXT("NavMeshSampler"),
			NSLOCTEXT("NavMeshSampler", "NavMeshSampler", "NavMesh Sampler Plugin"),
			NAME_None,
			FNavMeshSamplerStyle::GetStyleSetName()
		)
	{
	}

	// TCommands interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<class FUICommandInfo> OpenSamplerPanel;
	TSharedPtr<class FUICommandInfo> SamplePoints;
	TSharedPtr<class FUICommandInfo> ExportJSON;
	TSharedPtr<class FUICommandInfo> ClearPoints;
};