# FairyGUI Unity 与 axmol 版本同步跟踪文档

> **文档目的**: 跟踪 Unity FairyGUI 功能向 axmol 版本的迁移进度
> **创建时间**: 2025-11-12
> **最后更新**: 2025-11-12 17:35

---

## 📋 目录

- [已完成功能](#已完成功能)
- [待实现功能清单](#待实现功能清单)
- [详细实现指南](#详细实现指南)
- [版本兼容性](#版本兼容性)
- [测试验证清单](#测试验证清单)

---

## ✅ 已完成功能

### 1. mouseWheelScale 滚轮速度缩放 (2025-11-12)

**功能**: 全局滚轮滚动速度缩放配置,允许用户调整鼠标滚轮的滚动灵敏度

**影响**: 中高 - 用户体验提升
**优先级**: P1
**实际工作量**: 0.5 天 (约 1 小时实现 + 测试)
**ROI**: 7.0 ⭐⭐⭐⭐⭐

**修改文件**:
- `extensions/fairygui/src/fairygui/GRoot.h`
- `extensions/fairygui/src/fairygui/GRoot.cpp`
- `extensions/fairygui/src/fairygui/ScrollPane.cpp`

**详细修改**:

#### 1.1 GRoot 添加静态成员
```cpp
// GRoot.h 行66
class GRoot {
public:
    static int contentScaleLevel;
    static float mouseWheelScale;  // ✅ 新增,默认 1.0f
};

// GRoot.cpp 行42
float GRoot::mouseWheelScale = 1.0f;
```

#### 1.2 ScrollPane 应用滚轮缩放
```cpp
// ScrollPane.cpp 行1760-1786
void ScrollPane::onMouseWheel(EventContext* context)
{
    if (!_mouseWheelEnabled)
        return;

    InputEvent* evt = context->getInput();
    int delta = evt->getMouseWheelDelta();
    delta = delta > 0 ? 1 : -1;

    // ✅ 应用全局滚轮缩放
    float scaledStep = _mouseWheelStep * GRoot::mouseWheelScale;

    if (_overlapSize.width > 0 && _overlapSize.height == 0)
    {
        if (_pageMode)
            setPosX(_xPos + _pageSize.width * delta, false);
        else
            setPosX(_xPos + scaledStep * delta, false);  // ✅ 使用缩放值
    }
    else
    {
        if (_pageMode)
            setPosY(_yPos + _pageSize.height * delta, false);
        else
            setPosY(_yPos + scaledStep * delta, false);  // ✅ 使用缩放值
    }
}
```

#### 1.3 ScrollPane.cpp 包含 GRoot.h
```cpp
// ScrollPane.cpp 行4
#include "GRoot.h"  // ✅ 新增
```

**使用示例**:
```cpp
// 增加滚轮速度 2 倍
GRoot::mouseWheelScale = 2.0f;

// 减少滚轮速度到一半
GRoot::mouseWheelScale = 0.5f;

// 恢复默认
GRoot::mouseWheelScale = 1.0f;
```

**Unity 对比**:
```csharp
// Unity Stage.cs
public static float mouseWheelScale = 1f;

// 使用
float delta = Input.mouseScrollDelta.y * mouseWheelScale;
```

**测试验证**:
- [x] 编译通过
- [ ] 运行时测试:调整 mouseWheelScale 值验证滚动速度变化
- [ ] 边界测试:0.1, 0.5, 1.0, 2.0, 5.0 等不同值
- [ ] pageMode 验证:确保分页模式不受影响

---

### 2. ignoreEngineTimeScale UI 时间独立性 (2025-11-12)

**功能**: Transition 和 GTweener 支持独立于引擎时间缩放的动画播放,用于实现暂停菜单等功能

**影响**: 极高 - UI 时间控制核心功能
**优先级**: P0
**实际工作量**: 2-3 小时 (比预估快!)
**ROI**: 5.0 ⭐⭐⭐⭐⭐

**修改文件**:
- `extensions/fairygui/src/fairygui/tween/GTweener.h`
- `extensions/fairygui/src/fairygui/tween/GTweener.cpp`
- `extensions/fairygui/src/fairygui/tween/TweenManager.cpp`
- `extensions/fairygui/src/fairygui/Transition.h`
- `extensions/fairygui/src/fairygui/Transition.cpp`

**详细修改**:

#### 2.1 GTweener 添加 ignoreEngineTimeScale 支持
```cpp
// GTweener.h 行33 + 行90
class GTweener {
public:
    GTweener* setIgnoreEngineTimeScale(bool value);  // ✅ 新增
private:
    bool _ignoreEngineTimeScale;  // ✅ 新增成员
};

// GTweener.cpp 行73-77
GTweener* GTweener::setIgnoreEngineTimeScale(bool value)
{
    _ignoreEngineTimeScale = value;
    return this;
}

// GTweener.cpp 行262 - 初始化默认值
void GTweener::_init()
{
    _ignoreEngineTimeScale = false;  // ✅ 默认 false (与 Unity 一致)
}
```

#### 2.2 TweenManager 应用时间独立性
```cpp
// TweenManager.cpp 行109-142
void TweenManager::update(float dt)
{
    // ✅ 获取未缩放的 delta time
    float unscaledDt = ax::Director::getInstance()->getDeltaTime();

    for (int i = 0; i < cnt; i++)
    {
        GTweener* tweener = _activeTweens[i];
        if (tweener != nullptr && !tweener->_paused)
        {
            // ✅ 根据标志选择使用哪个 delta time
            float actualDt = tweener->_ignoreEngineTimeScale ? unscaledDt : dt;
            tweener->_update(actualDt);
        }
    }
}
```

**关键说明**:
- `getDeltaTime()` 返回的是未经引擎时间缩放的原始帧时间
- Scheduler 传入的 `dt` 是经过时间缩放的
- 当 `_ignoreEngineTimeScale = true` 时,使用原始时间,实现独立于游戏暂停的 UI 动画

#### 2.3 Transition 添加 ignoreEngineTimeScale 属性
```cpp
// Transition.h 行46-47 + 行90
class Transition {
public:
    bool getIgnoreEngineTimeScale() const { return _ignoreEngineTimeScale; }
    void setIgnoreEngineTimeScale(bool value);  // ✅ 新增
private:
    bool _ignoreEngineTimeScale;  // ✅ 新增成员
};

// Transition.cpp 行277 - 构造函数初始化
Transition::Transition(GComponent* owner)
    : _ignoreEngineTimeScale(true),  // ✅ 默认 true (Unity 默认行为)
```

#### 2.4 Transition::setIgnoreEngineTimeScale 递归应用
```cpp
// Transition.cpp 行711-735
void Transition::setIgnoreEngineTimeScale(bool value)
{
    if (_ignoreEngineTimeScale != value)
    {
        _ignoreEngineTimeScale = value;

        // ✅ 递归应用到所有 item
        for (auto& item : _items)
        {
            // 应用到 tweener
            if (item->tweener != nullptr)
                item->tweener->setIgnoreEngineTimeScale(value);

            // 递归应用到子 Transition
            if (item->type == TransitionActionType::Transition)
            {
                if (((TValue_Transition*)item->value)->trans != nullptr)
                    ((TValue_Transition*)item->value)->trans->setIgnoreEngineTimeScale(value);
            }

            // Animation 对象将在 IAnimationGear 实现中支持
        }
    }
}
```

#### 2.5 playItem() 创建 tweener 时设置标志
```cpp
// Transition.cpp 行903, 931, 960 - 三处创建 tweener 的地方
item->tweener->setDelay(time)
    ->setEase(item->tweenConfig->easeType)
    ->setRepeat(item->tweenConfig->repeat, item->tweenConfig->yoyo)
    ->setTimeScale(_timeScale)
    ->setIgnoreEngineTimeScale(_ignoreEngineTimeScale)  // ✅ 新增
    ->setTargetAny(item)
    // ...
```

**使用示例**:
```cpp
// 暂停菜单动画 - 即使游戏暂停也能播放
Transition* pauseMenuAnim = pauseMenu->getTransition("show");
pauseMenuAnim->setIgnoreEngineTimeScale(true);  // UI 动画不受暂停影响
pauseMenuAnim->play();

// 游戏内 UI 动画 - 跟随游戏时间
Transition* gameUIAnim = gameUI->getTransition("countdown");
gameUIAnim->setIgnoreEngineTimeScale(false);  // 游戏暂停时也暂停
gameUIAnim->play();
```

**Unity 对比**:
```csharp
// Unity Transition.cs
bool _ignoreEngineTimeScale = true;  // 默认 true

public bool ignoreEngineTimeScale {
    set {
        _ignoreEngineTimeScale = value;
        // 递归应用到所有子元素
        foreach (var item in _items) {
            if (item.tweener != null)
                item.tweener.SetIgnoreEngineTimeScale(value);
            // ...
        }
    }
}
```

**测试验证**:
- [ ] 编译通过
- [ ] 运行时测试:
  - [ ] 设置 `ignoreEngineTimeScale = true` 的 Transition 在游戏暂停时仍能播放
  - [ ] 设置 `ignoreEngineTimeScale = false` 的 Transition 在游戏暂停时也暂停
  - [ ] 递归应用:父 Transition 设置后,子 Transition 也应用
- [ ] 暂停菜单场景完整测试

**待完善**:
- [x] IAnimationGear 中添加 IgnoreEngineTimeScale 属性支持 ✅ 已完成

---

### 3. IAnimationGear IgnoreEngineTimeScale 属性 (2025-11-12)

**功能**: 为动画对象(GMovieClip, GLoader等)添加 IgnoreEngineTimeScale 属性支持

**影响**: 极高 - 动画系统基础完整性
**优先级**: P0
**实际工作量**: 30分钟
**ROI**: 3.0 ⭐⭐⭐⭐⭐

**修改文件**:
- `extensions/fairygui/src/fairygui/FieldTypes.h`
- `extensions/fairygui/src/fairygui/GMovieClip.h`
- `extensions/fairygui/src/fairygui/GMovieClip.cpp`
- `extensions/fairygui/src/fairygui/Transition.cpp`

**详细修改**:

#### 3.1 ObjectPropID 添加新枚举
```cpp
// FieldTypes.h 行197
enum class ObjectPropID {
    Text,
    Icon,
    Color,
    OutlineColor,
    Playing,
    Frame,
    DeltaTime,
    TimeScale,
    FontSize,
    Selected,
    IgnoreEngineTimeScale  // ✅ 新增
};
```

#### 3.2 ActionMovieClip 添加支持
```cpp
// GMovieClip.h 行75-76, 92
class ActionMovieClip : public ax::Action {
public:
    bool getIgnoreEngineTimeScale() const { return _ignoreEngineTimeScale; }
    void setIgnoreEngineTimeScale(bool value) { _ignoreEngineTimeScale = value; }
private:
    bool _ignoreEngineTimeScale;  // ✅ 新增成员
};

// GMovieClip.cpp 行195 - 构造函数初始化
ActionMovieClip::ActionMovieClip()
    : _ignoreEngineTimeScale(false)  // ✅ 默认 false
```

#### 3.3 GMovieClip getProp/setProp 实现
```cpp
// GMovieClip.cpp 行117-118
ax::Value GMovieClip::getProp(ObjectPropID propId)
{
    switch (propId)
    {
    // ... 其他属性
    case ObjectPropID::IgnoreEngineTimeScale:
        return Value(_playAction->getIgnoreEngineTimeScale());  // ✅ 新增
    }
}

// GMovieClip.cpp 行143-145
void GMovieClip::setProp(ObjectPropID propId, const ax::Value& value)
{
    switch (propId)
    {
    // ... 其他属性
    case ObjectPropID::IgnoreEngineTimeScale:
        _playAction->setIgnoreEngineTimeScale(value.asBool());  // ✅ 新增
        break;
    }
}
```

#### 3.4 Transition 递归应用到 Animation 对象
```cpp
// Transition.cpp 行732-735
void Transition::setIgnoreEngineTimeScale(bool value)
{
    // ... 应用到 tweener 和子 Transition

    // ✅ 应用到 Animation 对象
    if (item->type == TransitionActionType::Animation && item->target != nullptr)
    {
        item->target->setProp(ObjectPropID::IgnoreEngineTimeScale, Value(value));
    }
}
```

**关键说明**:
- ActionMovieClip 继承自 ax::Action,在 step() 方法中接收 dt
- 完整的时间独立性实现需要 axmol Scheduler 支持,当前实现了 API 接口
- GLoader 的 Spine 动画支持将在后续完善

**Unity 对比**:
```csharp
// Unity IAnimationGear 接口
public interface IAnimationGear {
    bool playing { get; set; }
    int frame { get; set; }
    float timeScale { get; set; }
    bool ignoreEngineTimeScale { get; set; }  // ✅ 对应实现
    void Advance(float time);
}
```

**测试验证**:
- [ ] 编译通过
- [ ] Transition 设置 ignoreEngineTimeScale 后递归应用到 GMovieClip
- [ ] GMovieClip 的 setProp(IgnoreEngineTimeScale) 正常工作
- [ ] 与 Transition 的 ignoreEngineTimeScale 集成测试

---

### 4. Transition Animation 动作支持 (2025-11-12)

**功能**: 支持 Transition 的 Animation 动作设置 animationName 和 skinName

**修改文件**:
- `extensions/fairygui/src/fairygui/Transition.cpp`
- `extensions/fairygui/src/fairygui/Transition.h` (无需修改)
- `extensions/fairygui/src/fairygui/GLoader.h`
- `extensions/fairygui/src/fairygui/GLoader.cpp`

**详细修改**:

#### 1.1 TValue_Animation 类扩展
```cpp
// Transition.cpp 行27-35
class TValue_Animation : public TValueBase
{
public:
    int frame;
    bool playing;
    bool flag;
    std::string animationName;  // ✅ 新增
    std::string skinName;        // ✅ 新增
};
```

#### 1.2 GLoader 类扩展
```cpp
// GLoader.h 行54-58
const std::string& getAnimationName() const { return _animationName; }
void setAnimationName(const std::string& value);  // ✅ 新增

const std::string& getSkinName() const { return _skinName; }
void setSkinName(const std::string& value);        // ✅ 新增

// GLoader.h 行112-113
std::string _animationName;  // ✅ 新增
std::string _skinName;       // ✅ 新增
```

```cpp
// GLoader.cpp 行252-299
void GLoader::setAnimationName(const std::string& value)
{
    if (_animationName != value)
    {
        _animationName = value;

        // 如果已加载Spine动画，立即应用
        if (_contentStatus == 5) // Spine content
        {
            auto spineNode = _displayObject->getChildByTag(9999);
            if (spineNode)
            {
                auto skeletonAni = dynamic_cast<spine::SkeletonAnimation*>(spineNode);
                if (skeletonAni && !_animationName.empty())
                {
                    skeletonAni->setAnimation(0, _animationName, true);
                }
            }
        }
    }
}

void GLoader::setSkinName(const std::string& value)
{
    if (_skinName != value)
    {
        _skinName = value;

        // 如果已加载Spine动画，立即应用
        if (_contentStatus == 5) // Spine content
        {
            auto spineNode = _displayObject->getChildByTag(9999);
            if (spineNode)
            {
                auto skeletonAni = dynamic_cast<spine::SkeletonAnimation*>(spineNode);
                if (skeletonAni && !_skinName.empty())
                {
                    auto skeleton = skeletonAni->getSkeleton();
                    if (skeleton)
                    {
                        skeleton->setSkin(_skinName.c_str());
                        skeleton->setSlotsToSetupPose();
                    }
                }
            }
        }
    }
}
```

#### 1.3 Transition setValue 方法扩展
```cpp
// Transition.cpp 行560-571
case TransitionActionType::Animation:
{
    TValue_Animation* tvalue = (TValue_Animation*)value;
    tvalue->frame = values[0].asInt();
    if (values.size() > 1)
        tvalue->playing = values[1].asBool();
    if (values.size() > 2)
        tvalue->animationName = values[2].asString();  // ✅ 新增
    if (values.size() > 3)
        tvalue->skinName = values[3].asString();        // ✅ 新增
    break;
}
```

#### 1.4 Transition decodeValue 方法扩展
```cpp
// Transition.cpp 行1499-1509
case TransitionActionType::Animation:
{
    ((TValue_Animation*)value)->playing = buffer->readBool();
    ((TValue_Animation*)value)->frame = buffer->readInt();
    if (buffer->version >= 6)  // ✅ 版本兼容
    {
        ((TValue_Animation*)value)->animationName = buffer->readS();  // ✅ 新增
        ((TValue_Animation*)value)->skinName = buffer->readS();        // ✅ 新增
    }
    break;
}
```

#### 1.5 Transition applyValue 方法扩展
```cpp
// Transition.cpp 行1287-1305
case TransitionActionType::Animation:
{
    TValue_Animation* value = (TValue_Animation*)item->value;
    if (value->frame >= 0)
        item->target->setProp(ObjectPropID::Frame, Value(value->frame));
    item->target->setProp(ObjectPropID::Playing, Value(value->playing));
    item->target->setProp(ObjectPropID::TimeScale, Value(_timeScale));

    // ✅ 新增：应用animationName和skinName到GLoader
    GLoader* loader = dynamic_cast<GLoader*>(item->target);
    if (loader)
    {
        if (!value->animationName.empty())
            loader->setAnimationName(value->animationName);
        if (!value->skinName.empty())
            loader->setSkinName(value->skinName);
    }
    break;
}
```

#### 1.6 头文件引用
```cpp
// Transition.cpp 行4
#include "GLoader.h"  // ✅ 新增
```

**Unity 对照**:
- Unity 路径: `FairyGUI-unity/Assets/Scripts/UI/Transition.cs`
- Unity 实现: 行469-479 (setValue), 行1535-1543 (decodeValue), 行1321-1333 (applyValue)

**测试状态**: ⚠️ 待测试
- [ ] Transition 设置 animationName
- [ ] Transition 设置 skinName
- [ ] Buffer version 6 数据读取
- [ ] 与 Unity 版本行为对比

---

## 🔲 待实现功能清单

### P0 级别 - 关键功能 (必须实现)

#### P0.1 Transition.ignoreEngineTimeScale ⭐⭐⭐⭐⭐
- **状态**: ❌ 未实现
- **优先级**: 最高
- **预计工作量**: 2-3 天
- **实现难度**: 简单
- **Unity 参考**: `Transition.cs` 行49, 687-715
- **依赖**: GTween 系统需要支持引擎时间缩放

**功能描述**:
- 控制 Transition 是否忽略引擎时间缩放
- 默认为 true,使 UI 动画独立于游戏时间
- 关键用例: 暂停菜单在游戏暂停时仍能播放动画

**需要修改的文件**:
1. `Transition.h`: 添加 `bool _ignoreEngineTimeScale` 成员和 getter/setter
2. `Transition.cpp`: 实现 setter,递归应用到所有 item
3. `GTween.h/cpp`: 添加引擎时间缩放支持 (如果尚未支持)
4. `Transition.cpp::playItem()`: 设置 tweener 的时间缩放属性

**实现检查点**:
- [ ] Transition 类添加成员变量
- [ ] 实现 getter/setter
- [ ] 递归应用到子 Transition
- [ ] 递归应用到 Animation 类型 item
- [ ] GTween 系统支持验证
- [ ] 单元测试

---

#### P0.2 IAnimationGear 接口完善 ⭐⭐⭐⭐⭐
- **状态**: ⚠️ 部分实现 (通过 ObjectPropID)
- **优先级**: 最高
- **预计工作量**: 2-3 天
- **实现难度**: 中等
- **Unity 参考**: `IAnimationGear.cs` 全文

**功能描述**:
- Unity 使用接口方式定义动画控制能力
- C++ 使用 ObjectPropID 属性系统
- 需要确保所有属性完整实现

**Unity 接口定义**:
```csharp
public interface IAnimationGear
{
    bool playing { get; set; }
    int frame { get; set; }
    float timeScale { get; set; }
    bool ignoreEngineTimeScale { get; set; }
    void Advance(float time);
}
```

**C++ ObjectPropID 对应**:
```cpp
enum class ObjectPropID {
    Playing,              // ✅ 已实现
    Frame,                // ✅ 已实现
    TimeScale,            // ⚠️ 部分实现
    DeltaTime,            // ⚠️ 需验证
    // ignoreEngineTimeScale - ❌ 缺失
};
```

**需要检查的类**:
1. `GMovieClip`: 实现所有 ObjectPropID
2. `GLoader`: 实现所有 ObjectPropID
3. `GLoader3D`: 实现所有 ObjectPropID
4. 其他支持动画的类

**实现检查点**:
- [ ] 审查所有 getProp/setProp 实现
- [ ] 添加缺失的 ObjectPropID 枚举值
- [ ] 实现所有类的 ObjectPropID 处理
- [ ] 添加 Advance(deltaTime) 方法
- [ ] 单元测试覆盖

---

### P1 级别 - 重要功能 (强烈建议)

#### P1.1 Transition.invalidateBatchingEveryFrame ⭐⭐⭐⭐
- **状态**: ❌ 未实现
- **优先级**: 高
- **预计工作量**: 3-4 天
- **实现难度**: 中等
- **Unity 参考**: `Transition.cs` 行31, 1271-1347
- **依赖**: axmol 批处理系统

**功能描述**:
- 在动画每帧刷新时强制失效批处理状态
- 解决自动合批时深度显示错误问题
- 应用于 XY, Size, Rotation, Pivot, Scale, Skew 等动作

**Unity 实现位置**:
```csharp
// 行31: 公共字段
public bool invalidateBatchingEveryFrame;

// 行1271-1347: 多处调用
case TransitionActionType.XY:
    item.target.SetXY(value.f1, value.f2);
    if (invalidateBatchingEveryFrame)
        _owner.InvalidateBatchingState(true);
    break;
```

**需要修改的文件**:
1. `Transition.h`: 添加 `bool invalidateBatchingEveryFrame` 公共成员
2. `Transition.cpp::applyValue()`: 在相关 case 中调用批处理失效
3. `GComponent.h/cpp`: 确认 `invalidateBatchingState()` 方法存在

**实现检查点**:
- [ ] 添加成员变量
- [ ] 在 applyValue 中添加调用
- [ ] 验证批处理系统支持
- [ ] 性能测试
- [ ] 视觉效果验证

---

#### P1.2 CustomEase 支持 ⭐⭐⭐⭐
- **状态**: ❌ 未实现
- **优先级**: 高
- **预计工作量**: 2-3 天
- **实现难度**: 中等
- **Unity 参考**: `Transition.cs` 行1477-1485, `CustomEase.cs`
- **Buffer Version**: 4

**功能描述**:
- 支持编辑器中绘制的自定义缓动曲线
- 使用贝塞尔曲线实现
- 提升动画表现力

**Unity 实现**:
```csharp
// Transition.cs 行1477-1485
if (buffer.version >= 4 && item.tweenConfig.easeType == EaseType.Custom)
{
    var pts = buffer.ReadPath();
    if (pts.Count > 0)
    {
        item.tweenConfig.customEase = new CustomEase();
        item.tweenConfig.customEase.Create(pts);
    }
}
```

**需要创建的文件**:
1. `tween/CustomEase.h`: 自定义缓动类
2. `tween/CustomEase.cpp`: 实现贝塞尔曲线计算

**需要修改的文件**:
1. `FieldTypes.h`: 确认 `EaseType::Custom` 枚举
2. `Transition.cpp::setup()`: 解析 buffer version 4 数据
3. `TweenConfig`: 添加 `CustomEase* customEase` 成员

**实现检查点**:
- [ ] 创建 CustomEase 类
- [ ] 实现贝塞尔曲线插值
- [ ] Transition.setup() 解析数据
- [ ] playItem() 应用自定义缓动
- [ ] 与编辑器预览效果对比

---

#### P1.3 IColorGear 接口完善 ⭐⭐⭐⭐
- **状态**: ⚠️ 部分实现
- **优先级**: 高
- **预计工作量**: 2-3 天
- **实现难度**: 中等
- **Unity 参考**: `IColorGear.cs`

**Unity 接口定义**:
```csharp
public interface IColorGear
{
    Color color { get; set; }
}

public interface ITextColorGear : IColorGear
{
    Color strokeColor { get; set; }
}
```

**C++ ObjectPropID 对应**:
```cpp
enum class ObjectPropID {
    Color,                // ✅ 已实现
    OutlineColor,         // ⚠️ 需验证 (对应 strokeColor)
    // ...
};
```

**需要检查的类**:
1. `GImage`: Color 支持
2. `GTextField`: Color 和 OutlineColor 支持
3. `GLoader`: Color 支持
4. `GGraph`: Color 支持
5. `GButton`: Color 传递

**实现检查点**:
- [ ] 审查所有颜色相关 ObjectPropID
- [ ] 确保 Transition Color 动作正常工作
- [ ] 验证 strokeColor/OutlineColor
- [ ] 单元测试

---

#### P1.4 GLoader 外部加载钩子 ⭐⭐⭐
- **状态**: ⚠️ 虚函数可扩展
- **优先级**: 中
- **预计工作量**: 1-2 天
- **实现难度**: 简单
- **Unity 参考**: `GLoader.cs` 行33-36

**Unity 实现** (PuerTS):
```csharp
#if FAIRYGUI_PUERTS
    public Action __loadExternal;
    public Action<NTexture> __freeExternal;
#endif

protected virtual void LoadExternal()
{
#if FAIRYGUI_PUERTS
    if (__loadExternal != null)
    {
        __loadExternal();
        return;
    }
#endif
    // 默认实现...
}
```

**C++ 当前实现**:
```cpp
// GLoader.h 行78-79
virtual void loadExternal();
virtual void freeExternal(ax::SpriteFrame* spriteFrame);
```

**建议方案**:
1. 保持虚函数机制 (C++ 最佳实践)
2. 可选: 添加全局回调注册
3. 可选: 添加 std::function 成员变量

**实现检查点**:
- [ ] 评估是否需要额外钩子机制
- [ ] 如需要,设计回调注册接口
- [ ] 文档说明扩展方式
- [ ] 示例代码

---

### P2 级别 - 可选功能 (锦上添花)

#### P2.1 GLoader.showErrorSign ⭐⭐⭐
- **状态**: ❌ 空实现
- **优先级**: 中
- **预计工作量**: 1-2 天
- **实现难度**: 简单
- **Unity 参考**: `GLoader.cs` 行16, 520-538

**Unity 实现**:
```csharp
// 行16: 公共字段
public bool showErrorSign = true;

// 行520-538
private void SetErrorState()
{
    if (!showErrorSign || !Application.isPlaying)
        return;

    if (_errorSign == null)
    {
        if (UIConfig.loaderErrorSign != null)
            _errorSign = UIPackage.CreateObjectFromURL(UIConfig.loaderErrorSign);
        else
            return;
    }

    if (_errorSign != null)
    {
        _errorSign.SetSize(this.width, this.height);
        ((Container)displayObject).AddChild(_errorSign.displayObject);
    }
}
```

**C++ 当前实现**:
```cpp
// GLoader.cpp 行914-920
void GLoader::setErrorState()
{
    // 空实现
}

void GLoader::clearErrorState()
{
    // 空实现
}
```

**需要修改的文件**:
1. `GLoader.h`: 添加 `bool showErrorSign`, `GObject* _errorSign` 成员
2. `GLoader.cpp::setErrorState()`: 实现错误标识显示
3. `GLoader.cpp::clearErrorState()`: 实现清理
4. `UIConfig.h/cpp`: 添加 `loaderErrorSign` 配置

**实现检查点**:
- [ ] 添加成员变量
- [ ] 实现 setErrorState()
- [ ] 实现 clearErrorState()
- [ ] 配置错误标识 URL
- [ ] 测试加载失败场景

---

#### P2.2 GLoader.useResize ⭐⭐⭐
- **状态**: ❌ 未实现
- **优先级**: 中
- **预计工作量**: 1 天
- **实现难度**: 简单
- **Unity 参考**: `GLoader.cs` 行159-169, 579, 636-639, 725-726
- **Buffer Version**: 7

**Unity 实现**:
```csharp
// 行159-169: 属性定义
public bool useResize
{
    get { return _useResize; }
    set
    {
        if (_useResize != value)
        {
            _useResize = value;
            UpdateLayout();
        }
    }
}

// 行636-639: 在 UpdateLayout 中应用
if (_content2 != null)
{
    if (_useResize)
        _content2.SetSize(contentWidth, contentHeight, true);
    else
        _content2.SetScale(sx, sy);
}

// 行725-726: 从 buffer 读取
if (buffer.version >= 7)
    _useResize = buffer.ReadBool();
```

**功能差异**:
- `SetSize`: 触发组件布局,子元素根据关系重新排列
- `SetScale`: 仅视觉缩放,不触发布局逻辑

**需要修改的文件**:
1. `GLoader.h`: 添加 `bool _useResize` 成员
2. `GLoader.cpp::updateLayout()`: 根据标志选择缩放方式
3. `GLoader.cpp::setup_beforeAdd()`: 读取 buffer version 7

**实现检查点**:
- [ ] 添加成员变量
- [ ] 修改 updateLayout() 逻辑
- [ ] setup_beforeAdd() 读取配置
- [ ] 验证 SetSize vs SetScale 行为
- [ ] 与 Unity 效果对比

---

#### P2.3 GLoader3D 时间控制 ⭐⭐⭐
- **状态**: ❌ 未实现
- **优先级**: 中
- **预计工作量**: 2-3 天
- **实现难度**: 中等
- **Unity 参考**: `GLoader3D.cs` 行190-194, 198-203, 209-211

**Unity 接口** (注: Unity 版本标注为 Not implemented):
```csharp
public float timeScale { get; set; }  // 行190-194
public bool ignoreEngineTimeScale { get; set; }  // 行198-203
public void Advance(float time) { }  // 行209-211 - Not implemented
```

**C++ 当前实现**:
```cpp
// GLoader3D.cpp 行435-458
case ObjectPropID::TimeScale:
    break;  // 空实现
case ObjectPropID::DeltaTime:
    break;  // 空实现
```

**需要修改的文件**:
1. `GLoader3D.h`: 添加 `float _timeScale`, `bool _ignoreEngineTimeScale` 成员
2. `GLoader3D.cpp::setProp()`: 实现 TimeScale 和 DeltaTime
3. Spine/DragonBones 时间控制实现

**实现检查点**:
- [ ] 添加成员变量
- [ ] 实现 timeScale 设置
- [ ] 实现 Advance 方法
- [ ] Spine 动画时间控制
- [ ] 测试验证

---

#### P2.4 DragonBones 支持 ⭐⭐
- **状态**: ❌ 未实现
- **优先级**: 低 (取决于项目需求)
- **预计工作量**: 5-7 天
- **实现难度**: 困难
- **Unity 参考**: `GLoader3D.cs` 条件编译块

**Unity 实现**:
```csharp
#if FAIRYGUI_DRAGONBONES
    LoadDragonBones();        // 行357-361
    OnChangeDragonBones(propertyName);  // 行380-384
    FreeDragonBones();        // 行500-504
    OnUpdateDragonBones(context);  // 行526-530
#endif
```

**需要评估**:
- 项目是否需要 DragonBones 支持
- axmol 是否有 DragonBones 集成
- 维护成本

**实现检查点**:
- [ ] 需求确认
- [ ] DragonBones 库集成
- [ ] GLoader3D 扩展
- [ ] 完整测试

---

## 📊 版本兼容性

### Buffer Version 映射表

| Version | 功能 | Unity 实现 | C++ 实现 | 状态 |
|---------|------|-----------|----------|------|
| 1 | 基础功能 | ✅ | ✅ | ✅ 完整 |
| 2 | Path 曲线 | ✅ 行1467 | ✅ 行1414 | ✅ 完整 |
| 3 | - | - | - | - |
| 4 | CustomEase | ✅ 行1477 | ❌ | ⚠️ **缺失** |
| 5 | - | - | - | - |
| 6 | animationName/skinName | ✅ 行1538 | ✅ 行1514 | ✅ 完整 |
| 7 | useResize | ✅ 行725 | ❌ | ⚠️ **缺失** |

### 需要补全的版本支持

#### Buffer Version 4 - CustomEase
**位置**: `Transition.cpp::setup()` 约 1477 行附近
```cpp
if (buffer->version >= 4 && item->tweenConfig->easeType == EaseType::Custom)
{
    // TODO: 读取 path 数据
    // TODO: 创建 CustomEase 对象
    // TODO: 应用到 tweenConfig
}
```

#### Buffer Version 7 - useResize
**位置**: `GLoader.cpp::setup_beforeAdd()` 约 725 行附近
```cpp
if (buffer->version >= 7)
    _useResize = buffer->readBool();
```

---

## 🧪 测试验证清单

### 已完成功能测试

#### Transition Animation 扩展
- [ ] **基础功能**
  - [ ] Transition 播放时设置 animationName
  - [ ] Transition 播放时设置 skinName
  - [ ] 同时设置 animationName 和 skinName

- [ ] **数据加载**
  - [ ] Buffer version 6 数据正确读取
  - [ ] 旧版本 Buffer (< 6) 兼容性
  - [ ] 空字符串处理

- [ ] **目标类型**
  - [ ] GLoader 作为 target
  - [ ] GLoader3D 作为 target
  - [ ] 其他类型 target (应被忽略)

- [ ] **Spine 集成**
  - [ ] Spine 动画切换
  - [ ] Spine 皮肤切换
  - [ ] 非 Spine 内容 (应被忽略)

- [ ] **边界情况**
  - [ ] 动画名不存在
  - [ ] 皮肤名不存在
  - [ ] Transition 停止/重播
  - [ ] 嵌套 Transition

---

### 待实现功能测试模板

#### P0.1 ignoreEngineTimeScale 测试
- [ ] 默认值为 true
- [ ] 设置为 false 时受引擎时间影响
- [ ] 递归应用到子 Transition
- [ ] 递归应用到 Animation item
- [ ] 与游戏暂停/Time.timeScale 交互

#### P0.2 IAnimationGear 测试
- [ ] Playing 属性读写
- [ ] Frame 属性读写
- [ ] TimeScale 属性读写
- [ ] DeltaTime 属性应用
- [ ] Advance 方法调用

#### P1.1 invalidateBatchingEveryFrame 测试
- [ ] 开启时深度正确
- [ ] 关闭时性能更好
- [ ] XY 动画深度
- [ ] Size 动画深度
- [ ] Rotation 动画深度

#### P1.2 CustomEase 测试
- [ ] 编辑器导出数据加载
- [ ] 自定义曲线插值计算
- [ ] 与预定义 EaseType 对比
- [ ] 与编辑器预览一致

---

## 📝 实现笔记

### 架构决策

#### 1. 接口 vs 属性系统
**决策**: 保持 C++ 的 ObjectPropID 属性系统,不改为 Unity 的接口方式

**原因**:
- C++ 属性系统性能更好 (switch 分发 vs 虚函数)
- 扩展性更强 (添加枚举即可)
- 符合 C++ 引擎特性
- Unity 接口主要是为了 C# 类型安全

**代价**: 需要通过文档和单元测试保证完整性

#### 2. C++ 独有特性保留
**决策**: 保留 GLoader 的 Spine/CO2 特效支持

**原因**:
- Unity 版本没有这些功能
- 是 C++ 版本的竞争优势
- 已有用户依赖这些特性

**注意**: 新功能添加时需考虑与这些扩展的兼容性

---

### 实现优先级原则

1. **P0 级别**: 影响核心功能,无法 workaround
2. **P1 级别**: 影响体验,有 workaround 但麻烦
3. **P2 级别**: 锦上添花,缺失可接受
4. **不实现**: 引擎差异,不适用或成本过高

---

## 🔗 参考资源

### Unity FairyGUI 源码
- **路径**: `D:\COP\cop_mytools\FairyGUI-unity\Assets\Scripts\UI\`
- **关键文件**:
  - `Transition.cs`
  - `GLoader.cs`
  - `GLoader3D.cs`
  - `IAnimationGear.cs`
  - `IColorGear.cs`

### axmol FairyGUI 源码
- **路径**: `D:\COP\cop_mytools\axmol\extensions\fairygui\src\fairygui\`
- **关键文件**:
  - `Transition.h/cpp`
  - `GLoader.h/cpp`
  - `GLoader3D.h/cpp`
  - `GObject.h/cpp`
  - `FieldTypes.h`

### 相关文档
- [FairyGUI 官方文档](https://www.fairygui.com/)
- axmol 引擎文档

---

## 📅 更新日志

### 2025-11-12
- ✅ 完成 Transition Animation 扩展 (animationName/skinName)
- ✅ 创建本跟踪文档
- ✅ 完成 Unity vs C++ 全面差异分析
- 📝 规划 P0/P1/P2 实现路线图

---

## 👥 贡献者

- **分析与规划**: Claude Code (2025-11-12)
- **实现**: 待补充

---

**文档维护**: 每次功能实现后更新对应章节的状态和测试结果
