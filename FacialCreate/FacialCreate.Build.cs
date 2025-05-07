// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class FacialCreate : ModuleRules
{
    public FacialCreate(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core" ,"DesktopPlatform", "Json", "JsonUtilities",
                // ... add other public dependencies that you statically link with here ...
            }
            );
            
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "EditorFramework",
                "UnrealEd",
                "ToolMenus",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                // ... add private dependencies that you statically link with here ...    
            }
            );
        
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );

        string DNASDKPath = "D:/DnaCalibTestTool/DnaCalibTestTool/Source/DnaCalibTestTool/DNACalib";
        PublicIncludePaths.Add(Path.Combine(DNASDKPath, "include"));
        PublicIncludePaths.Add(Path.Combine(DNASDKPath, "src"));

        string FBXSDKPath = "D:/tools/2020.3.7";
        PublicIncludePaths.Add(Path.Combine(FBXSDKPath, "include"));
        PublicAdditionalLibraries.Add(Path.Combine(FBXSDKPath, "lib/x64/release/libfbxsdk.lib"));
    }
}
