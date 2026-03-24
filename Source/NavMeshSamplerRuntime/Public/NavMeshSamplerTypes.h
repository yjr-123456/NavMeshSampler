// NavMeshSamplerTypes.h
// Data structures for NavMesh sampling

#pragma once

#include "CoreMinimal.h"
#include "NavMeshSamplerTypes.generated.h"

/** Result structure for a single sampled point */
USTRUCT(BlueprintType)
struct NAVMESHSAMPLERRUNTIME_API FNavMeshSamplePoint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	bool bIsValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	int32 TileIndex = -1;

	FNavMeshSamplePoint()
		: Location(FVector::ZeroVector)
		, bIsValid(false)
		, TileIndex(-1)
	{}

	FNavMeshSamplePoint(const FVector& InLocation, bool bInIsValid, int32 InTileIndex = -1)
		: Location(InLocation)
		, bIsValid(bInIsValid)
		, TileIndex(InTileIndex)
	{}
};

/** Configuration for sampling parameters */
USTRUCT(BlueprintType)
struct NAVMESHSAMPLERRUNTIME_API FNavMeshSamplerConfig
{
	GENERATED_BODY()

	/** Grid spacing in Unreal units (centimeters) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	float GridSpacing = 100.0f;

	/** Extent for point validation check */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	FVector ValidationExtent = FVector(10.0f, 10.0f, 10.0f);

	/** Minimum height above NavMesh surface for valid placement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	float MinHeightAboveNavMesh = 0.0f;

	/** Maximum height above NavMesh surface for valid placement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	float MaxHeightAboveNavMesh = 500.0f;

	/** Whether to filter points near walls (using NavMesh raycast) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	bool bFilterNearWalls = false;

	/** Minimum distance from walls (if filtering enabled) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	float MinDistanceFromWall = 50.0f;

	FNavMeshSamplerConfig()
		: GridSpacing(100.0f)
		, ValidationExtent(10.0f, 10.0f, 10.0f)
		, MinHeightAboveNavMesh(0.0f)
		, MaxHeightAboveNavMesh(500.0f)
		, bFilterNearWalls(false)
		, MinDistanceFromWall(50.0f)
	{}
};

/** Configuration for world-space filtering (requires UWorld access) */
USTRUCT(BlueprintType)
struct NAVMESHSAMPLERRUNTIME_API FNavMeshWorldFilterConfig
{
	GENERATED_BODY()

	/** Whether to filter points under overhangs/roofs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	bool bFilterUnderOverhangs = true;

	/** Minimum clear height above the point (in cm) - points with obstacles within this height are filtered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	float MinClearHeightAbove = 200.0f;

	/** Maximum distance to trace upward for overhang detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	float OverhangTraceDistance = 500.0f;

	/** Collision channel to use for overhang detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	TEnumAsByte<ECollisionChannel> OverhangTraceChannel = ECC_Visibility;

	/** Whether to filter points inside or on obstacles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	bool bFilterOnObstacles = true;

	/** Collision channel to use for obstacle detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Sampler")
	TEnumAsByte<ECollisionChannel> ObstacleTraceChannel = ECC_Visibility;

	FNavMeshWorldFilterConfig()
		: bFilterUnderOverhangs(true)
		, MinClearHeightAbove(200.0f)
		, OverhangTraceDistance(500.0f)
		, OverhangTraceChannel(ECC_Visibility)
		, bFilterOnObstacles(true)
		, ObstacleTraceChannel(ECC_Visibility)
	{}
};

/** Result structure for sampling operation */
USTRUCT(BlueprintType)
struct NAVMESHSAMPLERRUNTIME_API FNavMeshSamplingResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	TArray<FNavMeshSamplePoint> SamplePoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	int32 TotalPoints = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	int32 ValidPoints = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	FBox NavMeshBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	float TotalArea = 0.0f;

	void Reset()
	{
		SamplePoints.Empty();
		TotalPoints = 0;
		ValidPoints = 0;
		NavMeshBounds = FBox(ForceInit);
		TotalArea = 0.0f;
	}
};