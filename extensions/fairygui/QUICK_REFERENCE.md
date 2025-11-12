# FairyGUI Unity 同步 - 快速参考卡

> 一页纸速查表,用于快速了解功能差异和实施要点

---

## 🎯 核心差异 Top 5

| # | 功能 | 状态 | 优先级 | 工作量 | 影响 |
|---|------|------|--------|--------|------|
| 1 | **ignoreEngineTimeScale**<br>UI 独立时间控制 | ❌ | P0 | 3天 | ⭐⭐⭐⭐⭐<br>暂停菜单必需 |
| 2 | **CustomEase**<br>自定义缓动曲线 | ❌ | P1 | 3天 | ⭐⭐⭐⭐<br>编辑器功能 |
| 3 | **invalidateBatchingEveryFrame**<br>批处理深度修复 | ❌ | P1 | 4天 | ⭐⭐⭐⭐<br>显示正确性 |
| 4 | **IAnimationGear 完善**<br>动画属性完整性 | ⚠️ | P0 | 3天 | ⭐⭐⭐⭐⭐<br>系统基础 |
| 5 | **animationName/skinName**<br>Transition 动画控制 | ✅ | - | - | ⭐⭐⭐⭐<br>已完成! |

---

## ⚡ 立即行动 (本周)

### 必做: P0 关键功能

```cpp
// 1. Transition.h - 添加成员
private:
    bool _ignoreEngineTimeScale;  // 默认 true

public:
    bool getIgnoreEngineTimeScale() const { return _ignoreEngineTimeScale; }
    void setIgnoreEngineTimeScale(bool value);

// 2. 递归应用逻辑
void Transition::setIgnoreEngineTimeScale(bool value) {
    if (_ignoreEngineTimeScale != value) {
        _ignoreEngineTimeScale = value;
        for (auto& item : _items) {
            // 递归应用到子 Transition 和 Animation
        }
    }
}

// 3. playItem() 设置 tweener
item->tweener->setIgnoreEngineTimeScale(_ignoreEngineTimeScale);
```

---

## 📐 架构对比

### Unity 接口 vs C++ 属性

| Unity 方式 | C++ 方式 | 推荐 |
|-----------|---------|------|
| `IAnimationGear` 接口<br>强类型,编译检查 | `ObjectPropID` 枚举<br>灵活,性能好 | ✅ 保持 C++ 方式 |
| `IColorGear` 接口<br>明确约定 | `getProp/setProp`<br>统一入口 | ✅ 保持 C++ 方式 |

**结论**: C++ 架构合理,无需改接口,但需完善文档和测试

---

## 🔢 版本兼容性速查

| Buffer Ver | 功能 | C++ 状态 | 优先级 |
|-----------|------|----------|--------|
| 2 | Path 曲线 | ✅ | - |
| **4** | **CustomEase** | ❌ | **P1** |
| **6** | **animationName/skinName** | ✅ | **完成** |
| **7** | **useResize** | ❌ | **P2** |

---

## 🎨 C++ 独有优势

✨ **这些功能 Unity 版本没有,应该保留!**

1. **GLoader Spine 支持** (行499-694)
   - `spine://` 协议
   - 配置文件映射
   - JSON/Binary 自动检测

2. **CO2 特效系统** (行429-496)
   - `effect://` 协议
   - CO2EffectPlayer 集成

3. **WDB 格式支持**
   - 自定义资源格式

---

## 🗺️ 实施路线图

```
Week 1 (P0):
├─ Day 1-3: ignoreEngineTimeScale ⭐⭐⭐⭐⭐
└─ Day 4-5: IAnimationGear 完善 ⭐⭐⭐⭐⭐

Week 2 (P1 核心):
├─ Day 1-3: CustomEase 支持 ⭐⭐⭐⭐
└─ Day 4-5: IColorGear 完善 ⭐⭐⭐⭐

Week 3 (P1 剩余):
├─ Day 1-4: invalidateBatchingEveryFrame ⭐⭐⭐⭐
└─ Day 5: GLoader 外部钩子 ⭐⭐⭐

Week 4+ (P2 可选):
├─ useResize (1天) ⭐⭐⭐
├─ showErrorSign (2天) ⭐⭐⭐
├─ GLoader3D 时间控制 (3天) ⭐⭐⭐
└─ DragonBones (7天,待评估) ⭐⭐
```

**目标**: 4 周达到 95%+ 功能对等

---

## 🔍 代码位置速查

### Unity 源码关键位置

```
Transition.cs
├─ 行49: _ignoreEngineTimeScale 定义
├─ 行687-715: ignoreEngineTimeScale setter
├─ 行1477-1485: CustomEase 解析 (version 4)
├─ 行1538-1542: animationName/skinName (version 6)
└─ 行1271-1347: invalidateBatchingEveryFrame 调用

GLoader.cs
├─ 行16: showErrorSign 定义
├─ 行159-169: useResize 属性
├─ 行520-538: SetErrorState 实现
└─ 行725-726: useResize 读取 (version 7)

IAnimationGear.cs
└─ 接口定义: playing, frame, timeScale, ignoreEngineTimeScale, Advance
```

### C++ 源码关键位置

```
Transition.cpp
├─ 行27-35: TValue_Animation 类 ✅ 已扩展
├─ 行560-571: setValue Animation ✅ 已扩展
├─ 行1287-1305: applyValue Animation ✅ 已扩展
├─ 行1499-1509: decodeValue Animation ✅ 已扩展
└─ 需添加: ignoreEngineTimeScale, invalidateBatchingEveryFrame

GLoader.cpp
├─ 行252-299: setAnimationName/setSkinName ✅ 已实现
├─ 行499-694: Spine 协议支持 ✨ 独有
├─ 行429-496: CO2 特效支持 ✨ 独有
└─ 需添加: showErrorSign, useResize

GObject.h
└─ ObjectPropID 枚举: 需确认完整性
```

---

## 🧪 测试优先级

### P0 测试 (本周必做)

```cpp
// 测试 1: ignoreEngineTimeScale
Transition* trans = comp->getTransition("pause_menu");
trans->setIgnoreEngineTimeScale(true);
trans->play();
// 验证: 在 Time.timeScale = 0 时仍正常播放

// 测试 2: IAnimationGear 完整性
loader->setProp(ObjectPropID::Playing, Value(true));
loader->setProp(ObjectPropID::Frame, Value(5));
loader->setProp(ObjectPropID::TimeScale, Value(2.0f));
// 验证: 所有属性正常工作
```

### P1 测试 (第 2-3 周)

```cpp
// 测试 3: CustomEase
// 使用编辑器导出的自定义缓动 UI 包
// 验证: 曲线动画与编辑器预览一致

// 测试 4: invalidateBatchingEveryFrame
trans->invalidateBatchingEveryFrame = true;
trans->play();
// 验证: 动画过程中深度显示正确
```

---

## ⚠️ 常见陷阱

### 1. Buffer Version 检查
```cpp
// ❌ 错误: 未检查版本
((TValue_Animation*)value)->animationName = buffer->readS();

// ✅ 正确: 检查版本
if (buffer->version >= 6) {
    ((TValue_Animation*)value)->animationName = buffer->readS();
}
```

### 2. 递归应用
```cpp
// ❌ 错误: 只设置当前 Transition
_ignoreEngineTimeScale = value;

// ✅ 正确: 递归应用到所有子元素
_ignoreEngineTimeScale = value;
for (auto& item : _items) {
    if (item->type == TransitionActionType::Transition)
        ((TValue_Transition*)item->value)->trans->setIgnoreEngineTimeScale(value);
    // ... 其他类型
}
```

### 3. 空指针检查
```cpp
// ❌ 错误: 直接转换
GLoader* loader = (GLoader*)item->target;
loader->setAnimationName(name);

// ✅ 正确: 使用 dynamic_cast
GLoader* loader = dynamic_cast<GLoader*>(item->target);
if (loader) {
    loader->setAnimationName(name);
}
```

---

## 📞 快速联系

**详细文档**: `FAIRYGUI_UNITY_SYNC.md`
**任务清单**: `TODO.md`
**Unity 源码**: `D:\COP\cop_mytools\FairyGUI-unity\Assets\Scripts\UI\`
**C++ 源码**: `D:\COP\cop_mytools\axmol\extensions\fairygui\src\fairygui\`

---

**最后更新**: 2025-11-12
