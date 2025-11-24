# Axmol .NET 跨平台集成方案 - 总结文档

## 🎯 项目概述

为Axmol游戏引擎创建**全平台.NET脚本绑定**，支持使用C#编写游戏逻辑。

### 核心目标
✅ **全平台支持** - Windows, Linux, macOS, iOS, Android
✅ **双运行时架构** - CoreCLR(桌面) + Mono(移动)
✅ **统一API接口** - 一套代码，多平台运行
✅ **基于CMake构建** - 无缝集成Axmol现有构建系统

---

## 📊 技术方案对比

| 方案 | 平台支持 | 性能 | 复杂度 | 选择 |
|------|----------|------|--------|------|
| **P/Invoke + 双运行时** | 全平台 | 优秀 | 中等 | ✅ **采用** |
| 仅CoreCLR（桌面） | 仅桌面 | 最佳 | 简单 | ❌ 不够全面 |
| 全面使用Mono | 全平台 | 良好 | 简单 | ❌ 桌面性能差 |
| C++/CLI | 仅Windows | 优秀 | 简单 | ❌ 不跨平台 |

---

## 🏗️ 架构设计

### 目录结构

```
extensions/scripting/dotnet-bindings/
├── common/                    # 公共抽象层 ✅
│   ├── ScriptEngine.h         # 统一接口定义
│   ├── ScriptEngine.cpp       # 工厂实现
│   └── ScriptTypes.h          # 类型定义
│
├── coreclr/                   # 桌面平台 (Windows/Linux/macOS) ✅
│   ├── CoreCLREngine.h        # CoreCLR实现头文件
│   └── CoreCLREngine.cpp      # [待实现]
│
├── mono/                      # 移动平台 (iOS/Android) ✅
│   ├── MonoEngine.h           # Mono实现头文件
│   └── MonoEngine.cpp         # [待实现]
│
├── managed/                   # C#代码 ✅
│   ├── Axmol.Scripting.csproj.template
│   └── EngineAPI.cs.template
│
├── docs/                      # 文档 ✅
│   ├── INTEGRATION.md         # 集成指南
│   └── PLATFORM_NOTES.md      # 平台说明
│
├── CMakeLists.txt             # CMake配置 ✅
└── README.md                  # 项目说明 ✅
```

### 架构图

```
┌────────────────────────────────────────────────────┐
│              Axmol C++ Game Engine                 │
│        (Node, Sprite, Scene, Director...)          │
└───────────────────┬────────────────────────────────┘
                    │
        ┌───────────▼──────────────┐
        │  ScriptEngineManager     │ 单例管理器
        │  • getInstance()         │
        │  • initialize()          │
        └───────────┬──────────────┘
                    │
        ┌───────────▼──────────────┐
        │     ScriptEngine         │ 抽象接口
        │  • initialize()          │
        │  • loadAssembly()        │
        │  • executeMethod()       │
        │  • createInstance()      │
        └───────────┬──────────────┘
                    │
           ┌────────┴─────────┐
           │                  │
    ┌──────▼──────┐    ┌─────▼───────┐
    │CoreCLREngine│    │ MonoEngine  │
    │             │    │             │
    │ • nethost  │    │ • mono_jit │
    │ • hostfxr  │    │ • mono AOT │
    └──────┬──────┘    └─────┬───────┘
           │                  │
    ┌──────▼──────────────────▼───────┐
    │    Axmol.Scripting.dll (C#)     │
    │  • EngineAPI                    │
    │  • Game Logic                   │
    └─────────────────────────────────┘
```

---

## 📦 已完成的工作

### ✅ 第一阶段：基础架构（已完成）

| 任务 | 状态 | 文件 |
|------|------|------|
| 目录结构创建 | ✅ | `/dotnet-bindings/` |
| 抽象接口设计 | ✅ | `common/ScriptEngine.h` |
| 类型系统定义 | ✅ | `common/ScriptTypes.h` |
| CoreCLR头文件 | ✅ | `coreclr/CoreCLREngine.h` |
| Mono头文件 | ✅ | `mono/MonoEngine.h` |
| CMake配置 | ✅ | `CMakeLists.txt` |
| C#项目模板 | ✅ | `managed/*.template` |
| 文档编写 | ✅ | `README.md`, `docs/*.md` |

---

## 🔄 平台运行时映射

### 桌面平台 (CoreCLR)

```
Windows x64
  └─ nethost.dll
      └─ hostfxr.dll
          └─ coreclr.dll (.NET 8 Runtime)
              └─ Axmol.Scripting.dll (Your C# code)

Linux x64
  └─ libnethost.so
      └─ libhostfxr.so
          └─ libcoreclr.so
              └─ Axmol.Scripting.dll

macOS x64/arm64
  └─ libnethost.dylib
      └─ libhostfxr.dylib
          └─ libcoreclr.dylib
              └─ Axmol.Scripting.dll
```

### 移动平台 (Mono)

```
iOS (AOT Only)
  └─ libmono-native.a (静态链接)
      └─ Axmol.Scripting.dll (预AOT编译)
          └─ Axmol.Scripting.dll.o (AOT对象文件)

Android
  └─ libmono-android.so
      └─ libmonosgen-2.0.so
          └─ Axmol.Scripting.dll
```

---

## 📝 核心API接口

### C++ 侧接口

```cpp
// 初始化
auto& manager = ScriptEngineManager::getInstance();
ScriptEngineConfig config;
config.configPath = "path/to/runtimeconfig.json";
manager.initialize(config);

// 获取引擎实例
auto* engine = manager.getEngine();

// 调用静态方法
engine->executeMethod("Namespace.Class", "Method");

// 创建C#对象
auto handle = engine->createInstance("Namespace.MyClass");

// 调用实例方法
engine->callMethod(handle, "Update", &args);

// 清理
manager.shutdown();
```

### C# 侧接口

```csharp
// 导出给C++调用的方法
[UnmanagedCallersOnly(EntryPoint = "initialize")]
public static int Initialize(IntPtr args, int size)
{
    // 初始化逻辑
    return 0;
}

// 调用C++函数
[DllImport("axmol")]
public static extern void NativeLog(string message);
```

---

## 🎮 使用示例

### 完整流程

#### 1. 启用扩展（CMake）

```bash
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build
```

#### 2. 创建C#项目

```bash
cd extensions/scripting/dotnet-bindings/managed
cp *.template .
dotnet build -c Release
```

#### 3. C++中初始化

```cpp
// AppDelegate.cpp
#include "extensions/scripting/dotnet-bindings/common/ScriptEngine.h"

bool AppDelegate::applicationDidFinishLaunching()
{
    auto& scriptMgr = ax::scripting::dotnet::ScriptEngineManager::getInstance();

    ax::scripting::dotnet::ScriptEngineConfig config;
    config.configPath = FileUtils::getInstance()->fullPathForFilename(
        "Axmol.Scripting.runtimeconfig.json"
    );

    if (scriptMgr.initialize(config)) {
        auto* engine = scriptMgr.getEngine();
        engine->executeMethod("Axmol.Scripting.EngineAPI", "Initialize");
    }

    return true;
}
```

#### 4. 运行游戏

```bash
./build/bin/YourGame
```

输出：
```
[CoreCLR] Initializing...
[Axmol.NET] Scripting system initialized
[Game] Initializing game logic
```

---

## 🚀 后续实施步骤

### Phase 2: 实现运行时引擎（预计2-3周）

```
□ 完成 CoreCLREngine.cpp 实现
  ├─ loadHostFxr()
  ├─ initializeRuntime()
  ├─ loadAssembly()
  ├─ executeMethod()
  └─ 错误处理

□ 完成 MonoEngine.cpp 实现
  ├─ mono_jit_init()
  ├─ mono_domain_assembly_open()
  ├─ mono_class_from_name()
  ├─ mono_runtime_invoke()
  └─ iOS AOT支持

□ 单元测试
  ├─ Windows桌面测试
  ├─ Linux桌面测试
  ├─ macOS桌面测试
  ├─ iOS设备测试
  └─ Android设备测试
```

### Phase 3: API绑定（预计2-3周）

```
□ 导出核心Axmol API
  ├─ Director (导演)
  ├─ Scene (场景)
  ├─ Node (节点)
  ├─ Sprite (精灵)
  ├─ Action (动作)
  └─ EventDispatcher (事件)

□ 创建C#包装层
  ├─ Axmol.Core.dll
  ├─ Axmol.UI.dll
  └─ Axmol.Physics.dll
```

### Phase 4: 示例和文档（预计1周）

```
□ 创建示例项目
  ├─ HelloWorld (基础)
  ├─ FlappyBird (2D游戏)
  └─ 3D Demo (3D演示)

□ 完善文档
  ├─ API参考文档
  ├─ 最佳实践
  └─ 性能优化指南
```

---

## ⚙️ CMake集成方式

### 在项目中启用

```cmake
# 1. 修改 core/CMakeLists.txt
option(AX_ENABLE_EXT_DOTNET "Build .NET scripting" OFF)

# 2. 修改 extensions/CMakeLists.txt
if(AX_ENABLE_EXT_DOTNET)
  add_subdirectory(scripting/dotnet-bindings)
endif()

# 3. 构建
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
```

### 平台检测逻辑

```cmake
# 在 dotnet-bindings/CMakeLists.txt 中
if(WINDOWS OR LINUX OR (MACOSX AND NOT IOS))
    # 使用 CoreCLR
    set(DOTNET_RUNTIME_TYPE "CoreCLR")
    add_definitions(-DAX_DOTNET_USE_CORECLR)

elseif(IOS OR ANDROID)
    # 使用 Mono
    set(DOTNET_RUNTIME_TYPE "Mono")
    add_definitions(-DAX_DOTNET_USE_MONO)
endif()
```

---

## 📚 参考资源

### 官方文档
- [.NET Hosting API](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
- [Mono Embedding Guide](https://www.mono-project.com/docs/advanced/embedding/)
- [Mono Game Engine Guide](https://peter1745.github.io/mono-guide/book/)

### 示例项目
- [UnrealCLR](https://github.com/nxrighthere/UnrealCLR) - Unreal Engine .NET集成
- [FlaxEngine](https://github.com/FlaxEngine/FlaxEngine) - C++/C#混合引擎
- [.NET Hosting Sample](https://github.com/dotnet/samples/tree/main/core/hosting)

### Axmol相关
- [Axmol Wiki](https://github.com/axmolengine/axmol/wiki)
- [Lua绑定参考](../lua-bindings/) - 类似的脚本绑定实现

---

## ✅ 项目检查清单

### 已完成 ✅
- [x] 目录结构设计
- [x] 抽象接口层
- [x] CoreCLR头文件
- [x] Mono头文件
- [x] CMake配置文件
- [x] C#项目模板
- [x] README文档
- [x] 平台说明文档
- [x] 集成指南文档

### 待实现 □
- [ ] CoreCLREngine.cpp 完整实现
- [ ] MonoEngine.cpp 完整实现
- [ ] ScriptEngine.cpp 工厂实现
- [ ] 跨平台测试用例
- [ ] 示例游戏项目
- [ ] API绑定层
- [ ] 性能测试

---

## 🎯 成功标准

### 桌面平台（CoreCLR）
- ✅ Windows 10+ 64位
- ✅ Ubuntu 20.04+ 64位
- ✅ macOS 11+ (x64/arm64)
- 目标：能够加载并运行简单的C#脚本

### 移动平台（Mono）
- ✅ iOS 13+ (arm64)
- ✅ Android 7.0+ (arm64-v8a, armeabi-v7a)
- 目标：iOS上AOT编译通过，Android上运行正常

### 性能目标
- C++→C# 调用延迟 < 100ns
- 运行时初始化 < 500ms
- 内存占用增加 < 100MB

---

## 📞 技术支持

**问题反馈：**
- GitHub Issues: https://github.com/axmolengine/axmol/issues
- Discord: https://discord.gg/QjaQBhFVay

**贡献指南：**
1. Fork仓库
2. 创建功能分支
3. 提交Pull Request
4. 遵循代码规范

---

## 📄 许可证

MIT License - 与Axmol主项目保持一致

---

**最后更新：** 2025-01-24
**版本：** 1.0.0-alpha
**状态：** 架构设计完成，待实现运行时引擎
