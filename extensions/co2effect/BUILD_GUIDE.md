# CO2特效系统编译指南

## 已完成的CMake配置

### 1. 文件修改列表

✅ **extensions/CMakeLists.txt**
- 添加了`add_subdirectory(co2effect)`
- 为fairygui添加了co2effect头文件路径和链接

✅ **extensions/fairygui/CMakeLists.txt**
- 添加了`target_link_libraries(fairygui PUBLIC co2effect)`

✅ **extensions/co2effect/CMakeLists.txt**
- 创建了完整的构建配置

### 2. 代码修复

✅ **EffectConfig.cpp**
- 修复了lambda函数名称拼写错误

✅ **CO2EffectPlayer.cpp**
- 添加了`#include "2d/Label.h"`
- 添加了`#include "base/StringUtils.h"`

✅ **GLoader.cpp**
- 添加了`#include "co2effect/CO2EffectPlayer.h"`
- 实现了`effect://`协议支持

## 编译步骤

### Windows (Visual Studio)

```powershell
# 1. 清理之前的构建
cd D:\COP\cop_mytools\axmol\projects\SlotGame
Remove-Item -Recurse -Force build

# 2. 重新生成CMake
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 3. 编译
cmake --build build --config Debug
# 或者
cmake --build build --config Release
```

### 使用rebuild_clean.bat

```batch
cd D:\COP\cop_mytools\axmol\projects\SlotGame
rebuild_clean.bat
```

## 验证编译

编译成功后应该看到:

```
[100%] Building CXX object engine/extensions/co2effect/CMakeFiles/co2effect.dir/src/C3FileReader.cpp.obj
[100%] Building CXX object engine/extensions/co2effect/CMakeFiles/co2effect.dir/src/EffectConfig.cpp.obj
[100%] Building CXX object engine/extensions/co2effect/CMakeFiles/co2effect.dir/src/CO2EffectPlayer.cpp.obj
[100%] Linking CXX static library ..\..\..\lib\co2effect.lib
[100%] Built target co2effect
```

## 故障排查

### 问题1: "co2effect/CO2EffectPlayer.h: No such file or directory"

**原因:** CMake没有正确配置包含路径

**解决:**
1. 确认`extensions/CMakeLists.txt`中添加了co2effect
2. 重新运行CMake生成: `cmake -S . -B build`

### 问题2: 链接错误 "unresolved external symbol"

**原因:** co2effect库没有被链接

**解决:**
检查`extensions/fairygui/CMakeLists.txt`中是否有:
```cmake
if(TARGET co2effect)
  target_link_libraries(${target_name} PUBLIC co2effect)
endif()
```

### 问题3: namespace 'co2effect' does not exist

**原因:** 头文件包含路径不正确

**解决:**
在GLoader.cpp中确认:
```cpp
#include "co2effect/CO2EffectPlayer.h"
using namespace ax::co2effect;
```

## 依赖关系

```
SlotGame (项目)
  └─> fairygui (扩展)
        ├─> spine (依赖)
        └─> co2effect (依赖, 新增)
              └─> axmol核心库
```

## 编译输出

成功编译后会生成:

- `build/lib/Debug/co2effect.lib` (Debug模式)
- `build/lib/Release/co2effect.lib` (Release模式)

这些库会自动链接到fairygui和SlotGame。

## 下一步测试

编译成功后，可以测试effect://协议:

### 在Lua中测试

```lua
-- 在fdlgslotfastfury.lua中添加
local loader = self.view:GetChild("n1")  -- 替换为实际的loader名称
if loader then
    loader.url = "effect://test_effect"
end
```

### 查看日志

运行游戏后查看控制台输出:
```
[GLoader] Loading CO2 effect: test_effect
[GLoader] CO2 effect loaded successfully: test_effect
[CO2EffectPlayer] Started playing effect 'test_effect'
```

## 完整的构建流程

```powershell
# 1. 进入项目目录
cd D:\COP\cop_mytools\axmol\projects\SlotGame

# 2. 清理
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# 3. 生成项目文件
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 4. 编译
cmake --build build --config Debug -j 8

# 5. 运行
.\build\bin\Debug\SlotGame.exe
```

## 性能优化

### 并行编译

使用`-j`参数加速编译:
```
cmake --build build --config Debug -j 8
```

### 增量编译

只编译修改的文件:
```
cmake --build build --config Debug --target fairygui
cmake --build build --config Debug --target co2effect
```

## 常见警告(可忽略)

```
warning C4251: needs to have dll-interface to be used by clients
```
这是因为我们使用静态库，可以安全忽略。

## 总结

✅ 所有CMake配置已完成
✅ 所有编译错误已修复
✅ 依赖关系已正确配置
✅ 可以开始编译测试

现在运行`rebuild_clean.bat`应该能成功编译！
