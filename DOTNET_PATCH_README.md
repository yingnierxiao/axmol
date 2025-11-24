# Axmol .NET集成补丁 - macOS测试指南

本补丁为Axmol游戏引擎添加完整的.NET 8支持，包括Windows、macOS、Linux、iOS和Android平台。

## 补丁内容

本补丁包含：
- ✅ 跨平台.NET运行时集成（CoreCLR + Mono）
- ✅ C++与C#互操作实现
- ✅ CMake构建配置
- ✅ C#示例代码
- ✅ 完整文档

**已验证平台**：
- ✅ Windows（CoreCLR）- 已测试成功

**已配置平台**（待测试）：
- ⏳ macOS（CoreCLR）
- ⏳ Linux（CoreCLR）
- ⏳ iOS（Mono）
- ⏳ Android（Mono）

## 在macOS上应用补丁

### 前置要求

1. **macOS系统**：10.15 Catalina或更高
2. **Xcode**：13.0+（用于编译器）
3. **Homebrew**：用于安装依赖

### 步骤1：准备环境

```bash
# 安装.NET SDK
brew install dotnet-sdk

# 安装CMake
brew install cmake

# 验证安装
dotnet --version  # 应输出 8.0.x
cmake --version   # 应输出 3.22+
```

### 步骤2：应用补丁

```bash
# 进入Axmol根目录
cd /path/to/axmol

# 应用补丁
git apply dotnet-integration.patch

# 如果遇到行尾符问题，使用：
git apply --ignore-whitespace dotnet-integration.patch
```

### 步骤3：运行设置脚本

```bash
# 给脚本添加执行权限
chmod +x setup-dotnet-macos.sh

# 运行设置脚本（自动下载Mono库并配置）
./setup-dotnet-macos.sh
```

**脚本会自动完成**：
- 下载iOS Mono运行时库（约30MB）
- 解压并配置库文件
- 编译C#程序集
- 验证CMake配置

**预计时间**：5-10分钟（取决于网络速度）

### 步骤4：编译测试项目

```bash
cd projects/SlotGame

# 配置CMake（启用.NET支持）
cmake -B build \
      -DCMAKE_BUILD_TYPE=Debug \
      -DAX_ENABLE_EXT_DOTNET=ON

# 编译
cmake --build build --config Debug --target SlotGame
```

### 步骤5：运行测试

```bash
# 进入可执行文件目录
cd build/bin/SlotGame/Debug

# 运行游戏
./SlotGame
```

**预期输出**：
```
=========================================
初始化 .NET 脚本引擎...
=========================================
[CoreCLR] Initializing CoreCLR runtime...
[CoreCLR] Runtime loaded successfully
[AppDelegate] .NET Engine initialized successfully
[CoreCLR] Method executed successfully. Return value: 0
[AppDelegate] .NET C# Initialize method called
=========================================
```

**验证成功标志**：
- 控制台显示`.NET Engine initialized successfully`
- 在可执行文件目录生成`DOTNET_INITIALIZED.txt`文件
- 文件内容包含运行时版本和平台信息

### 步骤6：查看初始化文件

```bash
cat DOTNET_INITIALIZED.txt
```

**预期内容**：
```
[Axmol.NET] Initialized at 2025/11/24 ...
Runtime: .NET 8.0.20
Platform: Darwin 23.x.x ...
```

## 补丁文件说明

### 核心代码文件

**CMake配置**：
- `extensions/scripting/dotnet-bindings/CMakeLists.txt` - 跨平台构建配置
- `projects/SlotGame/CMakeLists.txt` - SlotGame项目.NET链接

**C++实现**：
- `extensions/scripting/dotnet-bindings/common/ScriptEngine.*` - 统一接口
- `extensions/scripting/dotnet-bindings/coreclr/CoreCLREngine.*` - CoreCLR实现
- `extensions/scripting/dotnet-bindings/mono/MonoEngine.*` - Mono实现

**C#代码**：
- `extensions/scripting/dotnet-bindings/managed/Axmol.Scripting.csproj` - C#项目
- `extensions/scripting/dotnet-bindings/managed/EngineAPI.cs` - C#入口点

**应用集成**：
- `projects/SlotGame/Source/AppDelegate.cpp` - .NET初始化代码

### 文档文件

- `PLATFORM_STATUS.md` - 跨平台集成状态总览
- `DESKTOP_BUILD.md` - macOS/Linux详细构建指南
- `IOS_BUILD.md` - iOS详细构建指南
- `ANDROID_BUILD.md` - Android详细构建指南

## 在macOS上测试iOS（可选）

如果您想测试iOS平台：

```bash
# 打开Xcode项目
cd projects/SlotGame
open proj.ios_mac/SlotGame.xcodeproj

# 在Xcode中：
# 1. 选择目标：iOS模拟器或真机
# 2. 确保CMake选项包含 -DAX_ENABLE_EXT_DOTNET=ON
# 3. 点击 Run
```

详细说明请参考：`extensions/scripting/dotnet-bindings/IOS_BUILD.md`

## 故障排除

### 问题1：nethost库找不到

**症状**：
```
.NET Scripting: nethost library not found
```

**解决**：
```bash
# 设置DOTNET_ROOT环境变量
export DOTNET_ROOT=/usr/local/share/dotnet

# 或重新安装.NET SDK
brew reinstall dotnet-sdk

# 重新运行CMake
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
```

### 问题2：补丁应用失败

**症状**：
```
error: patch failed: ...
```

**解决**：
```bash
# 使用3-way merge
git apply --3way dotnet-integration.patch

# 或忽略空白字符
git apply --ignore-whitespace dotnet-integration.patch

# 检查冲突
git status
```

### 问题3：CMake找不到.NET配置

**症状**：
```
CMake Error: Could not find .NET Scripting configuration
```

**解决**：
```bash
# 确认补丁完全应用
git diff extensions/scripting/dotnet-bindings/CMakeLists.txt

# 清理并重新配置
rm -rf build
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
```

### 问题4：运行时找不到DLL

**症状**：
```
Failed to load assembly: Axmol.Scripting.dll
```

**解决**：
```bash
# 确保C#程序集已编译
cd extensions/scripting/dotnet-bindings/managed
dotnet build -c Debug

# 复制DLL到游戏目录
cp bin/Debug/net8.0/Axmol.Scripting.dll \
   ../../projects/SlotGame/build/bin/SlotGame/Debug/

cp bin/Debug/net8.0/Axmol.Scripting.runtimeconfig.json \
   ../../projects/SlotGame/build/bin/SlotGame/Debug/
```

## 性能测试建议

在macOS上测试时，建议检查：

1. **启动时间**：.NET初始化应在1秒内完成
2. **内存占用**：增加约30-50MB（包含CoreCLR运行时）
3. **帧率影响**：应无明显影响（<1ms开销）
4. **热重载**：测试C#代码更改后是否能快速重新加载

## Apple Silicon (M1/M2) 注意事项

本补丁完全支持Apple Silicon：

```bash
# 检查架构
file build/bin/SlotGame/Debug/SlotGame
# 应输出：Mach-O 64-bit executable arm64

# 如需构建通用二进制（x86_64 + arm64）
cmake -B build-universal \
      -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
      -DAX_ENABLE_EXT_DOTNET=ON
```

## 下一步

测试成功后，您可以：

1. **扩展C#功能**
   - 在`EngineAPI.cs`中添加更多C#方法
   - 实现游戏逻辑层

2. **测试其他平台**
   - Linux：在虚拟机或WSL中测试
   - iOS：在Xcode中编译并在模拟器/真机测试
   - Android：配置Android Studio并编译APK

3. **性能优化**
   - 测量C++↔C#互操作开销
   - 优化热点路径

4. **贡献反馈**
   - 报告测试结果
   - 提交问题和改进建议

## 技术支持

遇到问题？请查看：

1. **详细文档**：`extensions/scripting/dotnet-bindings/PLATFORM_STATUS.md`
2. **构建日志**：查看CMake和编译器输出
3. **运行时日志**：检查控制台和`debug.log`

## 补丁文件清单

```
dotnet-integration.patch          # 主补丁文件（163KB）
setup-dotnet-macos.sh            # macOS自动设置脚本
DOTNET_PATCH_README.md          # 本文档
```

## 许可证

本补丁继承Axmol引擎的MIT许可证。

---

**创建时间**：2025-11-24
**补丁版本**：2.0
**测试状态**：Windows已验证，macOS待测试
**预计测试时间**：30-60分钟

祝测试顺利！🚀
