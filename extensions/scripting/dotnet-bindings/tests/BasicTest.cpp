/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 Basic test for .NET scripting integration
 ****************************************************************************/

#include "../common/ScriptEngine.h"
#include "axmol.h"

using namespace ax;
using namespace ax::scripting::dotnet;

/**
 * @brief Basic smoke test for .NET scripting
 *
 * This test verifies:
 * 1. Engine initialization
 * 2. Assembly loading
 * 3. Static method execution
 * 4. Shutdown
 */
class DotNetBasicTest
{
public:
    static bool runTest()
    {
        AXLOG("=== Starting .NET Basic Test ===");

        // Step 1: Initialize engine
        AXLOG("[Test] Step 1: Initialize engine");
        auto& manager = ScriptEngineManager::getInstance();

        ScriptEngineConfig config;
        // Note: Update this path to match your project structure
        config.configPath = FileUtils::getInstance()->fullPathForFilename(
            "Axmol.Scripting.runtimeconfig.json"
        );

        AXLOG("[Test] Config path: %s", config.configPath.c_str());

        if (!manager.initialize(config))
        {
            AXLOGERROR("[Test] FAILED: Engine initialization failed");
            return false;
        }
        AXLOG("[Test] PASSED: Engine initialized");

        // Step 2: Get engine instance
        AXLOG("[Test] Step 2: Get engine instance");
        auto* engine = manager.getEngine();
        if (!engine || !engine->isInitialized())
        {
            AXLOGERROR("[Test] FAILED: Engine not available");
            return false;
        }
        AXLOG("[Test] PASSED: Engine instance obtained");
        AXLOG("[Test] Runtime type: %s",
              engine->getRuntimeType() == RuntimeType::CoreCLR ? "CoreCLR" : "Mono");

        // Step 3: Load assembly
        AXLOG("[Test] Step 3: Load assembly");
        std::string assemblyPath = FileUtils::getInstance()->fullPathForFilename(
            "Axmol.Scripting.dll"
        );
        AXLOG("[Test] Assembly path: %s", assemblyPath.c_str());

        if (!engine->loadAssembly(assemblyPath))
        {
            AXLOGERROR("[Test] FAILED: Assembly loading failed");
            return false;
        }
        AXLOG("[Test] PASSED: Assembly loaded");

        // Step 4: Execute static method
        AXLOG("[Test] Step 4: Execute static method");
        ScriptValue result = engine->executeMethod(
            "Axmol.Scripting.EngineAPI",
            "axmol_dotnet_initialize"
        );

        // Check result (expected: 0 for success)
        if (std::holds_alternative<int32_t>(result))
        {
            int32_t value = std::get<int32_t>(result);
            AXLOG("[Test] Method returned: %d", value);
            if (value == 0)
            {
                AXLOG("[Test] PASSED: Method executed successfully");
            }
            else
            {
                AXLOGERROR("[Test] FAILED: Method returned error code");
                return false;
            }
        }
        else
        {
            AXLOG("[Test] WARNING: Method returned non-integer value");
        }

        // Step 5: Shutdown
        AXLOG("[Test] Step 5: Shutdown");
        manager.shutdown();
        AXLOG("[Test] PASSED: Shutdown complete");

        AXLOG("=== .NET Basic Test COMPLETED SUCCESSFULLY ===");
        return true;
    }
};

// Example usage in AppDelegate or main function:
/*
bool AppDelegate::applicationDidFinishLaunching()
{
    // ... existing initialization ...

    // Run .NET test
    if (!DotNetBasicTest::runTest())
    {
        AXLOGERROR("DotNet test failed!");
        return false;
    }

    return true;
}
*/
