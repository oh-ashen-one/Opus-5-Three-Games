// Copyright Opus 5 Three Games. Original work.

using UnrealBuildTool;

public class Stormfall : ModuleRules
{
	public Stormfall(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// So sources in subdirectories (Tests/, etc.) can include module headers
		// by plain name rather than by relative path.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
		});
	}
}
