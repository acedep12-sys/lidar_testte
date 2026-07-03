// SquadAI.Build.cs — Complete module dependencies for the merged system
using UnrealBuildTool;

public class SquadAI : ModuleRules
{
	public SquadAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// ---- Runtime dependencies ----
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",

			// AI
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",

			// StateTree
			"StateTreeModule",
			"GameplayStateTreeModule",

			// Performance
			"SignificanceManager",

			// Smart Objects
			"SmartObjectsModule",

			// Gameplay tags / events / ability helper library
			"GameplayTags",
			"GameplayAbilities",

			// Settings
			"DeveloperSettings",

			// UMG (for HUD)
			"UMG",
			"Slate",
			"SlateCore"
		});

		// Keep private deps minimal for easier recompiles and fewer module-version issues on UE 5.7.x.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// ---- Editor-only dependencies ----
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"AssetRegistry",
				"ToolMenus",
			});
		}
	}
}
