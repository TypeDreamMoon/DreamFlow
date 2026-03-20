using UnrealBuildTool;

public class DreamFlowEditor : ModuleRules
{
    public DreamFlowEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "DreamFlow",
                "DreamFlowDialogue",
                "DreamFlowQuest",
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "ApplicationCore",
                "AssetTools",
                "ClassViewer",
                "CoreUObject",
                "EditorFramework",
                "Engine",
                "GraphEditor",
                "InputCore",
                "KismetWidgets",
                "PropertyEditor",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "UnrealEd",
            });
    }
}
