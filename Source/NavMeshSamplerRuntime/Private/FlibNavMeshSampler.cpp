// FlibNavMeshSampler.cpp
// Blueprint function library implementation

#include "FlibNavMeshSampler.h"
#include "NavMeshPointSampler.h"
#include "NavMeshSamplerRuntime.h"
#include "dtNavMeshWrapper.h"
#include "Json.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

bool UFlibNavMeshSampler::SamplePointsFromNavMeshFile(const FString& NavMeshBinPath, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult)
{
	FNavMeshPointSampler Sampler;
	if (!Sampler.Initialize(NavMeshBinPath))
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to initialize sampler for: %s"), *NavMeshBinPath);
		return false;
	}

	return Sampler.SampleGridPoints(Config, OutResult);
}

bool UFlibNavMeshSampler::SamplePointsFromNavMeshWrapper(UdtNavMeshWrapper* NavMeshWrapper, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult)
{
	if (!NavMeshWrapper || !NavMeshWrapper->IsAvailableNavData())
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Invalid NavMesh wrapper"));
		return false;
	}

	FNavMeshPointSampler Sampler;
	if (!Sampler.Initialize(NavMeshWrapper->GetNavData()))
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to initialize sampler from wrapper"));
		return false;
	}

	return Sampler.SampleGridPoints(Config, OutResult);
}

bool UFlibNavMeshSampler::SamplePointsInRegion(const FString& NavMeshBinPath, const FBox& Region, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult)
{
	FNavMeshPointSampler Sampler;
	if (!Sampler.Initialize(NavMeshBinPath))
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to initialize sampler for: %s"), *NavMeshBinPath);
		return false;
	}

	return Sampler.SampleGridPointsInRegion(Region, Config, OutResult);
}

bool UFlibNavMeshSampler::IsValidNavMeshPoint(const FString& NavMeshBinPath, const FVector& Point, const FVector& Extent)
{
	FNavMeshPointSampler Sampler;
	if (!Sampler.Initialize(NavMeshBinPath))
	{
		return false;
	}

	return Sampler.IsValidPoint(Point, Extent);
}

bool UFlibNavMeshSampler::GetNavMeshBounds(const FString& NavMeshBinPath, FBox& OutBounds)
{
	FNavMeshPointSampler Sampler;
	if (!Sampler.Initialize(NavMeshBinPath))
	{
		return false;
	}

	OutBounds = Sampler.GetNavMeshBounds();
	return true;
}

int32 UFlibNavMeshSampler::FilterPointsWithWorldCollision(UObject* WorldContextObject, FNavMeshSamplingResult& InOutResult, const FNavMeshWorldFilterConfig& FilterConfig)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		UE_LOG(LogNavMeshSampler, Warning, TEXT("FilterPointsWithWorldCollision: No valid world context"));
		return 0;
	}

	int32 RemovedCount = 0;
	TArray<FNavMeshSamplePoint> FilteredPoints;
	FilteredPoints.Reserve(InOutResult.SamplePoints.Num());

	for (const FNavMeshSamplePoint& Point : InOutResult.SamplePoints)
	{
		if (!Point.bIsValid)
			continue;

		bool bKeepPoint = true;
		FVector TraceStart = Point.Location + FVector(0, 0, 1.0f); // Slightly above surface
		FVector TraceEnd = TraceStart + FVector(0, 0, FilterConfig.OverhangTraceDistance);

		// Check for overhangs/obstacles above
		if (FilterConfig.bFilterUnderOverhangs)
		{
			FHitResult HitResult;
			FCollisionQueryParams QueryParams(NAME_None, true);
			QueryParams.bTraceComplex = false;

			bool bHit = World->LineTraceSingleByChannel(
				HitResult,
				TraceStart,
				TraceEnd,
				FilterConfig.OverhangTraceChannel,
				QueryParams
			);

			if (bHit)
			{
				float ClearHeight = HitResult.Location.Z - Point.Location.Z;
				if (ClearHeight < FilterConfig.MinClearHeightAbove)
				{
					bKeepPoint = false;
				}
			}
		}

		// Check if point is inside/on an obstacle (trace downward)
		if (bKeepPoint && FilterConfig.bFilterOnObstacles)
		{
			FHitResult HitResult;
			FCollisionQueryParams QueryParams(NAME_None, true);
			QueryParams.bTraceComplex = false;

			FVector DownTraceStart = Point.Location + FVector(0, 0, 50.0f);
			FVector DownTraceEnd = Point.Location - FVector(0, 0, 50.0f);

			bool bHit = World->LineTraceSingleByChannel(
				HitResult,
				DownTraceStart,
				DownTraceEnd,
				FilterConfig.ObstacleTraceChannel,
				QueryParams
			);

			// If we hit something and it's not at the expected NavMesh surface level, filter it
			if (bHit)
			{
				float SurfaceDelta = FMath::Abs(HitResult.Location.Z - Point.Location.Z);
				if (SurfaceDelta > 10.0f) // 10cm tolerance
				{
					// The traced surface is not at the NavMesh point height
					// This indicates the point is on top of an object (car, roof, etc.)
					// Note: This filter is now optional and can be enabled via bFilterOnObstacles
					bKeepPoint = false;
				}
			}
		}

		if (bKeepPoint)
		{
			FilteredPoints.Add(Point);
		}
		else
		{
			RemovedCount++;
		}
	}

	InOutResult.SamplePoints = FilteredPoints;
	InOutResult.ValidPoints = FilteredPoints.Num();

	UE_LOG(LogNavMeshSampler, Log, TEXT("FilterPointsWithWorldCollision: Removed %d points, %d remaining"),
		RemovedCount, InOutResult.ValidPoints);

	return RemovedCount;
}

bool UFlibNavMeshSampler::HasClearSpaceAbove(UObject* WorldContextObject, const FVector& Point, float MinClearHeight, float TraceDistance, ECollisionChannel TraceChannel)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return false;
	}

	FVector TraceStart = Point + FVector(0, 0, 1.0f);
	FVector TraceEnd = TraceStart + FVector(0, 0, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(NAME_None, true);
	QueryParams.bTraceComplex = false;

	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

	if (bHit)
	{
		float ClearHeight = HitResult.Location.Z - Point.Z;
		return ClearHeight >= MinClearHeight;
	}

	return true; // No hit means clear space
}

bool UFlibNavMeshSampler::ExportSamplingResultToJSON(const FNavMeshSamplingResult& Result, const FString& FilePath)
{
	FString JsonString = SamplingResultToJSON(Result);
	if (JsonString.IsEmpty())
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to generate JSON string"));
		return false;
	}

	// Create directory if it doesn't exist
	FString Directory = FPaths::GetPath(FilePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!Directory.IsEmpty() && !PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	// Write to file
	if (FFileHelper::SaveStringToFile(JsonString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogNavMeshSampler, Log, TEXT("Exported sampling result to: %s"), *FilePath);
		return true;
	}

	UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to write JSON to: %s"), *FilePath);
	return false;
}

FString UFlibNavMeshSampler::SamplingResultToJSON(const FNavMeshSamplingResult& Result)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	// Add metadata
	JsonObject->SetNumberField(TEXT("totalPoints"), Result.TotalPoints);
	JsonObject->SetNumberField(TEXT("validPoints"), Result.ValidPoints);
	JsonObject->SetNumberField(TEXT("totalArea"), Result.TotalArea);

	// Add bounds
	TSharedPtr<FJsonObject> BoundsObject = MakeShareable(new FJsonObject);
	BoundsObject->SetArrayField(TEXT("min"), TArray<TSharedPtr<FJsonValue>>{
		MakeShareable(new FJsonValueNumber(Result.NavMeshBounds.Min.X)),
		MakeShareable(new FJsonValueNumber(Result.NavMeshBounds.Min.Y)),
		MakeShareable(new FJsonValueNumber(Result.NavMeshBounds.Min.Z))
	});
	BoundsObject->SetArrayField(TEXT("max"), TArray<TSharedPtr<FJsonValue>>{
		MakeShareable(new FJsonValueNumber(Result.NavMeshBounds.Max.X)),
		MakeShareable(new FJsonValueNumber(Result.NavMeshBounds.Max.Y)),
		MakeShareable(new FJsonValueNumber(Result.NavMeshBounds.Max.Z))
	});
	JsonObject->SetObjectField(TEXT("bounds"), BoundsObject);

	// Add points array
	TArray<TSharedPtr<FJsonValue>> PointsArray;
	for (const FNavMeshSamplePoint& Point : Result.SamplePoints)
	{
		if (Point.bIsValid)
		{
			TSharedPtr<FJsonObject> PointObject = MakeShareable(new FJsonObject);
			PointObject->SetArrayField(TEXT("location"), TArray<TSharedPtr<FJsonValue>>{
				MakeShareable(new FJsonValueNumber(Point.Location.X)),
				MakeShareable(new FJsonValueNumber(Point.Location.Y)),
				MakeShareable(new FJsonValueNumber(Point.Location.Z))
			});
			PointObject->SetBoolField(TEXT("isValid"), Point.bIsValid);

			TSharedPtr<FJsonValueObject> PointValue = MakeShareable(new FJsonValueObject(PointObject));
			PointsArray.Add(PointValue);
		}
	}
	JsonObject->SetArrayField(TEXT("points"), PointsArray);

	// Convert to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return OutputString;
}

TArray<FVector> UFlibNavMeshSampler::GetValidPoints(const FNavMeshSamplingResult& Result)
{
	TArray<FVector> ValidPoints;
	ValidPoints.Reserve(Result.ValidPoints);

	for (const FNavMeshSamplePoint& Point : Result.SamplePoints)
	{
		if (Point.bIsValid)
		{
			ValidPoints.Add(Point.Location);
		}
	}

	return ValidPoints;
}