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

		if (File.Exists(Path.Combine(EngineDirectory, "Source", "Runtime", "AES", "AES.Build.cs")))
		{
			PrivateDependencyModuleNames.Add("AES");
		}
	}
}
