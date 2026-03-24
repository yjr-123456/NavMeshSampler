// FlibNavMeshSampler.h
// Blueprint function library for NavMesh sampling

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NavMeshSamplerTypes.h"
#include "FlibNavMeshSampler.generated.h"

class UdtNavMeshWrapper;
class FNavMeshPointSampler;

/**
 * Blueprint function library for NavMesh sampling operations
 */
UCLASS()
class NAVMESHSAMPLERRUNTIME_API UFlibNavMeshSampler : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Sample points from a NavMesh binary file using grid-based sampling
	 * @param NavMeshBinPath Path to the .bin NavMesh file
	 * @param Config Sampling configuration
	 * @param OutResult Result containing sampled points
	 * @return True if sampling was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	static bool SamplePointsFromNavMeshFile(const FString& NavMeshBinPath, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult);

	/**
	 * Sample points from a NavMesh wrapper object
	 * @param NavMeshWrapper The loaded NavMesh wrapper
	 * @param Config Sampling configuration
	 * @param OutResult Result containing sampled points
	 * @return True if sampling was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	static bool SamplePointsFromNavMeshWrapper(UdtNavMeshWrapper* NavMeshWrapper, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult);

	/**
	 * Sample points within a specific region
	 * @param NavMeshBinPath Path to the .bin NavMesh file
	 * @param Region The region to sample within
	 * @param Config Sampling configuration
	 * @param OutResult Result containing sampled points
	 * @return True if sampling was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	static bool SamplePointsInRegion(const FString& NavMeshBinPath, const FBox& Region, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult);

	/**
	 * Check if a point is on the NavMesh
	 * @param NavMeshBinPath Path to the .bin NavMesh file
	 * @param Point The point to check
	 * @param Extent The extent for validation
	 * @return True if the point is on the NavMesh
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NavMesh Sampler")
	static bool IsValidNavMeshPoint(const FString& NavMeshBinPath, const FVector& Point, const FVector& Extent);

	/**
	 * Get the bounds of a NavMesh file
	 * @param NavMeshBinPath Path to the .bin NavMesh file
	 * @param OutBounds The bounds of the NavMesh
	 * @return True if bounds were retrieved successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	static bool GetNavMeshBounds(const FString& NavMeshBinPath, FBox& OutBounds);

	/**
	 * Filter sampled points using world collision (removes points under overhangs, on obstacles, etc.)
	 * @param WorldContextObject World context for raycasting
	 * @param InOutResult Sampling result to filter (modified in place)
	 * @param FilterConfig Filter configuration
	 * @return Number of points removed
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler", meta = (WorldContext = "WorldContextObject"))
	static int32 FilterPointsWithWorldCollision(UObject* WorldContextObject, UPARAM(ref) FNavMeshSamplingResult& InOutResult, const FNavMeshWorldFilterConfig& FilterConfig);

	/**
	 * Check if a point has clear space above it (no overhangs)
	 * @param WorldContextObject World context for raycasting
	 * @param Point The point to check
	 * @param MinClearHeight Minimum clear height required
	 * @param TraceDistance Maximum distance to trace upward
	 * @param TraceChannel Collision channel to use
	 * @return True if point has clear space above
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NavMesh Sampler", meta = (WorldContext = "WorldContextObject"))
	static bool HasClearSpaceAbove(UObject* WorldContextObject, const FVector& Point, float MinClearHeight, float TraceDistance, ECollisionChannel TraceChannel);

	/**
	 * Export sampling result to JSON file
	 * @param Result The sampling result to export
	 * @param FilePath The output file path
	 * @return True if export was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	static bool ExportSamplingResultToJSON(const FNavMeshSamplingResult& Result, const FString& FilePath);

	/**
	 * Convert sampling result to JSON string
	 * @param Result The sampling result to convert
	 * @return JSON string representation
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NavMesh Sampler")
	static FString SamplingResultToJSON(const FNavMeshSamplingResult& Result);

	/**
	 * Extract only valid points from result
	 * @param Result The sampling result
	 * @return Array of valid point locations
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NavMesh Sampler")
	static TArray<FVector> GetValidPoints(const FNavMeshSamplingResult& Result);
};