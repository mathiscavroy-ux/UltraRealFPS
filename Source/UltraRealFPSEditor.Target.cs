using UnrealBuildTool;
using System.Collections.Generic;

public class UltraRealFPSEditorTarget : TargetRules
{
    public UltraRealFPSEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("UltraRealFPS");
    }
}
