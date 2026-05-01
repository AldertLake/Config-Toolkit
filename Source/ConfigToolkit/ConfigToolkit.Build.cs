using UnrealBuildTool;
using System.IO;

public class ConfigToolkit : ModuleRules
{
	public ConfigToolkit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings"
			}
		);

		// FAES is provided by Core in stock UE 5.4+. If a custom engine exposes
		// a standalone AES module, consume it without breaking stock engines.
		if (File.Exists(Path.Combine(EngineDirectory, "Source", "Runtime", "AES", "AES.Build.cs")))
		{
			PrivateDependencyModuleNames.Add("AES");
		}
	}
}
