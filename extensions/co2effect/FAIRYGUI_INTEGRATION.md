# FairyGUI集成指南 - effect://协议

## 概述

已成功集成CO2特效系统到FairyGUI的GLoader组件中，支持通过`effect://`协议加载和播放特效。

## 修改内容

### 1. GLoader.cpp修改

**文件:** `extensions/fairygui/src/fairygui/GLoader.cpp`

**添加的代码:**

```cpp
// 文件头部添加
#include "co2effect/CO2EffectPlayer.h"

// loadExternal()函数中添加(在spine://协议之前)
void GLoader::loadExternal()
{
    // 处理effect://协议 (CO2特效系统)
    if (_url.compare(0, 9, "effect://") == 0)
    {
        std::string effectName = _url.substr(9);

        auto player = co2effect::CO2EffectPlayer::create();
        player->setResourcePath("c3/effect/");
        player->loadEffectConfig("ini/+3DEffect.ini");

        if (player->playEffect(effectName))
        {
            _contentStatus = 6;
            _content->setVisible(false);
            player->setAnchorPoint(Vec2(0.5f, 0.5f));
            _displayObject->addChild(player);
            player->setTag(9998);

            sourceSize.width = 200;
            sourceSize.height = 200;
            updateLayout();
        }
        return;
    }

    // ... 原有的spine://和其他协议处理
}
```

### 2. 状态码说明

- `_contentStatus = 4` - 外部图片
- `_contentStatus = 5` - Spine动画
- `_contentStatus = 6` - CO2特效 (新增)

### 3. Tag标识

- `9999` - Spine节点
- `9998` - CO2特效节点 (新增)

## 使用方法

### 在FairyGUI编辑器中

1. 添加GLoader组件
2. 设置URL为: `effect://特效名称`
3. 运行时会自动加载并播放特效

### 在Lua代码中

```lua
-- 获取GLoader组件
local loader = self.view:GetChild("effect_loader")

-- 方法1: 直接设置URL
loader.url = "effect://cjwp_beishukuang"

-- 方法2: 动态切换特效
local function playEffect(effectName)
    loader.url = "effect://" .. effectName
end

playEffect("effect1")
```

### 在C++代码中

```cpp
// 获取GLoader组件
auto loader = dynamic_cast<GLoader*>(view->getChild("effect_loader"));

// 设置URL
loader->setURL("effect://cjwp_beishukuang");
```

## 配置文件

### 1. 特效配置文件

**路径:** `Content/Resources/ini/+3DEffect.ini`

**格式:**
```ini
[cjwp_beishukuang]
Amount=3
EffectId0=1
TextureId0=1000
Asb0=5
Adb0=6
EffectId1=2
TextureId1=1001
Asb1=5
Adb1=6
EffectId2=3
TextureId2=1002
Asb2=5
Adb2=6
Delay=0
LoopTime=1
FrameInterval=33
LoopInterval=0
OffsetX=0
OffsetY=0
OffsetZ=0
```

### 2. C3文件结构

**路径:** `Content/Resources/c3/effect/`

```
c3/effect/
├── 1.c3
├── 2.c3
├── 3.c3
└── ...
```

## 路径配置

当前硬编码的路径:
- 特效配置: `ini/+3DEffect.ini`
- C3文件目录: `c3/effect/`

### 自定义路径

如果需要自定义路径，可以修改GLoader.cpp中的这两行:

```cpp
player->setResourcePath("c3/effect/");           // C3文件路径
player->loadEffectConfig("ini/+3DEffect.ini");  // 配置文件路径
```

或者创建全局配置:

```cpp
// 在游戏初始化时设置
co2effect::CO2EffectPlayer::setGlobalResourcePath("your/custom/path/");
co2effect::CO2EffectPlayer::setGlobalConfigPath("your/config.ini");
```

## 示例场景

### 老虎机倍数框特效

**Lua代码示例:**

```lua
-- fdlgslotfastfury.lua

local FDlgSlotFastFury = class("FDlgSlotFastFury", UIBase)

function FDlgSlotFastFury:OnEnable()
    -- 获取特效加载器
    self.effectLoader = self.view:GetChild("effect_loader")

    -- 播放倍数框特效
    self:PlayMultiplierEffect()
end

function FDlgSlotFastFury:PlayMultiplierEffect()
    -- 显示倍数框特效
    if self.effectLoader then
        self.effectLoader.url = "effect://cjwp_beishukuang"
        self.effectLoader.visible = true
    end
end

function FDlgSlotFastFury:HideMultiplierEffect()
    if self.effectLoader then
        self.effectLoader.visible = false
    end
end

function FDlgSlotFastFury:OnWinResult(multiplier)
    -- 根据倍数播放不同特效
    local effectName = "cjwp_beishukuang"

    if multiplier >= 100 then
        effectName = "cjwp_beishukuang_big"  -- 大倍数特效
    elseif multiplier >= 10 then
        effectName = "cjwp_beishukuang_medium"  -- 中倍数特效
    end

    self.effectLoader.url = "effect://" .. effectName
end

return FDlgSlotFastFury
```

### FairyGUI编辑器设置

1. 在编辑器中创建组件
2. 添加GLoader，命名为`effect_loader`
3. 设置默认URL: `effect://cjwp_beishukuang`
4. 调整大小和位置
5. 发布导出

## 注意事项

### 1. 资源预加载

特效会在第一次使用时加载，建议在场景初始化时预加载:

```lua
function Scene:PreloadEffects()
    local effects = {
        "cjwp_beishukuang",
        "cjwp_guangxiao",
        "cjwp_jiguang"
    }

    for _, name in ipairs(effects) do
        -- 创建隐藏的loader预加载
        local loader = fgui.GLoader.new()
        loader.url = "effect://" .. name
        loader.visible = false
        self.view:AddChild(loader)
    end
end
```

### 2. 内存管理

CO2特效会缓存帧数据，使用完毕后建议释放:

```lua
function Scene:OnDisable()
    -- 清理特效
    if self.effectLoader then
        self.effectLoader.url = ""  -- 清空URL会释放特效
    end
end
```

### 3. 性能优化

- 特效文件较大时，建议异步加载
- 避免同时播放过多特效
- 使用对象池复用GLoader

### 4. 调试日志

启用日志查看加载过程:

```
[GLoader] Loading CO2 effect: cjwp_beishukuang
[GLoader] CO2 effect loaded successfully: cjwp_beishukuang
[CO2EffectPlayer] Started playing effect 'cjwp_beishukuang', 3 frames, loop=1
```

## 故障排查

### 问题1: 特效不显示

**检查项:**
1. 配置文件路径是否正确: `ini/+3DEffect.ini`
2. C3文件是否存在: `c3/effect/1.c3`, `c3/effect/2.c3`...
3. 特效名称是否在配置文件中定义
4. 查看控制台日志

### 问题2: 编译错误

**解决方法:**
```cmake
# 确保CMakeLists.txt中链接了co2effect
target_link_libraries(fairygui PUBLIC co2effect)
```

### 问题3: 特效位置不对

**解决方法:**
调整GLoader的pivot和锚点:
```lua
loader.pivotX = 0.5
loader.pivotY = 0.5
```

## 扩展功能

### 添加特效完成回调

当前实现不支持回调，如需支持，可以修改:

```cpp
// 在GLoader.cpp中保存回调
player->playEffect(effectName, [this]() {
    // 特效播放完成
    AXLOG("[GLoader] Effect finished: %s", effectName.c_str());
    // 触发FairyGUI事件
});
```

### 自定义特效参数

可以通过URL参数传递:

```lua
-- effect://特效名?loop=5&speed=2.0
loader.url = "effect://cjwp_beishukuang?loop=5&speed=2.0"
```

然后在GLoader.cpp中解析参数。

## 完整示例项目

参考文件:
- `projects/SlotGame/Content/src/app/views/fdlgslotfastfury.lua`
- `extensions/co2effect/example/CO2EffectTest.cpp`

## 总结

✅ **已完成:**
- effect://协议支持
- 自动加载和播放
- FairyGUI完美集成

⚠️ **待优化:**
- 3D网格渲染
- 粒子系统
- 回调支持
- 异步加载

现在可以在FairyGUI中使用`effect://特效名称`来播放CO2特效了！
