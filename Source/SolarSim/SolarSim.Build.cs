// =============================================================================
//  SolarSim.Build.cs
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

using UnrealBuildTool;

public class SolarSim : ModuleRules
{
    public SolarSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Niagara",                  // Niagara particle system
            "NiagaraCore",
            "UMG",                      // UMG widgets
            "Slate",
            "SlateCore",
            "RenderCore",
            "RHI",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "ApplicationCore",
        });

        // Enable C++17 features
        CppStandard = CppStandardVersion.Cpp20;

        // Allow fast math (slight precision tradeoff, ~20% perf gain in optimizer loops)
        bUseUnity = false;

        // Optimization: full optimization in shipping
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}
