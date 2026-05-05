// ----------------------------------------------------------------------------------
// Copyright (c) 2026 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Freelance:  https://www.upwork.com/freelancers/~01f46dab6bbf4fe99e?mp_source=share
// ----------------------------------------------------------------------------------

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
