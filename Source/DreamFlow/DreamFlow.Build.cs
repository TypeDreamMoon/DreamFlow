using UnrealBuildTool;

public class DreamFlow : ModuleRules
{
    public DreamFlow(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "DeveloperSettings",
                "Engine",
                "GameplayTags",
            });
    }
}
