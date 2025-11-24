# .NET 脚本快速开始指南

## 5分钟快速体验

### 前置要求
- ✅ .NET SDK 8.0+ 已安装
- ✅ Axmol 引擎已构建
- ✅ Windows/Linux/macOS 系统

### 步骤 1: 启用.NET扩展 (2分钟)

**修改 `core/CMakeLists.txt`**，在扩展选项部分添加：

```cmake
# 在 AX_ENABLE_EXT_LUA 等选项后面添加
option(AX_ENABLE_EXT_DOTNET "Build .NET scripting support" ON)
```

**修改 `extensions/CMakeLists.txt`**，在文件末尾添加：

```cmake
# .NET Scripting Extension
if(AX_ENABLE_EXT_DOTNET)
  add_subdirectory(scripting/dotnet-bindings)
endif()
```

### 步骤 2: 构建项目 (2分钟)

```bash
# 配置CMake（启用.NET）
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release
```

### 步骤 3: 创建C#项目 (1分钟)

```bash
cd extensions/scripting/dotnet-bindings/managed

# 复制模板文件
cp Axmol.Scripting.csproj.template Axmol.Scripting.csproj
cp EngineAPI.cs.template EngineAPI.cs

# 构建C#程序集
dotnet build -c Release
```

输出文件在：`bin/Release/net8.0/`
- `Axmol.Scripting.dll`
- `Axmol.Scripting.runtimeconfig.json`

### 步骤 4: 运行测试 (1分钟)

**在你的游戏代码中（如 `AppDelegate.cpp`）：**

```cpp
#include "extensions/scripting/dotnet-bindings/common/ScriptEngine.h"

bool AppDelegate::applicationDidFinishLaunching()
{
    // 初始化.NET引擎
    auto& manager = ax::scripting::dotnet::ScriptEngineManager::getInstance();

    ax::scripting::dotnet::ScriptEngineConfig config;
    config.configPath = ax::FileUtils::getInstance()->fullPathForFilename(
        "Axmol.Scripting.runtimeconfig.json"
    );

    if (manager.initialize(config))
    {
        auto* engine = manager.getEngine();

        // 加载程序集
        engine->loadAssembly(
            ax::FileUtils::getInstance()->fullPathForFilename("Axmol.Scripting.dll")
        );

        // 调用C#方法
        engine->executeMethod("Axmol.Scripting.EngineAPI", "axmol_dotnet_initialize");

        AXLOG("✅ .NET Scripting Initialized!");
    }

    return true;
}
```

**复制C#输出到Resources：**

```bash
# 假设你的游戏在 projects/MyGame
cp managed/bin/Release/net8.0/*.dll projects/MyGame/Resources/
cp managed/bin/Release/net8.0/*.json projects/MyGame/Resources/
```

**运行游戏：**

```bash
./build/bin/Release/MyGame
```

### 预期输出

```
[.NET] Creating CoreCLR engine for desktop platform
[CoreCLR] Engine instance created
[CoreCLR] Initializing...
[CoreCLR] Found hostfxr at: /path/to/hostfxr
[CoreCLR] Runtime initialized
[Axmol.NET] Scripting system initialized
[Axmol.NET] Runtime: .NET 8.0.0
[Game] Initializing game logic
✅ .NET Scripting Initialized!
```

---

## 完整示例

### C# 代码 (`EngineAPI.cs`)

```csharp
using System;
using System.Runtime.InteropServices;

namespace Axmol.Scripting
{
    public static class EngineAPI
    {
        // 初始化入口
        [UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_initialize")]
        public static int Initialize(IntPtr args, int size)
        {
            Console.WriteLine("[C#] .NET Scripting Initialized!");
            Console.WriteLine($"[C#] Runtime: {RuntimeInformation.FrameworkDescription}");
            return 0; // Success
        }

        // 每帧更新
        [UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_update")]
        public static void Update(float deltaTime)
        {
            // 游戏逻辑更新
        }

        // 关闭
        [UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_shutdown")]
        public static void Shutdown()
        {
            Console.WriteLine("[C#] Shutting down");
        }
    }
}
```

### C++ 代码 (`AppDelegate.cpp`)

```cpp
#include "extensions/scripting/dotnet-bindings/common/ScriptEngine.h"

using namespace ax::scripting::dotnet;

class AppDelegate : public ax::Application
{
public:
    bool applicationDidFinishLaunching() override
    {
        // 初始化.NET
        initDotNet();

        // 继续游戏初始化...
        return true;
    }

    void applicationWillEnterForeground() override
    {
        // 恢复更新
    }

private:
    void initDotNet()
    {
        auto& manager = ScriptEngineManager::getInstance();

        ScriptEngineConfig config;
        config.configPath = FileUtils::getInstance()->fullPathForFilename(
            "Axmol.Scripting.runtimeconfig.json"
        );

        if (!manager.initialize(config))
        {
            AXLOGERROR("Failed to initialize .NET");
            return;
        }

        auto* engine = manager.getEngine();

        // 加载程序集
        std::string dllPath = FileUtils::getInstance()->fullPathForFilename(
            "Axmol.Scripting.dll"
        );
        engine->loadAssembly(dllPath);

        // 调用初始化
        engine->executeMethod("Axmol.Scripting.EngineAPI", "axmol_dotnet_initialize");
    }
};
```

---

## 常见问题

### Q: 编译错误 "nethost.lib not found"

**A:** 设置环境变量：

Windows:
```cmd
set DOTNET_ROOT=C:\Program Files\dotnet
```

Linux/macOS:
```bash
export DOTNET_ROOT=/usr/share/dotnet
```

### Q: 运行时错误 "hostfxr.dll not found"

**A:** 添加到PATH或使用self-contained发布：

```bash
cd managed
dotnet publish -c Release --self-contained -r win-x64
```

### Q: 调用C#方法失败

**A:** 确保方法有正确的属性：

```csharp
// ✅ 正确
[UnmanagedCallersOnly(EntryPoint = "my_method")]
public static int MyMethod(IntPtr args, int size) { ... }

// ❌ 错误 - 缺少属性
public static int MyMethod(IntPtr args, int size) { ... }
```

---

## 下一步

✅ **Phase 1 完成** - 核心引擎可用

⏭️ **Phase 2: API绑定**
- 导出Axmol核心API（Node, Sprite, Scene等）
- 创建C#友好的包装层

⏭️ **Phase 3: 实战示例**
- 用C#编写完整游戏
- 性能优化
- 最佳实践

---

## 获取帮助

- 📖 [完整文档](README.md)
- 🔧 [构建指南](tests/BUILD_AND_TEST.md)
- 🌐 [Axmol Wiki](https://github.com/axmolengine/axmol/wiki)
- 💬 [Discord社区](https://discord.gg/QjaQBhFVay)
