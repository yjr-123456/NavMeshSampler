// SNavMeshSamplerWidget.cpp
// Slate widget implementation

#include "SNavMeshSamplerWidget.h"
#include "NavMeshSamplerStyle.h"
#include "NavMeshSamplerCommands.h"
#include "NavMeshSamplerActor.h"
#include "FlibNavMeshSampler.h"
#include "NavMeshSamplerRuntime.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "NavMeshSamplerUI"

SNavMeshSamplerWidget::~SNavMeshSamplerWidget()
{
	// Clear visualization when widget is closed
	ClearVisualizationActors();
}

void SNavMeshSamplerWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				// File Path Section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SAssignNew(FilePathTextBox, SEditableTextBox)
						.HintText(LOCTEXT("SelectNavMeshFile", "Select NavMesh .bin file..."))
						.Text(FText::FromString(NavMeshFilePath))
						.IsReadOnly(true)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("Browse", "Browse..."))
						.OnClicked(this, &SNavMeshSamplerWidget::OnBrowseNavMeshFile)
					]
				]

				// Sampling Parameters Section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(8.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SamplingParameters", "Sampling Parameters"))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("GridSpacing", "Grid Spacing (cm):"))
								.MinDesiredWidth(140.0f)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(4.0f, 0.0f)
							[
								SNew(SSpinBox<float>)
								.Value(SamplerConfig.GridSpacing)
								.MinValue(10.0f)
								.MaxValue(1000.0f)
								.MinSliderValue(10.0f)
								.MaxSliderValue(500.0f)
								.OnValueChanged(this, &SNavMeshSamplerWidget::OnGridSpacingChanged)
							]
						]
					]
				]

				// World Filter Parameters Section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(8.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("WorldFilterParameters", "World Filter (Overhangs/Obstacles)"))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("MinClearHeight", "Min Clear Height (cm):"))
								.MinDesiredWidth(140.0f)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(4.0f, 0.0f)
							[
								SNew(SSpinBox<float>)
								.Value(WorldFilterConfig.MinClearHeightAbove)
								.MinValue(50.0f)
								.MaxValue(1000.0f)
								.MinSliderValue(50.0f)
								.MaxSliderValue(500.0f)
								.OnValueChanged(this, &SNavMeshSamplerWidget::OnMinClearHeightChanged)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("FilterDescription", "Filters points under eaves, roofs, and low obstacles"))
							.ColorAndOpacity(FLinearColor::Gray)
						]
					]
				]

				// Action Buttons - Row 1
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 0.0f, 2.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("SamplePoints", "Sample Points"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SNavMeshSamplerWidget::OnSamplePointsClicked)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("FilterPoints", "Filter Overhangs"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SNavMeshSamplerWidget::OnFilterPointsClicked)
					]
				]

				// Action Buttons - Row 2
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 0.0f, 2.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ExportJSON", "Export to JSON"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SNavMeshSamplerWidget::OnExportJSONClicked)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("SpawnVisualizer", "Spawn Visualizer"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SNavMeshSamplerWidget::OnSpawnVisualizerActor)
					]
				]

				// Clear Button
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearPoints", "Clear All Points"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SNavMeshSamplerWidget::OnClearPointsClicked)
				]

				// Results Section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(8.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Results", "Results"))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(ResultTextBlock, STextBlock)
							.Text(LOCTEXT("NoResults", "No points sampled yet."))
							.AutoWrapText(true)
						]
					]
				]

				// Status Section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("Ready", "Ready"))
					.ColorAndOpacity(FLinearColor::Green)
				]
			]
		]
	];

	UpdateStatusText();
}

FReply SNavMeshSamplerWidget::OnBrowseNavMeshFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		const FString DefaultPath = FPaths::ProjectDir() / TEXT("NavMesh");

		bool bOpened = DesktopPlatform->OpenFileDialog(
			nullptr,
			LOCTEXT("OpenNavMeshFile", "Open NavMesh File").ToString(),
			DefaultPath,
			TEXT(""),
			TEXT("NavMesh Binary Files (*.bin)|*.bin|All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFiles
		);

		if (bOpened && OutFiles.Num() > 0)
		{
			NavMeshFilePath = OutFiles[0];
			FilePathTextBox->SetText(FText::FromString(NavMeshFilePath));
			UpdateStatusText();
		}
	}

	return FReply::Handled();
}

FReply SNavMeshSamplerWidget::OnSamplePointsClicked()
{
	if (NavMeshFilePath.IsEmpty())
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorNoFile", "Error: No NavMesh file selected"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
		return FReply::Handled();
	}

	if (!FPaths::FileExists(NavMeshFilePath))
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorFileNotFound", "Error: NavMesh file not found"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
		return FReply::Handled();
	}

	StatusTextBlock->SetText(LOCTEXT("Sampling", "Sampling points..."));
	StatusTextBlock->SetColorAndOpacity(FLinearColor::Yellow);

	// Perform sampling
	bool bSuccess = UFlibNavMeshSampler::SamplePointsFromNavMeshFile(NavMeshFilePath, SamplerConfig, SamplingResult);

	if (bSuccess)
	{
		UpdateResultDisplay();
		StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Sampled: %d valid points"), SamplingResult.ValidPoints)));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Green);
	}
	else
	{
		StatusTextBlock->SetText(LOCTEXT("SamplingFailed", "Sampling failed. Check log for details."));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
	}

	return FReply::Handled();
}

FReply SNavMeshSamplerWidget::OnFilterPointsClicked()
{
	if (SamplingResult.ValidPoints == 0)
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorNoPointsToFilter", "Error: No points to filter"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
		return FReply::Handled();
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorNoWorld", "Error: No editor world available"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
		return FReply::Handled();
	}

	StatusTextBlock->SetText(LOCTEXT("Filtering", "Filtering points..."));
	StatusTextBlock->SetColorAndOpacity(FLinearColor::Yellow);

	int32 RemovedCount = UFlibNavMeshSampler::FilterPointsWithWorldCollision(World, SamplingResult, WorldFilterConfig);

	UpdateResultDisplay();
	StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Filtered: removed %d points, %d remaining"), RemovedCount, SamplingResult.ValidPoints)));
	StatusTextBlock->SetColorAndOpacity(FLinearColor::Green);

	return FReply::Handled();
}

FReply SNavMeshSamplerWidget::OnExportJSONClicked()
{
	if (SamplingResult.ValidPoints == 0)
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorNoPoints", "Error: No points to export"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
		return FReply::Handled();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		FString DefaultPath = FPaths::ProjectDir() / TEXT("NavMesh");
		FString DefaultFileName = FPaths::GetBaseFilename(NavMeshFilePath) + TEXT("_samples.json");

		bool bSaved = DesktopPlatform->SaveFileDialog(
			nullptr,
			LOCTEXT("SaveJSONFile", "Save Sampling Results as JSON").ToString(),
			DefaultPath,
			DefaultFileName,
			TEXT("JSON Files (*.json)|*.json|All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFiles
		);

		if (bSaved && OutFiles.Num() > 0)
		{
			FString ExportPath = OutFiles[0];
			if (UFlibNavMeshSampler::ExportSamplingResultToJSON(SamplingResult, ExportPath))
			{
				StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Exported to: %s"), *ExportPath)));
				StatusTextBlock->SetColorAndOpacity(FLinearColor::Green);
			}
			else
			{
				StatusTextBlock->SetText(LOCTEXT("ExportFailed", "Failed to export JSON file"));
				StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
			}
		}
	}

	return FReply::Handled();
}

FReply SNavMeshSamplerWidget::OnClearPointsClicked()
{
	SamplingResult.Reset();

	// Clear all visualization actors (flushes debug lines)
	ClearVisualizationActors();

	UpdateResultDisplay();
	StatusTextBlock->SetText(LOCTEXT("PointsCleared", "Points cleared"));
	StatusTextBlock->SetColorAndOpacity(FLinearColor::Green);

	return FReply::Handled();
}

FReply SNavMeshSamplerWidget::OnSpawnVisualizerActor()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorNoWorld", "Error: No editor world available"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
		return FReply::Handled();
	}

	// Check if there's already a sampler actor
	TArray<AActor*> ExistingActors;
	UGameplayStatics::GetAllActorsOfClass(World, ANavMeshSamplerActor::StaticClass(), ExistingActors);

	ANavMeshSamplerActor* SamplerActor = nullptr;
	if (ExistingActors.Num() > 0)
	{
		SamplerActor = Cast<ANavMeshSamplerActor>(ExistingActors[0]);
	}
	else
	{
		// Spawn new actor
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = FName(TEXT("NavMeshSamplerVisualizer"));
		SamplerActor = World->SpawnActor<ANavMeshSamplerActor>(ANavMeshSamplerActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	if (SamplerActor)
	{
		SamplerActor->SetSamplingResult(SamplingResult);
		StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Visualizer spawned with %d points"), SamplingResult.ValidPoints)));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Green);

		// Select the actor in the editor
		GEditor->SelectActor(SamplerActor, true, true, true, true);
	}
	else
	{
		StatusTextBlock->SetText(LOCTEXT("ErrorSpawnActor", "Failed to spawn visualizer actor"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Red);
	}

	return FReply::Handled();
}

void SNavMeshSamplerWidget::OnGridSpacingChanged(float NewValue)
{
	SamplerConfig.GridSpacing = NewValue;
}

void SNavMeshSamplerWidget::OnMinClearHeightChanged(float NewValue)
{
	WorldFilterConfig.MinClearHeightAbove = NewValue;
}

void SNavMeshSamplerWidget::ClearVisualizationActors()
{
	if (GEditor)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (World)
		{
			// Find and clear all NavMeshSampler actors
			TArray<AActor*> Actors;
			UGameplayStatics::GetAllActorsOfClass(World, ANavMeshSamplerActor::StaticClass(), Actors);
			for (AActor* Actor : Actors)
			{
				if (ANavMeshSamplerActor* SamplerActor = Cast<ANavMeshSamplerActor>(Actor))
				{
					SamplerActor->ClearPoints();
				}
			}
		}
	}
}

void SNavMeshSamplerWidget::UpdateStatusText()
{
	if (NavMeshFilePath.IsEmpty())
	{
		StatusTextBlock->SetText(LOCTEXT("SelectNavMesh", "Select a NavMesh file to begin"));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Ready: %s"), *FPaths::GetCleanFilename(NavMeshFilePath))));
		StatusTextBlock->SetColorAndOpacity(FLinearColor::Green);
	}
}

void SNavMeshSamplerWidget::UpdateResultDisplay()
{
	if (SamplingResult.ValidPoints > 0)
	{
		FString ResultText = FString::Printf(
			TEXT("Total Points: %d\nValid Points: %d\nApproximate Area: %.2f m²\n\nBounds:\n  Min: (%.1f, %.1f, %.1f)\n  Max: (%.1f, %.1f, %.1f)"),
			SamplingResult.TotalPoints,
			SamplingResult.ValidPoints,
			SamplingResult.TotalArea / 10000.0f,  // Convert cm² to m²
			SamplingResult.NavMeshBounds.Min.X,
			SamplingResult.NavMeshBounds.Min.Y,
			SamplingResult.NavMeshBounds.Min.Z,
			SamplingResult.NavMeshBounds.Max.X,
			SamplingResult.NavMeshBounds.Max.Y,
			SamplingResult.NavMeshBounds.Max.Z
		);
		ResultTextBlock->SetText(FText::FromString(ResultText));
	}
	else
	{
		ResultTextBlock->SetText(LOCTEXT("NoValidPoints", "No valid points found."));
	}
}

#undef LOCTEXT_NAMESPACE