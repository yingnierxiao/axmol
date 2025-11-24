/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/
 ****************************************************************************/

#pragma once

#include <string>
#include <vector>
#include <variant>
#include <cstdint>

namespace ax {
namespace scripting {
namespace dotnet {

/**
 * @brief Runtime type enumeration
 */
enum class RuntimeType
{
    CoreCLR,  // Desktop platforms: Windows, Linux, macOS
    Mono      // Mobile platforms: iOS, Android
};

/**
 * @brief Configuration for script engine initialization
 */
struct ScriptEngineConfig
{
    // For CoreCLR: path to runtimeconfig.json
    // For Mono: path to main assembly
    std::string configPath;

    // Additional search paths for assemblies
    std::vector<std::string> assemblySearchPaths;

    // Enable debugging support
    bool enableDebugging = false;

    // Custom properties (platform-specific)
    std::vector<std::pair<std::string, std::string>> properties;
};

/**
 * @brief Handle to a managed object (opaque pointer)
 */
using ScriptObjectHandle = void*;

/**
 * @brief Value types that can be passed between C++ and C#
 */
using ScriptValue = std::variant<
    std::monostate,    // void/null
    bool,
    int32_t,
    int64_t,
    float,
    double,
    std::string,
    ScriptObjectHandle // Managed object reference
>;

/**
 * @brief Arguments for method calls
 */
struct ScriptArgs
{
    std::vector<ScriptValue> values;

    ScriptArgs() = default;
    ScriptArgs(std::initializer_list<ScriptValue> init) : values(init) {}
};

/**
 * @brief Platform detection macros
 */
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    #if defined(__APPLE__)
        #include <TargetConditionals.h>
        #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
            #define AX_DOTNET_USE_MONO 1
        #else
            #define AX_DOTNET_USE_CORECLR 1
        #endif
    #elif defined(__ANDROID__)
        #define AX_DOTNET_USE_MONO 1
    #else
        // Desktop: Windows, Linux, macOS
        #define AX_DOTNET_USE_CORECLR 1
    #endif
#else
    #error "Unsupported platform for .NET scripting"
#endif

} // namespace dotnet
} // namespace scripting
} // namespace ax
