# Axmol .NET集成 - 跨平台状态总结

## 项目概述

本项目将.NET 8运行时集成到Axmol C++游戏引擎中，支持在C++中嵌入和调用C#代码（反向托管）。

## 架构设计

采用**双运行时架构**，根据平台特性选择最适合的.NET运行时：

| 平台 | 运行时 | 原因 |
|------|--------|------|
| Windows | CoreCLR | JIT支持，高性能，官方支持 |
| Linux | CoreCLR | JIT支持，跨平台一致性 |
| macOS | CoreCLR | JIT支持，与iOS区分 |
| iOS | Mono | AOT必需（App Store要求），成熟的移动方案 |
| Android | Mono | AOT必需（Android限制），Xamarin技术栈 |

## 平台状态详情

### ✅ Windows平台（已完成并测试）

**状态**: 生产就绪

**运行时**: CoreCLR (nethost.dll + hostfxr.dll)

**已完成**:
- ✅ CMake配置自动查找.NET SDK
- ✅ CoreCLREngine.cpp实现完整
- ✅ 成功加载Axmol.Scripting.dll
- ✅ 成功调用C# Initialize方法
- ✅ Debug和Release模式均测试通过
- ✅ 文件I/O验证通过（DOTNET_INITIALIZED.txt创建成功）

**测试结果**:
```
[CoreCLR] Method executed successfully. Return value: 0
[Axmol.NET] Initialized at 2025/11/24 20:21:28
Runtime: .NET 8.0.20
Platform: Microsoft Windows 10.0.22621
```

**关键文件**:
- `extensions/scripting/dotnet-bindings/CMakeLists.txt:74-121` - CoreCLR配置
- `extensions/scripting/dotnet-bindings/coreclr/CoreCLREngine.cpp` - 引擎实现
- `projects/SlotGame/Source/AppDelegate.cpp:184-232` - 初始化代码

---

### ✅ Android平台（配置完成，待实测）

**状态**: 配置就绪，等待Android环境测试

**运行时**: Mono (libmonosgen-2.0.so)

**已完成**:
- ✅ 下载并集成Mono Android运行时库（arm64-v8a, armeabi-v7a, x86_64）
- ✅ CMake配置支持Android平台检测
- ✅ MonoEngine.cpp实现完整
- ✅ 头文件和库文件已部署到正确位置

**库文件位置**:
```
extensions/scripting/dotnet-bindings/mono/
├── lib/android/
│   ├── arm64-v8a/
│   │   ├── libmonosgen-2.0.so
│   │   ├── libmono-component-*.so
│   │   └── libSystem.*.so
│   ├── armeabi-v7a/ (同上)
│   └── x86_64/ (同上)
└── include/mono-2.0/ (Mono C API头文件)
```

**待测试**:
- ⏳ 使用Android NDK实际编译APK
- ⏳ 在Android设备/模拟器上运行验证
- ⏳ C#程序集部署到assets目录
- ⏳ Mono AOT编译配置

**构建命令参考**:
```bash
cd projects/SlotGame/proj.android

# 配置CMake（arm64-v8a示例）
cmake -B build/arm64-v8a \
      -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-21 \
      -DAX_ENABLE_EXT_DOTNET=ON

# 构建APK
./gradlew assembleDebug
```

**文档**: `extensions/scripting/dotnet-bindings/ANDROID_BUILD.md`

---

### ✅ Linux平台（配置完成，待实测）

**状态**: CMake配置就绪，待Linux环境测试

**运行时**: CoreCLR (libnethost.so)

**已完成**:
- ✅ CMake配置支持Linux平台检测
- ✅ CoreCLREngine.cpp跨平台实现（与Windows共享）
- ✅ 自动查找.NET SDK路径（/usr/share/dotnet）
- ✅ 支持多发行版（Ubuntu/Debian/Fedora/Arch）
- ✅ 完整构建文档

**待测试**:
- ⏳ 在Linux系统上编译测试
- ⏳ 验证nethost库加载
- ⏳ 验证C#程序集执行

**预期路径**:
- nethost: `/usr/share/dotnet/packs/Microsoft.NETCore.App.Host.linux-x64/*/runtimes/linux-x64/native/libnethost.so`

**文档**: `extensions/scripting/dotnet-bindings/DESKTOP_BUILD.md`

---

### ✅ macOS平台（配置完成，待实测）

**状态**: CMake配置就绪，待macOS环境测试

**运行时**: CoreCLR (libnethost.dylib)

**已完成**:
- ✅ CMake配置支持macOS平台检测
- ✅ CoreCLREngine.cpp跨平台实现（与Windows共享）
- ✅ 自动查找.NET SDK路径（/usr/local/share/dotnet）
- ✅ 支持Intel和Apple Silicon（M1/M2）
- ✅ 完整构建文档

**待测试**:
- ⏳ 在macOS系统上编译测试
- ⏳ 验证nethost库加载
- ⏳ 验证C#程序集执行

**预期路径**:
- nethost: `/usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-x64/*/runtimes/osx-x64/native/libnethost.dylib`

**文档**: `extensions/scripting/dotnet-bindings/DESKTOP_BUILD.md`

---

### ✅ iOS平台（配置完成，待实测）

**状态**: 配置就绪，等待macOS+Xcode环境测试

**运行时**: Mono (libmonosgen-2.0.a - 静态链接)

**已完成**:
- ✅ CMake配置支持iOS平台检测（真机+模拟器）
- ✅ MonoEngine.cpp实现完整（与Android共享）
- ✅ iOS_MONO_AOT_ONLY宏定义
- ✅ 获取Mono iOS运行时库（静态库.a文件）
- ✅ 配置iOS特定的CMake路径和架构检测
- ✅ 自动链接所有必需的Mono组件库

**库文件位置**:
```
extensions/scripting/dotnet-bindings/mono/lib/
├── ios/arm64/                  (真机 - iPhone/iPad)
│   ├── libmonosgen-2.0.a
│   ├── libmono-component-*.a
│   ├── libSystem.*.a
│   └── libicu*.a
├── ios-simulator/arm64/        (M1/M2 Mac模拟器)
│   └── (同上)
└── ios-simulator/x64/          (Intel Mac模拟器)
    └── (同上)
```

**待测试**:
- ⏳ 在Xcode中编译iOS项目
- ⏳ 在iOS模拟器上运行验证
- ⏳ 在iOS真机上运行验证
- ⏳ C#程序集AOT编译
- ⏳ App Bundle打包和部署

**iOS特殊要求**:
- iOS不允许JIT，必须使用AOT编译
- Mono库需要静态链接（.a文件）
- 需要配置Info.plist权限
- 需要Apple Developer证书（真机测试）

**文档**: `extensions/scripting/dotnet-bindings/IOS_BUILD.md`

---

## 技术实现细节

### 1. 核心组件

| 组件 | 路径 | 作用 |
|------|------|------|
| ScriptEngine.h/cpp | common/ | 统一的脚本引擎接口 |
| CoreCLREngine.h/cpp | coreclr/ | CoreCLR运行时封装 |
| MonoEngine.h/cpp | mono/ | Mono运行时封装 |
| CMakeLists.txt | 根目录 | 平台检测和库链接配置 |

### 2. C# 托管程序集

| 文件 | 路径 | 作用 |
|------|------|------|
| Axmol.Scripting.dll | managed/ | 主程序集 |
| EngineAPI.cs | managed/ | C++调用的C#入口点 |
| Axmol.Scripting.runtimeconfig.json | - | 运行时配置 |

### 3. 关键技术点

**CoreCLR平台**:
- 使用.NET Hosting API (nethost.dll/libnethost)
- 通过`load_assembly_and_get_function_pointer`调用C#方法
- 支持UnmanagedCallersOnly特性的方法调用
- 程序集类型需要使用完全限定名：`"Namespace.Class, AssemblyName"`

**Mono平台**:
- 使用Mono Embedding API (libmonosgen-2.0)
- 通过`mono_jit_init`初始化运行时
- 支持AOT编译（iOS/Android必需）
- 需要手动管理AppDomain和Assembly对象

## 构建说明

### Windows (已验证)

```bash
# 配置CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAX_ENABLE_EXT_DOTNET=ON

# 编译
cmake --build build --config Release --target SlotGame

# 运行（会自动加载.NET）
build/bin/SlotGame/Release/SlotGame.exe
```

### Android (配置就绪)

```bash
cd projects/SlotGame/proj.android

# 确保gradle.properties中启用了所需架构
# __1K_ARCHS=arm64-v8a:armeabi-v7a

# 编译C#程序集
cd ../../extensions/scripting/dotnet-bindings/managed
dotnet publish -c Release -r android-arm64

# 编译APK
cd ../../../projects/SlotGame/proj.android
./gradlew assembleDebug

# 安装测试
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat | grep "Axmol.NET"
```

### Linux/macOS (配置就绪)

```bash
# 安装.NET SDK 8.0+
# Linux: sudo apt install dotnet-sdk-8.0
# macOS: brew install dotnet-sdk

# 配置和编译（与Windows类似）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build --config Release --target SlotGame
```

## 下一步工作

**所有平台配置已完成！** 根据用户需求"需要确认所有平台能运行"，现在需要在相应环境中进行实际测试：

### 优先级1: 完成已配置平台的实机测试

**Windows** ✅ - 已完成并验证
- ✅ 编译成功
- ✅ 运行时测试通过
- ✅ C#调用验证成功

**待测试的平台**（配置已完成，需要相应环境）：

1. **macOS测试**（需要macOS系统）
   - 在macOS上编译SlotGame
   - 验证CoreCLR加载和C#执行
   - 测试Intel和Apple Silicon兼容性
   - 参考文档：`DESKTOP_BUILD.md`

2. **Linux测试**（需要Linux系统）
   - 在Ubuntu/Debian上编译SlotGame
   - 验证CoreCLR加载和C#执行
   - 测试不同发行版兼容性
   - 参考文档：`DESKTOP_BUILD.md`

3. **iOS测试**（需要macOS + Xcode）
   - 在Xcode中编译iOS项目
   - 在iOS模拟器上测试
   - 在真机上测试（需要开发者证书）
   - 参考文档：`IOS_BUILD.md`

4. **Android测试**（需要Android NDK + SDK）
   - 使用Gradle编译APK
   - 在Android设备/模拟器上测试
   - 验证Mono运行时工作
   - 参考文档：`ANDROID_BUILD.md`

### 优先级2: 功能完善
1. 实现更多的C++↔C#互操作API
2. 添加异常处理和错误恢复
3. 性能优化和内存管理
4. 编写单元测试

### 优先级3: 高级特性
1. C#热重载支持
2. 调试器集成
3. 性能分析工具
4. 自动化测试套件

## 参考资料

- [.NET Hosting API文档](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
- [Mono Embedding指南](https://www.mono-project.com/docs/advanced/embedding/)
- [Android .NET支持](https://learn.microsoft.com/en-us/dotnet/core/deploying/native-aot/)
- [Axmol引擎文档](https://axmol.dev)

---

**最后更新**: 2025-11-24
**版本**: 2.0 (所有平台配置完成 - Windows已验证，Android/iOS/macOS/Linux待实测)
