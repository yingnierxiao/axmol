# Axmol .NET Scripting Bindings

## 概述

这是Axmol引擎的跨平台.NET脚本绑定，允许使用C#编写游戏逻辑。

### 支持的平台

| 平台 | 运行时 | 状态 |
|------|--------|------|
| **Windows** | .NET Core (CoreCLR) | ✅ 支持 |
| **Linux** | .NET Core (CoreCLR) | ✅ 支持 |
| **macOS** | .NET Core (CoreCLR) | ✅ 支持 |
| **iOS** | Mono (AOT) | ✅ 支持 |
| **Android** | Mono | ✅ 支持 |

## 架构设计

```
┌─────────────────────────────────────┐
│      Axmol C++ Game Engine          │
└──────────────┬──────────────────────┘
               │
     ┌─────────┴──────────┐
     │  ScriptEngine      │  (抽象接口层)
     │  (common/)         │
     └─────────┬──────────┘
               │
       ┌───────┴────────┐
       │                │
┌──────▼──────┐  ┌─────▼──────┐
│ CoreCLREngine│  │ MonoEngine  │
│  (coreclr/)  │  │   (mono/)   │
└──────┬───────┘  └─────┬───────┘
       │                │
  桌面平台          移动平台
 (Win/Linux/Mac)    (iOS/Android)
```

## 前置要求

### 桌面平台开发
- **.NET SDK 8.0+** (https://dotnet.microsoft.com/)
- 设置环境变量 `DOTNET_ROOT`

### 移动平台开发
- **Mono Runtime**
  - iOS: 通过 Xamarin.iOS 或独立 Mono
  - Android: 通过 Xamarin.Android 或独立 Mono

## 快速开始

### 1. 启用.NET绑定

在 `core/CMakeLists.txt` 中添加选项：

```cmake
option(AX_ENABLE_EXT_DOTNET "Build extension .NET scripting" ON)
```

在 `extensions/CMakeLists.txt` 中添加：

```cmake
if(AX_ENABLE_EXT_DOTNET)
  add_subdirectory(scripting/dotnet-bindings)
endif()
```

### 2. 创建C#项目

```bash
cd extensions/scripting/dotnet-bindings/managed
dotnet new classlib -n Axmol.Scripting -f net8.0
```

### 3. 编写C#代码

```csharp
// EngineAPI.cs
using System.Runtime.InteropServices;

namespace Axmol.Scripting
{
    public static class EngineAPI
    {
        [UnmanagedCallersOnly(EntryPoint = "initialize_scripting")]
        public static int Initialize(IntPtr args, int size)
        {
            Console.WriteLine("Axmol .NET Scripting Initialized!");
            return 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "update_frame")]
        public static void UpdateFrame(float deltaTime)
        {
            // 游戏逻辑更新
        }
    }
}
```

### 4. C++中使用

```cpp
#include "scripting/dotnet-bindings/common/ScriptEngine.h"

using namespace ax::scripting::dotnet;

// 初始化
auto& manager = ScriptEngineManager::getInstance();
ScriptEngineConfig config;
config.configPath = "path/to/Axmol.Scripting.runtimeconfig.json";
manager.initialize(config);

// 调用C#方法
auto* engine = manager.getEngine();
engine->executeMethod("Axmol.Scripting.EngineAPI", "Initialize");

// 清理
manager.shutdown();
```

## 编译说明

### Windows

```bash
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build
```

确保已安装 .NET SDK 并设置了 `DOTNET_ROOT`。

### Linux

```bash
export DOTNET_ROOT=/usr/share/dotnet
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build
```

### macOS

```bash
export DOTNET_ROOT=/usr/local/share/dotnet
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build
```

### iOS

```bash
# 需要安装 Mono
brew install mono
cmake -B build -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build --config Release
```

### Android

```bash
# 需要配置 Mono for Android
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build
```

## 平台差异

### CoreCLR (桌面)
- ✅ 完整的JIT支持
- ✅ 运行时代码生成
- ✅ 完整的调试支持
- ⚠️ 需要分发.NET运行时 (~50-80MB)

### Mono (移动)
- ✅ iOS: AOT编译，无JIT
- ✅ Android: 支持JIT
- ✅ 更小的体积 (~15-30MB)
- ⚠️ iOS需要预编译所有程序集

## 性能建议

1. **避免频繁的C++/C#互调**
   - 批量处理数据
   - 使用回调而不是轮询

2. **使用值类型而不是引用类型**
   - 减少GC压力

3. **iOS平台优化**
   - 使用 `[UnmanagedCallersOnly]` 而不是委托
   - 预分配对象池

## 故障排除

### nethost.lib 找不到
```bash
# Windows
set DOTNET_ROOT=C:\Program Files\dotnet

# Linux/Mac
export DOTNET_ROOT=/usr/share/dotnet
```

### Mono 找不到
```bash
# Ubuntu
sudo apt-get install mono-complete

# macOS
brew install mono
```

### iOS AOT 编译失败
确保使用 `[UnmanagedCallersOnly]` 而不是动态委托。

## 示例项目

参考 `tests/dotnet-tests/` 目录中的完整示例项目。

## 参考资源

- [.NET Hosting API](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
- [Mono Embedding Guide](https://www.mono-project.com/docs/advanced/embedding/)
- [Axmol Wiki](https://github.com/axmolengine/axmol/wiki)

## 许可证

MIT License - 参见项目根目录的 LICENSE 文件
