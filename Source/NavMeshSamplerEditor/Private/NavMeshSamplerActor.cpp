// NavMeshSamplerActor.cpp
// Visualization actor implementation

#include "NavMeshSamplerActor.h"
#include "DrawDebugHelpers.h"

ANavMeshSamplerActor::ANavMeshSamplerActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);

	// Create root component
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);
}

void ANavMeshSamplerActor::BeginPlay()
{
	Super::BeginPlay();
}

void ANavMeshSamplerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDrawPoints || bDrawBounds)
	{
		DrawVisualization();
	}
}

void ANavMeshSamplerActor::SetSamplePoints(const TArray<FVector>& Points)
{
	SamplingResult.SamplePoints.Empty();
	SamplingResult.SamplePoints.Reserve(Points.Num());

	for (const FVector& Point : Points)
	{
		SamplingResult.SamplePoints.Add(FNavMeshSamplePoint(Point, true));
	}

	SamplingResult.ValidPoints = Points.Num();
	SamplingResult.TotalPoints = Points.Num();
}

void ANavMeshSamplerActor::SetSamplingResult(const FNavMeshSamplingResult& Result)
{
	SamplingResult = Result;
}

void ANavMeshSamplerActor::ClearPoints()
{
	SamplingResult.Reset();
}

void ANavMeshSamplerActor::DrawVisualization()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	// Draw NavMesh bounds
	if (bDrawBounds && SamplingResult.NavMeshBounds.IsValid)
	{
		DrawDebugBox(World, SamplingResult.NavMeshBounds.GetCenter(), SamplingResult.NavMeshBounds.GetExtent(), FColor::Yellow, false, -1.0f, SDPG_World, 1.0f);
	}

	// Draw sample points
	if (bDrawPoints)
	{
		for (const FNavMeshSamplePoint& SamplePoint : SamplingResult.SamplePoints)
		{
			if (SamplePoint.bIsValid)
			{
				DrawDebugPoint(World, SamplePoint.Location, PointSize, PointColor, false, -1.0f, SDPG_World);
			}
		}
	}
}