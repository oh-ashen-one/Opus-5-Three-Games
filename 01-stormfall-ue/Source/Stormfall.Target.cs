// Copyright Opus 5 Three Games. Original work.

using UnrealBuildTool;

public class StormfallTarget : TargetRules
{
	public StormfallTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Stormfall");
	}
}
