using UnrealBuildTool;

public class DreamFlowDialogue : ModuleRules
{
    public DreamFlowDialogue(ReadOnlyTargetRules Target) : base(Target)
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
