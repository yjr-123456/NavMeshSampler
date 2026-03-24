// NavMeshPointSampler.cpp
// Core sampling logic implementation

#include "NavMeshPointSampler.h"
#include "NavMeshSamplerRuntime.h"
#include "dtNavMeshWrapper.h"
#include "Detour/DetourStatus.h"
#include "Detour/DetourAlloc.h"
#include "Detour/DetourNavMesh.h"
#include "Misc/Paths.h"
#include <cstring>
#include <cstdio>

// NavMesh serialization constants
static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
static const int NAVMESHSET_VERSION = 1;

struct NavMeshSetHeader
{
	int32 magic;
	int32 version;
	int32 numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int32 dataSize;
};

FNavMeshPointSampler::FNavMeshPointSampler()
	: NavMesh(nullptr)
	, NavQuery(nullptr)
	, QueryFilter(nullptr)
	, CachedBounds(ForceInit)
	, bOwnsNavMesh(false)
	, bGroundRegionComputed(false)
{
}

FNavMeshPointSampler::~FNavMeshPointSampler()
{
	if (NavQuery)
	{
		dtFreeNavMeshQuery(NavQuery);
		NavQuery = nullptr;
	}

	if (QueryFilter)
	{
		delete QueryFilter;
		QueryFilter = nullptr;
	}

	if (NavMesh && bOwnsNavMesh)
	{
		dtFreeNavMesh(const_cast<dtNavMesh*>(NavMesh));
		NavMesh = nullptr;
	}
}

dtNavMesh* FNavMeshPointSampler::LoadNavMeshFromFile(const FString& FilePath)
{
	FILE* fp = nullptr;
#if PLATFORM_WINDOWS
	fopen_s(&fp, TCHAR_TO_ANSI(*FilePath), "rb");
#else
	fp = fopen(TCHAR_TO_ANSI(*FilePath), "rb");
#endif

	if (!fp)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to open file: %s"), *FilePath);
		return nullptr;
	}

	// Read header
	NavMeshSetHeader header;
	size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
	if (readLen != 1)
	{
		fclose(fp);
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to read header from: %s"), *FilePath);
		return nullptr;
	}

	if (header.magic != NAVMESHSET_MAGIC)
	{
		fclose(fp);
		UE_LOG(LogNavMeshSampler, Error, TEXT("Invalid NavMesh magic in: %s"), *FilePath);
		return nullptr;
	}

	if (header.version != NAVMESHSET_VERSION)
	{
		fclose(fp);
		UE_LOG(LogNavMeshSampler, Error, TEXT("Unsupported NavMesh version in: %s"), *FilePath);
		return nullptr;
	}

	dtNavMesh* mesh = dtAllocNavMesh();
	if (!mesh)
	{
		fclose(fp);
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to allocate NavMesh"));
		return nullptr;
	}

	dtStatus status = mesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		fclose(fp);
		dtFreeNavMesh(mesh);
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to initialize NavMesh"));
		return nullptr;
	}

	// Read tiles
	for (int32 i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		readLen = fread(&tileHeader, sizeof(tileHeader), 1, fp);
		if (readLen != 1)
		{
			fclose(fp);
			dtFreeNavMesh(mesh);
			UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to read tile header"));
			return nullptr;
		}

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_TEMP);
		if (!data)
		{
			fclose(fp);
			dtFreeNavMesh(mesh);
			UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to allocate tile data"));
			return nullptr;
		}

		memset(data, 0, tileHeader.dataSize);
		readLen = fread(data, tileHeader.dataSize, 1, fp);
		if (readLen != 1)
		{
			dtFree(data, DT_ALLOC_TEMP);
			fclose(fp);
			dtFreeNavMesh(mesh);
			UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to read tile data"));
			return nullptr;
		}

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	fclose(fp);
	UE_LOG(LogNavMeshSampler, Log, TEXT("Loaded NavMesh from: %s (%d tiles)"), *FilePath, header.numTiles);

	return mesh;
}

bool FNavMeshPointSampler::Initialize(const FString& NavMeshBinPath)
{
	if (!FPaths::FileExists(NavMeshBinPath))
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("NavMesh file not found: %s"), *NavMeshBinPath);
		return false;
	}

	// Load NavMesh using our own loader
	dtNavMesh* LoadedNavMesh = LoadNavMeshFromFile(NavMeshBinPath);
	if (!LoadedNavMesh)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to load NavMesh from: %s"), *NavMeshBinPath);
		return false;
	}

	NavMesh = LoadedNavMesh;
	bOwnsNavMesh = true;

	// Initialize query objects
	NavQuery = dtAllocNavMeshQuery();
	if (!NavQuery)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to allocate NavMeshQuery"));
		return false;
	}

#ifdef USE_DETOUR_BUILT_INTO_UE4
	dtQuerySpecialLinkFilter LinkFilter;
	dtStatus status = NavQuery->init(LoadedNavMesh, 0, &LinkFilter);
#else
	dtStatus status = NavQuery->init(LoadedNavMesh, 0);
#endif

	if (dtStatusFailed(status))
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to initialize NavMeshQuery"));
		return false;
	}

	QueryFilter = new dtQueryFilter();
	if (!QueryFilter)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to allocate QueryFilter"));
		return false;
	}

	CalculateBounds();

	// Find the largest connected region (ground) to filter out isolated surfaces like rooftops
	FindLargestConnectedRegion();

	UE_LOG(LogNavMeshSampler, Log, TEXT("NavMesh loaded successfully. Tiles: %d, Ground polygons: %d"), NavMesh->getMaxTiles(), GroundRegionPolys.Num());
	return true;
}

bool FNavMeshPointSampler::Initialize(const dtNavMesh* InNavMesh)
{
	if (!InNavMesh)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Invalid NavMesh pointer"));
		return false;
	}

	NavMesh = InNavMesh;
	bOwnsNavMesh = false;

	// Initialize query objects
	NavQuery = dtAllocNavMeshQuery();
	if (!NavQuery)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to allocate NavMeshQuery"));
		return false;
	}

#ifdef USE_DETOUR_BUILT_INTO_UE4
	dtQuerySpecialLinkFilter LinkFilter;
	dtStatus status = NavQuery->init(const_cast<dtNavMesh*>(InNavMesh), 0, &LinkFilter);
#else
	dtStatus status = NavQuery->init(const_cast<dtNavMesh*>(InNavMesh), 0);
#endif

	if (dtStatusFailed(status))
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to initialize NavMeshQuery"));
		return false;
	}

	QueryFilter = new dtQueryFilter();
	if (!QueryFilter)
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Failed to allocate QueryFilter"));
		return false;
	}

	CalculateBounds();

	// Find the largest connected region (ground) to filter out isolated surfaces like rooftops
	FindLargestConnectedRegion();

	UE_LOG(LogNavMeshSampler, Log, TEXT("NavMesh initialized from existing pointer. Tiles: %d, Ground polygons: %d"), NavMesh->getMaxTiles(), GroundRegionPolys.Num());
	return true;
}

void FNavMeshPointSampler::CalculateBounds()
{
	CachedBounds = FBox(ForceInit);

	if (!NavMesh)
		return;

	int32 MaxTiles = NavMesh->getMaxTiles();
	for (int32 i = 0; i < MaxTiles; ++i)
	{
		const dtMeshTile* Tile = NavMesh->getTile(i);
		if (!Tile || !Tile->header || !Tile->dataSize)
			continue;

		// Get tile bounds in Recast coordinates using memcpy
		dtReal RecastMin[3], RecastMax[3];
		std::memcpy(RecastMin, Tile->header->bmin, sizeof(RecastMin));
		std::memcpy(RecastMax, Tile->header->bmax, sizeof(RecastMax));

		// Convert to Unreal coordinates
		FVector UnrealMin = RecastToUnreal(RecastMin);
		FVector UnrealMax = RecastToUnreal(RecastMax);

		CachedBounds += UnrealMin;
		CachedBounds += UnrealMax;
	}

	UE_LOG(LogNavMeshSampler, Log, TEXT("NavMesh bounds: %s"), *CachedBounds.ToString());
}

FBox FNavMeshPointSampler::GetNavMeshBounds() const
{
	return CachedBounds;
}

FVector FNavMeshPointSampler::RecastToUnreal(const dtReal RecastPoint[3]) const
{
	// Recast: X, Y, Z -> Unreal: -X, -Z, Y
	return FVector(static_cast<float>(-RecastPoint[0]), static_cast<float>(-RecastPoint[2]), static_cast<float>(RecastPoint[1]));
}

void FNavMeshPointSampler::UnrealToRecast(const FVector& UnrealPoint, dtReal RecastPoint[3]) const
{
	// Unreal: X, Y, Z -> Recast: -X, Z, -Y
	RecastPoint[0] = -UnrealPoint.X;
	RecastPoint[1] = UnrealPoint.Z;
	RecastPoint[2] = -UnrealPoint.Y;
}

bool FNavMeshPointSampler::FindNearestPoly(const dtReal Point[3], const dtReal Extent[3], dtPolyRef& OutPolyRef, dtReal OutNearestPoint[3]) const
{
	if (!NavQuery || !QueryFilter)
		return false;

	dtStatus status = NavQuery->findNearestPoly(Point, Extent, QueryFilter, &OutPolyRef, OutNearestPoint);

	return dtStatusSucceed(status) && OutPolyRef != 0;
}

void FNavMeshPointSampler::FindLargestConnectedRegion()
{
	if (!NavMesh || bGroundRegionComputed)
		return;

	GroundRegionPolys.Empty();

	// Collect all valid polygon references
	TSet<dtPolyRef> AllPolys;
	TMap<dtPolyRef, TSet<dtPolyRef>> AdjacencyMap;

	int32 MaxTiles = NavMesh->getMaxTiles();
	for (int32 i = 0; i < MaxTiles; ++i)
	{
		const dtMeshTile* Tile = NavMesh->getTile(i);
		if (!Tile || !Tile->header || !Tile->dataSize)
			continue;

		dtPolyRef PolyRefBase = NavMesh->getPolyRefBase(Tile);

		for (int32 j = 0; j < Tile->header->polyCount; ++j)
		{
			const dtPoly* Poly = &Tile->polys[j];
			// Skip off-mesh connections (both point and segment types)
			if (Poly->getType() != DT_POLYTYPE_GROUND)
				continue;

			dtPolyRef PolyRef = PolyRefBase | (dtPolyRef)j;
			AllPolys.Add(PolyRef);
			AdjacencyMap.Add(PolyRef, TSet<dtPolyRef>());
		}
	}

	UE_LOG(LogNavMeshSampler, Log, TEXT("Total polygons: %d"), AllPolys.Num());

	// Build adjacency map by traversing links
	for (int32 i = 0; i < MaxTiles; ++i)
	{
		const dtMeshTile* Tile = NavMesh->getTile(i);
		if (!Tile || !Tile->header || !Tile->dataSize)
			continue;

		dtPolyRef PolyRefBase = NavMesh->getPolyRefBase(Tile);

		for (int32 j = 0; j < Tile->header->polyCount; ++j)
		{
			const dtPoly* Poly = &Tile->polys[j];
			// Skip off-mesh connections (both point and segment types)
			if (Poly->getType() != DT_POLYTYPE_GROUND)
				continue;

			dtPolyRef PolyRef = PolyRefBase | (dtPolyRef)j;

			// Traverse links to find neighbors
			for (unsigned int LinkIdx = Poly->firstLink; LinkIdx != DT_NULL_LINK; LinkIdx = Tile->links[LinkIdx].next)
			{
				const dtLink& Link = Tile->links[LinkIdx];
				dtPolyRef NeighborRef = Link.ref;

				if (AllPolys.Contains(NeighborRef))
				{
					AdjacencyMap[PolyRef].Add(NeighborRef);
				}
			}
		}
	}

	// BFS to find all connected regions
	TSet<dtPolyRef> Visited;
	TArray<TArray<dtPolyRef>> Regions;

	for (dtPolyRef StartPoly : AllPolys)
	{
		if (Visited.Contains(StartPoly))
			continue;

		// BFS from this polygon
		TArray<dtPolyRef> Region;
		TArray<dtPolyRef> Queue;
		Queue.Add(StartPoly);
		Visited.Add(StartPoly);

		while (Queue.Num() > 0)
		{
			dtPolyRef CurrentPoly = Queue.Pop(EAllowShrinking::No);
			Region.Add(CurrentPoly);

			// Add all unvisited neighbors
			if (TSet<dtPolyRef>* Neighbors = AdjacencyMap.Find(CurrentPoly))
			{
				for (dtPolyRef Neighbor : *Neighbors)
				{
					if (!Visited.Contains(Neighbor))
					{
						Visited.Add(Neighbor);
						Queue.Add(Neighbor);
					}
				}
			}
		}

		Regions.Add(Region);
	}

	UE_LOG(LogNavMeshSampler, Log, TEXT("Found %d connected regions"), Regions.Num());

	// Find the largest region
	int32 LargestRegionIndex = -1;
	int32 LargestRegionSize = 0;

	for (int32 i = 0; i < Regions.Num(); ++i)
	{
		if (Regions[i].Num() > LargestRegionSize)
		{
			LargestRegionSize = Regions[i].Num();
			LargestRegionIndex = i;
		}

		UE_LOG(LogNavMeshSampler, Log, TEXT("Region %d: %d polygons"), i, Regions[i].Num());
	}

	// Store the largest region as ground
	if (LargestRegionIndex >= 0)
	{
		for (dtPolyRef PolyRef : Regions[LargestRegionIndex])
		{
			GroundRegionPolys.Add(PolyRef);
		}

		UE_LOG(LogNavMeshSampler, Log, TEXT("Largest region (ground): %d polygons (%.1f%% of total)"),
			LargestRegionSize, 100.0f * LargestRegionSize / AllPolys.Num());
	}

	bGroundRegionComputed = true;
}

bool FNavMeshPointSampler::IsInLargestRegion(dtPolyRef PolyRef) const
{
	return GroundRegionPolys.Contains(PolyRef);
}

int32 FNavMeshPointSampler::GetPolysInBox(const dtReal* Center, const dtReal* Extent, dtPolyRef* OutPolys, int32 MaxPolys) const
{
	if (!NavQuery || !QueryFilter)
		return 0;

	int32 PolyCount = 0;
	dtStatus status = NavQuery->queryPolygons(Center, Extent, QueryFilter, OutPolys, &PolyCount, MaxPolys);

	return dtStatusSucceed(status) ? PolyCount : 0;
}

bool FNavMeshPointSampler::IsValidPoint(const FVector& Point, const FVector& Extent) const
{
	if (!NavMesh || !NavQuery || !QueryFilter)
		return false;

	dtReal RecastPoint[3], RecastExtent[3];
	UnrealToRecast(Point, RecastPoint);
	UnrealToRecast(Extent.GetAbs(), RecastExtent);

	dtPolyRef PolyRef;
	dtReal NearestPoint[3];

	dtStatus status = NavQuery->findNearestPoly(RecastPoint, RecastExtent, QueryFilter, &PolyRef, NearestPoint);

	return dtStatusSucceed(status) && PolyRef != 0;
}

bool FNavMeshPointSampler::SampleGridPoints(const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult)
{
	if (!IsValid())
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Sampler not initialized"));
		return false;
	}

	return SampleGridPointsInRegion(CachedBounds, Config, OutResult);
}

bool FNavMeshPointSampler::SampleGridPointsInRegion(const FBox& Region, const FNavMeshSamplerConfig& Config, FNavMeshSamplingResult& OutResult)
{
	OutResult.Reset();

	if (!IsValid())
	{
		UE_LOG(LogNavMeshSampler, Error, TEXT("Sampler not initialized"));
		return false;
	}

	float GridSpacing = FMath::Max(Config.GridSpacing, 1.0f);

	// Get region bounds
	FVector RegionMin = Region.Min;
	FVector RegionMax = Region.Max;

	// Calculate grid dimensions
	int32 GridSizeX = FMath::CeilToInt((RegionMax.X - RegionMin.X) / GridSpacing) + 1;
	int32 GridSizeY = FMath::CeilToInt((RegionMax.Y - RegionMin.Y) / GridSpacing) + 1;

	UE_LOG(LogNavMeshSampler, Log, TEXT("Sampling grid: %d x %d = %d points"), GridSizeX, GridSizeY, GridSizeX * GridSizeY);

	// Use a larger extent for validation - at least half the grid spacing in X/Y
	float ExtentXY = FMath::Max(Config.ValidationExtent.X, GridSpacing * 0.6f);

	// Use the FULL NavMesh height range for ExtentZ - connectivity filtering will handle isolated surfaces
	// This allows the query to find polygons at any height, and we filter by connectivity afterward
	float ExtentZ = FMath::Abs(CachedBounds.Max.Z - CachedBounds.Min.Z) + 100.0f;  // Full height + margin

	// Convert to Recast coordinates for the extent
	// Extent in Recast: X, Z, -Y -> ExtentXY, ExtentZ, ExtentXY
	dtReal RecastExtent[3];
	RecastExtent[0] = ExtentXY;   // X extent (Recast X = -Unreal X)
	RecastExtent[1] = ExtentZ;    // Y extent in Recast = Z extent in Unreal
	RecastExtent[2] = ExtentXY;   // Z extent in Recast = Y extent in Unreal

	UE_LOG(LogNavMeshSampler, Log, TEXT("Using extent: X=%.1f, Y=%.1f, Z=%.1f (Recast coords) - Full height range with connectivity filtering"),
		RecastExtent[0], RecastExtent[1], RecastExtent[2]);

	int32 ValidCount = 0;
	int32 TotalCount = 0;
	int32 FilteredOutByConnectivity = 0;

	// Use the MIDDLE Z as the query height for each grid cell
	// Since we're using full height extent, the query will find polygons at any height
	// Connectivity filtering will then determine if it's ground or an isolated surface
	float QueryZ = (RegionMin.Z + RegionMax.Z) * 0.5f;

	// Iterate through grid
	for (int32 GridY = 0; GridY < GridSizeY; ++GridY)
	{
		for (int32 GridX = 0; GridX < GridSizeX; ++GridX)
		{
			// Calculate Unreal world position
			FVector UnrealPoint(
				RegionMin.X + GridX * GridSpacing,
				RegionMin.Y + GridY * GridSpacing,
				QueryZ
			);

			// Convert to Recast coordinates
			dtReal RecastPoint[3];
			UnrealToRecast(UnrealPoint, RecastPoint);

			// Find nearest polygon using 3D distance (not 2D projection)
			// This ensures we get the actually closest polygon in 3D space,
			// which helps filter out points on rooftops/car tops that are closer in 2D but farther in 3D
			dtPolyRef PolyRef = 0;
			dtReal NearestPoint[3] = {0, 0, 0};

			dtStatus status = NavQuery->findNearestPoly(RecastPoint, RecastExtent, QueryFilter, &PolyRef, NearestPoint);

			TotalCount++;

			if (dtStatusSucceed(status) && PolyRef != 0)
			{
				// CONNECTIVITY FILTERING:
				// Check if the polygon belongs to the largest connected region (ground)
				// This filters out isolated surfaces like rooftops, car tops, etc.
				if (!IsInLargestRegion(PolyRef))
				{
					FilteredOutByConnectivity++;
					continue;  // Skip this point - it's on an isolated surface
				}

				// Convert result back to Unreal coordinates
				FVector ResultPoint = RecastToUnreal(NearestPoint);

				// Check if the closest point is within the validation extent (similar to UE4RecastHelper)
				FVector Delta = ResultPoint - UnrealPoint;
				Delta = Delta.GetAbs();

				if (Delta.X <= ExtentXY && Delta.Y <= ExtentXY && Delta.Z <= ExtentZ)
				{
					FNavMeshSamplePoint SamplePoint(ResultPoint, true, -1);
					OutResult.SamplePoints.Add(SamplePoint);
					ValidCount++;
				}
			}
		}
	}

	OutResult.TotalPoints = TotalCount;
	OutResult.ValidPoints = ValidCount;
	OutResult.NavMeshBounds = CachedBounds;

	// Calculate approximate area (based on grid spacing)
	OutResult.TotalArea = ValidCount * GridSpacing * GridSpacing;

	UE_LOG(LogNavMeshSampler, Log, TEXT("Sampling complete. Total: %d, Valid: %d, Filtered by connectivity: %d, Area: %.2f cm^2"),
		TotalCount, ValidCount, FilteredOutByConnectivity, OutResult.TotalArea);

	return true;
}