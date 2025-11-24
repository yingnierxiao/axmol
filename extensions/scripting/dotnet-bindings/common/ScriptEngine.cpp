/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 Common ScriptEngine implementation
 ****************************************************************************/

#include "ScriptEngine.h"
#include "axmol.h"

// Include platform-specific implementations
#ifdef AX_DOTNET_USE_CORECLR
    #include "../coreclr/CoreCLREngine.h"
#elif defined(AX_DOTNET_USE_MONO)
    #include "../mono/MonoEngine.h"
#endif

namespace ax {
namespace scripting {
namespace dotnet {

// Factory method implementation
std::unique_ptr<ScriptEngine> ScriptEngine::create()
{
#ifdef AX_DOTNET_USE_CORECLR
    AXLOG("[.NET] Creating CoreCLR engine for desktop platform");
    return std::make_unique<CoreCLREngine>();
#elif defined(AX_DOTNET_USE_MONO)
    AXLOG("[.NET] Creating Mono engine for mobile platform");
    return std::make_unique<MonoEngine>();
#else
    #error "No .NET runtime implementation selected"
    return nullptr;
#endif
}

// ScriptEngineManager implementation
ScriptEngineManager& ScriptEngineManager::getInstance()
{
    static ScriptEngineManager instance;
    return instance;
}

bool ScriptEngineManager::initialize(const ScriptEngineConfig& config)
{
    if (_engine && _engine->isInitialized())
    {
        AXLOG("[.NET] Engine already initialized");
        return true;
    }

    AXLOG("[.NET] Initializing script engine manager");

    // Create platform-specific engine
    if (!_engine)
    {
        _engine = ScriptEngine::create();
        if (!_engine)
        {
            AXLOGERROR("[.NET] Failed to create script engine");
            return false;
        }
    }

    // Initialize the engine
    if (!_engine->initialize(config))
    {
        AXLOGERROR("[.NET] Failed to initialize script engine");
        _engine.reset();
        return false;
    }

    AXLOG("[.NET] Script engine manager initialized successfully");
    AXLOG("[.NET] Runtime type: %s",
          _engine->getRuntimeType() == RuntimeType::CoreCLR ? "CoreCLR" : "Mono");

    return true;
}

void ScriptEngineManager::shutdown()
{
    if (_engine)
    {
        AXLOG("[.NET] Shutting down script engine manager");
        _engine->shutdown();
        _engine.reset();
        AXLOG("[.NET] Script engine manager shutdown complete");
    }
}

} // namespace dotnet
} // namespace scripting
} // namespace ax
