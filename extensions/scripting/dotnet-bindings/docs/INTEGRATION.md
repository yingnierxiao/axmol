# Axmol .NET 集成完整指南

## 概述

本文档提供完整的Axmol .NET脚本绑定集成步骤，从零开始到运行第一个C#脚本。

---

## 📋 实施路线图

```
Phase 1: 基础框架 (1-2周)
  └─ [✅] 目录结构创建
  └─ [✅] 抽象接口层设计
  └─ [✅] CMake配置文件
  └─ [🔲] CoreCLR实现 (桌面)
  └─ [🔲] 测试基本调用

Phase 2: 跨平台扩展 (2-3周)
  └─ [🔲] Mono实现 (移动)
  └─ [🔲] iOS AOT支持
  └─ [🔲] Android集成
  └─ [🔲] 全平台测试

Phase 3: 完善生态 (1-2周)
  └─ [🔲] 引擎API绑定
  └─ [🔲] 示例项目
  └─ [🔲] 文档完善
  └─ [🔲] 性能优化
```

---

## 第一步：启用.NET扩展

### 1.1 修改 `core/CMakeLists.txt`

在扩展选项部分添加：

```cmake
# 在现有的 AX_ENABLE_EXT_LUA 等选项后面添加
option(AX_ENABLE_EXT_DOTNET "Build .NET scripting support" OFF)
```

### 1.2 修改 `extensions/CMakeLists.txt`

在文件末尾添加：

```cmake
# .NET Scripting Extension
if(AX_ENABLE_EXT_DOTNET)
  message(STATUS "Adding .NET scripting extension")
  add_subdirectory(scripting/dotnet-bindings)
endif()
```

### 1.3 验证配置

```bash
cd /path/to/axmol
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
```

应该看到：
```
-- Adding .NET scripting extension
-- .NET Scripting: Using CoreCLR for desktop platform
```

---

## 第二步：实现CoreCLR引擎（桌面平台）

### 2.1 创建实现文件

创建 `extensions/scripting/dotnet-bindings/coreclr/CoreCLREngine.cpp`:

```cpp
#include "CoreCLREngine.h"
#include "axmol.h"
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_EXPORT(lib, name) GetProcAddress((HMODULE)lib, name)
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY | RTLD_LOCAL)
    #define GET_EXPORT(lib, name) dlsym(lib, name)
#endif

namespace ax {
namespace scripting {
namespace dotnet {

CoreCLREngine::CoreCLREngine()
{
    AXLOG("[CoreCLR] Engine created");
}

CoreCLREngine::~CoreCLREngine()
{
    shutdown();
}

bool CoreCLREngine::initialize(const ScriptEngineConfig& config)
{
    if (_initialized) {
        AXLOG("[CoreCLR] Already initialized");
        return true;
    }

    AXLOG("[CoreCLR] Initializing with config: %s", config.configPath.c_str());

    // Step 1: Load hostfxr
    if (!loadHostFxr()) {
        AXLOGERROR("[CoreCLR] Failed to load hostfxr");
        return false;
    }

    // Step 2: Initialize runtime
    if (!initializeRuntime(config.configPath)) {
        AXLOGERROR("[CoreCLR] Failed to initialize runtime");
        return false;
    }

    // Step 3: Get function pointers
    if (!loadFunctionPointers()) {
        AXLOGERROR("[CoreCLR] Failed to load function pointers");
        return false;
    }

    _initialized = true;
    AXLOG("[CoreCLR] Initialization complete");
    return true;
}

bool CoreCLREngine::loadHostFxr()
{
    char_t buffer[MAX_PATH];
    size_t buffer_size = sizeof(buffer) / sizeof(char_t);

    int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if (rc != 0) {
        AXLOGERROR("[CoreCLR] Failed to find hostfxr. Error code: %d", rc);
        return false;
    }

    AXLOG("[CoreCLR] Found hostfxr at: %s", buffer);

    // Load hostfxr library
    _hostfxrLib = LOAD_LIBRARY(buffer);
    if (!_hostfxrLib) {
        AXLOGERROR("[CoreCLR] Failed to load hostfxr library");
        return false;
    }

    // Load function pointers
    _initFxr = (hostfxr_initialize_for_runtime_config_fn)
        GET_EXPORT(_hostfxrLib, "hostfxr_initialize_for_runtime_config");
    _getRuntimeDelegate = (hostfxr_get_runtime_delegate_fn)
        GET_EXPORT(_hostfxrLib, "hostfxr_get_runtime_delegate");
    _closeFxr = (hostfxr_close_fn)
        GET_EXPORT(_hostfxrLib, "hostfxr_close");

    if (!_initFxr || !_getRuntimeDelegate || !_closeFxr) {
        AXLOGERROR("[CoreCLR] Failed to load hostfxr functions");
        return false;
    }

    return true;
}

bool CoreCLREngine::initializeRuntime(const std::string& runtimeConfigPath)
{
    int rc = _initFxr(runtimeConfigPath.c_str(), nullptr, &_hostContext);
    if (rc != 0 || !_hostContext) {
        AXLOGERROR("[CoreCLR] Failed to init runtime. Error code: %d", rc);
        return false;
    }

    AXLOG("[CoreCLR] Runtime initialized");
    return true;
}

bool CoreCLREngine::loadFunctionPointers()
{
    int rc = _getRuntimeDelegate(
        _hostContext,
        hdt_load_assembly_and_get_function_pointer,
        (void**)&_loadAssemblyAndGetFunctionPointer
    );

    if (rc != 0 || !_loadAssemblyAndGetFunctionPointer) {
        AXLOGERROR("[CoreCLR] Failed to get function pointer delegate");
        return false;
    }

    return true;
}

// ... 其他方法实现 ...

void CoreCLREngine::shutdown()
{
    if (!_initialized) return;

    AXLOG("[CoreCLR] Shutting down");

    if (_hostContext && _closeFxr) {
        _closeFxr(_hostContext);
        _hostContext = nullptr;
    }

    _initialized = false;
}

} // namespace dotnet
} // namespace scripting
} // namespace ax
```

### 2.2 实现工厂方法

创建 `extensions/scripting/dotnet-bindings/common/ScriptEngine.cpp`:

```cpp
#include "ScriptEngine.h"

#ifdef AX_DOTNET_USE_CORECLR
    #include "../coreclr/CoreCLREngine.h"
#elif defined(AX_DOTNET_USE_MONO)
    #include "../mono/MonoEngine.h"
#endif

namespace ax {
namespace scripting {
namespace dotnet {

std::unique_ptr<ScriptEngine> ScriptEngine::create()
{
#ifdef AX_DOTNET_USE_CORECLR
    return std::make_unique<CoreCLREngine>();
#elif defined(AX_DOTNET_USE_MONO)
    return std::make_unique<MonoEngine>();
#else
    #error "No .NET runtime implementation selected"
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
    if (!_engine) {
        _engine = ScriptEngine::create();
    }
    return _engine->initialize(config);
}

void ScriptEngineManager::shutdown()
{
    if (_engine) {
        _engine->shutdown();
        _engine.reset();
    }
}

} // namespace dotnet
} // namespace scripting
} // namespace ax
```

---

## 第三步：创建C#示例项目

### 3.1 创建项目文件

```bash
cd extensions/scripting/dotnet-bindings/managed
cp Axmol.Scripting.csproj.template Axmol.Scripting.csproj
cp EngineAPI.cs.template EngineAPI.cs
```

### 3.2 构建C#项目

```bash
cd managed
dotnet build -c Release
```

这将生成：
- `Axmol.Scripting.dll`
- `Axmol.Scripting.runtimeconfig.json`

---

## 第四步：在游戏中使用

### 4.1 初始化脚本引擎

在你的游戏初始化代码中（如 `AppDelegate.cpp`）:

```cpp
#include "extensions/scripting/dotnet-bindings/common/ScriptEngine.h"

bool AppDelegate::applicationDidFinishLaunching()
{
    // ... 现有初始化代码 ...

    // 初始化.NET脚本引擎
    auto& scriptMgr = ax::scripting::dotnet::ScriptEngineManager::getInstance();

    ax::scripting::dotnet::ScriptEngineConfig config;
    config.configPath = ax::FileUtils::getInstance()->fullPathForFilename(
        "Axmol.Scripting.runtimeconfig.json"
    );

    if (!scriptMgr.initialize(config)) {
        AXLOGERROR("Failed to initialize .NET scripting");
        return false;
    }

    // 调用C#初始化方法
    auto* engine = scriptMgr.getEngine();
    engine->executeMethod("Axmol.Scripting.EngineAPI", "Initialize");

    return true;
}
```

### 4.2 每帧更新

```cpp
void AppDelegate::applicationDidEnterBackground()
{
    auto* engine = ax::scripting::dotnet::ScriptEngineManager::getInstance().getEngine();
    if (engine && engine->isInitialized()) {
        // 通知C#进入后台
        engine->executeMethod("Axmol.Scripting.EngineAPI", "OnPause");
    }
}
```

---

## 第五步：测试验证

### 5.1 编译项目

```bash
cmake --build build --config Release
```

### 5.2 运行测试

运行游戏后，应该在控制台看到：

```
[CoreCLR] Initializing...
[CoreCLR] Runtime initialized
[Axmol.NET] Scripting system initialized
[Axmol.NET] Runtime: .NET 8.0.0
[Game] Initializing game logic
```

---

## 第六步：添加更多API绑定

### 6.1 导出C++函数给C#

在C++中创建导出函数：

```cpp
// 在某个cpp文件中
extern "C" {
    AX_DLL void AXMOL_CALL axmol_log(const char* message) {
        ax::log("%s", message);
    }

    AX_DLL void AXMOL_CALL axmol_create_sprite(const char* filename) {
        // 创建精灵的实现
    }
}
```

### 6.2 在C#中声明

```csharp
public static class NativeAPI
{
    [DllImport("axmol", EntryPoint = "axmol_log")]
    public static extern void Log(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string message
    );

    [DllImport("axmol", EntryPoint = "axmol_create_sprite")]
    public static extern void CreateSprite(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string filename
    );
}
```

---

## 故障排除

### 问题1：找不到nethost.lib

**解决方案：**
```bash
# 设置环境变量
export DOTNET_ROOT=/path/to/dotnet

# 或在CMake中指定
cmake -B build -DDOTNET_ROOT=/path/to/dotnet
```

### 问题2：runtimeconfig.json找不到

**解决方案：**
确保将C#项目的输出复制到游戏的资源目录：

```bash
cp managed/bin/Release/net8.0/* /path/to/game/Resources/
```

### 问题3：调用C#方法失败

**解决方案：**
检查方法签名是否正确，必须使用 `[UnmanagedCallersOnly]`：

```csharp
// ✅ 正确
[UnmanagedCallersOnly(EntryPoint = "my_method")]
public static int MyMethod(IntPtr args, int size) { ... }

// ❌ 错误 - 缺少 EntryPoint
[UnmanagedCallersOnly]
public static void MyMethod() { ... }
```

---

## 下一步

1. ✅ 实现完整的CoreCLR引擎
2. ⏭️ 实现Mono引擎（移动平台）
3. ⏭️ 绑定更多Axmol API
4. ⏭️ 创建完整的游戏示例

---

## 参考资源

- [.NET Hosting Tutorial](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
- [Axmol Wiki](https://github.com/axmolengine/axmol/wiki)
- [PLATFORM_NOTES.md](PLATFORM_NOTES.md) - 平台特定说明
