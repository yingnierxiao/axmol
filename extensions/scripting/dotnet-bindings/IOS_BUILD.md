# iOS平台.NET支持 - 构建指南

## 当前状态

✅ **已完成**：
- CMake配置已支持iOS平台检测和Mono库链接
- Mono引擎实现（mono/MonoEngine.cpp）
- 跨平台架构设计
- Mono运行时库集成（arm64真机 + 模拟器）
- iOS特定的CMake配置（自动检测真机/模拟器架构）
- Mono库文件和头文件已就位

✅ **库文件位置**：
```
extensions/scripting/dotnet-bindings/mono/
├── lib/
│   ├── ios/arm64/              (真机 - iPhone/iPad)
│   │   ├── libmonosgen-2.0.a
│   │   ├── libmono-component-*.a
│   │   ├── libSystem.*.a
│   │   └── libicu*.a (ICU国际化库)
│   └── ios-simulator/
│       ├── arm64/              (M1/M2 Mac模拟器)
│       │   └── (同上)
│       └── x64/                (Intel Mac模拟器)
│           └── (同上)
└── include/mono-2.0/ (Mono头文件)
```

⏳ **待测试**：
- 实际的iOS项目编译（需要macOS + Xcode）
- .NET程序集在iOS设备上的运行验证
- AOT编译配置

## iOS平台架构

iOS使用**Mono运行时**（静态链接），原因：
- iOS不允许JIT编译（App Store规则）
- 必须使用AOT（Ahead-Of-Time）编译
- Mono提供成熟的iOS支持（Xamarin.iOS技术栈）
- 静态链接避免动态库加载限制

## 构建要求

### 1. 环境准备

**必需工具**：
```bash
- macOS 10.15+
- Xcode 13.0+
- CMake 3.22+
- .NET SDK 8.0+
```

**安装.NET SDK**：
```bash
# 使用Homebrew
brew install dotnet-sdk

# 或从官网下载
# https://dotnet.microsoft.com/download
```

### 2. Mono运行时库

Mono库已经下载并配置完成（从NuGet获取）：
- ✅ iOS真机（arm64）
- ✅ iOS模拟器（arm64 - M1/M2 Mac）
- ✅ iOS模拟器（x64 - Intel Mac）

**库来源**：
```bash
Microsoft.NETCore.App.Runtime.Mono.ios-arm64 (8.0.11)
Microsoft.NETCore.App.Runtime.Mono.iossimulator-arm64 (8.0.11)
Microsoft.NETCore.App.Runtime.Mono.iossimulator-x64 (8.0.11)
```

### 3. CMake配置详情

CMake会自动检测iOS编译目标：
- **真机**：`CMAKE_OSX_SYSROOT` 包含 "iPhoneOS" → 使用 `ios/arm64`
- **模拟器ARM64**：`CMAKE_OSX_SYSROOT` 包含 "iPhoneSimulator" + `CMAKE_OSX_ARCHITECTURES` 包含 "arm64" → 使用 `ios-simulator/arm64`
- **模拟器x64**：`CMAKE_OSX_SYSROOT` 包含 "iPhoneSimulator" + 其他架构 → 使用 `ios-simulator/x64`

CMakeLists.txt自动链接：
- libmonosgen-2.0.a（主运行时）
- libmono-component-*.a（Mono组件）
- libSystem.*.a（.NET系统库）
- libicu*.a（ICU国际化支持）

## 构建步骤

### 方法1：使用Xcode（推荐）

1. 打开Xcode项目：
```bash
cd projects/SlotGame
open proj.ios_mac/SlotGame.xcodeproj
```

2. 在Xcode中配置：
   - 选择目标：真机或模拟器
   - 确保CMake选项包含：`-DAX_ENABLE_EXT_DOTNET=ON`
   - 构建设置 → C++ Language Dialect → C++17或更高

3. 点击"Build"或"Run"

### 方法2：使用CMake命令行

#### iOS真机（Device）：
```bash
cd projects/SlotGame

cmake -B build-ios-device \
      -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_SYSROOT=iphoneos \
      -DAX_ENABLE_EXT_DOTNET=ON \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build-ios-device --config Release
```

#### iOS模拟器（Simulator - M1/M2 Mac）：
```bash
cmake -B build-ios-simulator \
      -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_SYSROOT=iphonesimulator \
      -DAX_ENABLE_EXT_DOTNET=ON \
      -DCMAKE_BUILD_TYPE=Debug

cmake --build build-ios-simulator --config Debug
```

## C#程序集部署

iOS应用需要将.NET程序集打包到App Bundle中：

### 1. 编译C#程序集（AOT）
```bash
cd extensions/scripting/dotnet-bindings/managed

# iOS真机
dotnet publish -c Release -r ios-arm64

# iOS模拟器（M1/M2）
dotnet publish -c Release -r iossimulator-arm64
```

### 2. 复制到iOS项目
```bash
# 将DLL复制到iOS项目资源目录
cp bin/Release/net8.0/ios-arm64/Axmol.Scripting.dll \
   ../../projects/SlotGame/proj.ios_mac/ios/

# 在Xcode中添加DLL到项目，确保"Copy Bundle Resources"中包含它
```

### 3. 运行时配置
在iOS应用启动时（AppDelegate.cpp），确保正确设置DLL搜索路径：
```cpp
// iOS: DLL在Bundle内
std::string dllPath = ax::FileUtils::getInstance()->fullPathForFilename(
    "Axmol.Scripting.dll"
);
```

## Info.plist配置

iOS可能需要额外的权限配置：

```xml
<key>NSAppTransportSecurity</key>
<dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>
</dict>

<!-- 如果使用文件I/O -->
<key>UIFileSharingEnabled</key>
<true/>
<key>LSSupportsOpeningDocumentsInPlace</key>
<true/>
```

## 测试

### 模拟器测试（推荐先测试）：
```bash
# 启动模拟器
open -a Simulator

# 在Xcode中选择模拟器目标并运行
# 或使用命令行
xcodebuild -scheme SlotGame \
           -configuration Debug \
           -destination 'platform=iOS Simulator,name=iPhone 15' \
           run
```

### 真机测试：
1. 连接iOS设备
2. 在Xcode中选择设备
3. 配置签名证书（需要Apple Developer账号）
4. 运行应用
5. 查看Xcode控制台输出：
```
[Axmol.NET] Initialized at ...
Runtime: Mono ...
Platform: iOS ...
```

## 已知限制

1. **AOT编译必需**：
   - 所有C#代码必须提前编译
   - 不支持动态代码生成（Reflection.Emit等）
   - 不支持动态加载程序集

2. **性能**：
   - Mono在iOS上性能略低于原生代码
   - AOT启动速度快，但运行时性能不如JIT

3. **体积**：
   - Mono静态库增加约10-20MB IPA大小
   - 每个架构都需要单独的库

4. **调试**：
   - 需要Xcode调试器
   - C#异常需要特殊处理才能在Xcode中显示

## 故障排除

### 问题1：Mono库找不到
**错误**：`Mono library not found for iOS at ...`

**解决**：
- 确认库文件存在：`ls extensions/scripting/dotnet-bindings/mono/lib/ios/arm64/`
- 检查CMake日志中的架构检测
- 重新运行CMake配置

### 问题2：链接错误 - 未定义符号
**错误**：`Undefined symbols for architecture arm64: "_mono_jit_init"`

**解决**：
- 确保所有.a文件都被链接（检查CMakeLists.txt）
- 添加缺失的系统框架（Foundation, CoreFoundation等）

### 问题3：C#程序集加载失败
**错误**：`Failed to load assembly: Axmol.Scripting.dll`

**解决**：
- 确认DLL已添加到Xcode项目的"Copy Bundle Resources"
- 检查DLL路径是否正确
- 确认DLL是为iOS编译的（不是Windows/Android版本）

## Mono AOT限制

iOS的AOT编译有以下限制：

**不支持的功能**：
- `Reflection.Emit`
- 动态代码生成
- 某些泛型操作（复杂泛型实例化）
- `System.Reflection.Assembly.Load(byte[])`

**解决方案**：
- 使用静态泛型类型
- 避免运行时代码生成
- 预先注册所有需要的类型

## 下一步

当前iOS平台的.NET集成配置已完成。需要在macOS环境中进行实际测试：

1. [ ] 在macOS上编译iOS模拟器版本
2. [ ] 在iOS模拟器中运行测试
3. [ ] 验证.NET初始化成功
4. [ ] 测试C#方法调用
5. [ ] 配置真机签名并测试
6. [ ] 优化AOT编译设置

## 参考资料

- [Mono iOS文档](https://www.mono-project.com/docs/compiling-mono/ios/)
- [.NET iOS支持](https://learn.microsoft.com/en-us/dotnet/core/deploying/native-aot/)
- [Xamarin.iOS AOT](https://learn.microsoft.com/en-us/xamarin/ios/internals/architecture)
- [Axmol iOS构建](https://axmol.dev)

---

**最后更新**: 2025-11-24
**版本**: 1.0 (配置完成，待测试)
