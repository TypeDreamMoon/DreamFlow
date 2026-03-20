using UnrealBuildTool;

public class DreamFlowQuest : ModuleRules
{
    public DreamFlowQuest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "DreamFlow",
                "Engine",
            });
    }
}
