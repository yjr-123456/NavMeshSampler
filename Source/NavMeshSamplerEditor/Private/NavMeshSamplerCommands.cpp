// NavMeshSamplerCommands.cpp
// Commands implementation

#include "NavMeshSamplerCommands.h"

#define LOCTEXT_NAMESPACE "NavMeshSamplerCommands"

void FNavMeshSamplerCommands::RegisterCommands()
{
	UI_COMMAND(OpenSamplerPanel, "NavMesh Sampler", "Open the NavMesh Sampler panel", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SamplePoints, "Sample Points", "Sample points from NavMesh", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ExportJSON, "Export to JSON", "Export sampling results to JSON file", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ClearPoints, "Clear Points", "Clear all sampled points", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE