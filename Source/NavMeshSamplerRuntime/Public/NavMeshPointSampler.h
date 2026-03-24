// NavMeshPointSampler.h
// Core sampling logic for NavMesh point extraction

#pragma once

#include "CoreMinimal.h"
#include "NavMeshSamplerTypes.h"
#include "Detour/DetourNavMesh.h"
#include "Detour/DetourNavMeshQuery.h"

class UdtNavMeshWrapper;

/**
 * NavMesh Point Sampler
 * Performs grid-based sampling on a NavMesh to find candidate placement points
 */
class NAVMESHSAMPLERRUNTIME_API FNavMeshPointSampler
{
public:
	FNavMeshPointSampler();
	~FNavMeshPointSampler();

	/** Initialize sampler with a NavMesh binary file */
	bool Initialize(const FString& NavMeshBinPath);

	/** Initialize sampler with a dtNavMesh pointer (does not take ownership) */
	bool Initialize(const dtNavMesh* InNavMesh);

	/** Check if sampler has valid NavMesh data */
	bool IsValid() const { return NavMesh != nullptr; }

	/** Get NavMesh bounds in Unreal coordinates */
	FBox GetNavMeshBounds() const;

	/** Sample points using grid-based method */
	bool SampleGridPoints(const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult);

	/** Sample points within a specific region */
	bool SampleGridPointsInRegion(const FBox& Region, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult);

	/** Check if a point is on the NavMesh */
	bool IsValidPoint(const FVector& Point, const FVector& Extent) const;

	/** Get the underlying NavMesh pointer */
	const dtNavMesh* GetNavMesh() const { return NavMesh; }

private:
	/** Load NavMesh from binary file */
	dtNavMesh* LoadNavMeshFromFile(const FString& FilePath);

	/** Convert Recast coordinate to Unreal coordinate */
	FVector RecastToUnreal(const dtReal RecastPoint[3]) const;

	/** Convert Unreal coordinate to Recast coordinate */
	void UnrealToRecast(const FVector& UnrealPoint, dtReal RecastPoint[3]) const;

	/** Calculate bounds from all tiles */
	void CalculateBounds();

	/** Find nearest polygon for a point */
	bool FindNearestPoly(const dtReal Point[3], const dtReal Extent[3], dtPolyRef& OutPolyRef, dtReal OutNearestPoint[3]) const;

	/** Find the largest connected region in the NavMesh (typically the ground) */
	void FindLargestConnectedRegion();

	/** Check if a polygon belongs to the largest connected region */
	bool IsInLargestRegion(dtPolyRef PolyRef) const;

	/** Get all polygons within a search box */
	int32 GetPolysInBox(const dtReal* Center, const dtReal* Extent, dtPolyRef* OutPolys, int32 MaxPolys) const;

private:
	const dtNavMesh* NavMesh;
	dtNavMeshQuery* NavQuery;
	dtQueryFilter* QueryFilter;
	FBox CachedBounds;
	bool bOwnsNavMesh;

	/** Set of polygon references that belong to the largest connected region (ground) */
	TSet<dtPolyRef> GroundRegionPolys;

	/** Whether the largest region has been computed */
	bool bGroundRegionComputed;
};