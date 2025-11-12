# CMake配置修复说明

## 问题描述

CMake错误:
```
The plain signature for target_link_libraries has already been used with
the target "fairygui". All uses of target_link_libraries with a target
must be either all-keyword or all-plain.
```

## 原因

CMake不允许在同一个target上混用plain signature和keyword signature:

```cmake
# Plain signature (在setup_ax_extension_config中使用)
target_link_libraries(fairygui ${_AX_CORE_LIB})

# Keyword signature (不能混用)
target_link_libraries(fairygui PUBLIC co2effect)
```

## 解决方案

所有`target_link_libraries`调用都使用plain signature。

### 修改1: extensions/CMakeLists.txt

```cmake
if(AX_ENABLE_EXT_FAIRYGUI)
  # Add co2effect extension
  add_subdirectory(co2effect)

  add_subdirectory(fairygui)

  if(BUILD_SHARED_LIBS)
    target_link_libraries(fairygui spine co2effect)  # Plain signature
  else()
    target_include_directories(fairygui
      PUBLIC ${CMAKE_CURRENT_LIST_DIR}/spine/runtime/include
      PUBLIC ${CMAKE_CURRENT_LIST_DIR}/spine/src
      PUBLIC ${CMAKE_CURRENT_LIST_DIR}/co2effect/include
    )
    # Link co2effect library in static mode
    target_link_libraries(fairygui co2effect)  # Plain signature
  endif()
endif()
```

### 修改2: extensions/fairygui/CMakeLists.txt

删除了重复的链接配置:

```cmake
setup_ax_extension_config(${target_name})

# 不再需要这段代码(已在extensions/CMakeLists.txt中处理)
# if(TARGET co2effect)
#   target_link_libraries(${target_name} PUBLIC co2effect)
# endif()
```

## 验证修复

现在重新运行CMake应该成功:

```powershell
cd D:\COP\cop_mytools\axmol\projects\SlotGame
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

期望输出:
```
-- Enabled axmol extensions:...,co2effect,fairygui,...
-- Configuring done
-- Generating done
```

## 依赖关系图

```
fairygui target
  ├─> ${_AX_CORE_LIB}  (通过setup_ax_extension_config)
  ├─> spine             (通过extensions/CMakeLists.txt)
  └─> co2effect         (通过extensions/CMakeLists.txt)
        └─> ${_AX_CORE_LIB}  (通过setup_ax_extension_config)
```

## 完整的编译流程

```powershell
# 1. 清理
cd D:\COP\cop_mytools\axmol\projects\SlotGame
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# 2. 生成项目
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 3. 编译
cmake --build build --config Debug

# 4. 验证co2effect库
dir build\lib\Debug\co2effect.lib
```

应该看到:
```
co2effect.lib
fairygui.lib
spine.lib
```

## 常见问题

### Q: 为什么要在两个地方配置？

A:
- `extensions/CMakeLists.txt` - 统一管理扩展之间的依赖
- `extensions/fairygui/CMakeLists.txt` - 只管理fairygui内部的配置

### Q: 静态链接和动态链接的区别？

A:
- **BUILD_SHARED_LIBS=ON** (动态链接): 只需要`target_link_libraries`
- **BUILD_SHARED_LIBS=OFF** (静态链接,默认): 需要同时设置头文件路径和链接库

### Q: 为什么setup_ax_extension_config使用plain signature?

A: 这是axmol的约定，所有扩展都统一使用plain signature以避免冲突。

## 测试编译

修复后运行完整编译测试:

```powershell
.\rebuild_clean.bat
```

或者:

```powershell
pwsh.exe -ExecutionPolicy Bypass -File "..\..\tools\cmdline\axmol.ps1" build -p win32
```

成功标志:
```
[100%] Building CXX object engine/extensions/co2effect/CMakeFiles/co2effect.dir/src/CO2EffectPlayer.cpp.obj
[100%] Linking CXX static library ..\..\..\lib\co2effect.lib
[100%] Built target co2effect
...
[100%] Built target fairygui
...
[100%] Built target SlotGame
```

## 总结

✅ 修复了CMake签名混用错误
✅ 保持了plain signature的一致性
✅ 正确配置了静态和动态链接模式
✅ 可以正常编译co2effect和fairygui

现在可以重新编译了！
