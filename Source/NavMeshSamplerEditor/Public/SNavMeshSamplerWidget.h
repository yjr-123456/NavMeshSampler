// SNavMeshSamplerWidget.h
// Slate widget for NavMesh Sampler UI

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "NavMeshSamplerTypes.h"

class SNavMeshSamplerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNavMeshSamplerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// UI callbacks
	FReply OnBrowseNavMeshFile();
	FReply OnSamplePointsClicked();
	FReply OnFilterPointsClicked();
	FReply OnExportJSONClicked();
	FReply OnClearPointsClicked();
	FReply OnSpawnVisualizerActor();

	// Property change handlers
	void OnGridSpacingChanged(float NewValue);
	void OnMinClearHeightChanged(float NewValue);

	// State update
	void UpdateStatusText();
	void UpdateResultDisplay();

private:
	// Current state
	FString NavMeshFilePath;
	FNavMeshSamplerConfig SamplerConfig;
	FNavMeshWorldFilterConfig WorldFilterConfig;
	FNavMeshSamplingResult SamplingResult;

	// UI elements
	TSharedPtr<class SEditableTextBox> FilePathTextBox;
	TSharedPtr<class STextBlock> StatusTextBlock;
	TSharedPtr<class STextBlock> ResultTextBlock;

	// Cached values
	int32 LastPointCount = 0;
};