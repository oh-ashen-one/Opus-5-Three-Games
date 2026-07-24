// Copyright Opus 5 Three Games. Original work.

using UnrealBuildTool;

public class StormfallEditorTarget : TargetRules
{
	public StormfallEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Stormfall");
	}
}
