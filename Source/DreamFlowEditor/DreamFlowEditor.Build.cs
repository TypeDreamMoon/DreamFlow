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
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "ApplicationCore",
                "AssetRegistry",
                "AssetTools",
                "ClassViewer",
                "ContentBrowser",
                "ContentBrowserData",
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
