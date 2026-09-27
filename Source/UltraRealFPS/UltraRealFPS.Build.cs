using UnrealBuildTool;

public class UltraRealFPS : ModuleRules
{
    public UltraRealFPS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "PhysicsCore"
        });
    }
}
