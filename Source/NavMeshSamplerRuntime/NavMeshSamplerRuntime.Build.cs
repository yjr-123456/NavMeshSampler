// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class NavMeshSamplerRuntime : ModuleRules
{
	public NavMeshSamplerRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDirectory, "Source/Runtime/Launch")
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities",
				"NavigationSystem",
				"Navmesh"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);

		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

		PublicDefinitions.AddRange(
			new string[]
			{
				"USE_DETOUR_BUILT_INTO_UE4",
			}
		);
	}
}