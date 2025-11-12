# FairyGUI Unity → axmol 功能同步优先级路线图

> **文档版本**: v2.0 (合并代码分析 + Git 历史分析)
> **创建时间**: 2025-11-12
> **分析方法**: 代码对比 + Git 历史挖掘 + 功能状态确认

---

## 📊 执行摘要

### 发现总览
- **代码对比分析**: 发现 10 个主要功能差异
- **Git 历史分析**: 新发现 12 个功能点
- **功能状态确认**:
  - ✅ getTouchTarget: **已实现**
  - ❌ UIPackage 异步加载: **未实现**
  - ❌ mouseWheelScale: **未实现**
  - ❌ Atlas 引用计数: **未实现**

### 合并后统计
- **已完成**: 1 项 (animationName/skinName)
- **P0 关键**: 3 项 (7-13 天)
- **P1 重要**: 10 项 (20-30 天)
- **P2 增强**: 8 项 (15-22 天)
- **P3 低优**: 2 项 (不适用)
- **总计**: 24 项功能, **42-65 天** (约 2-3 个月)

### 完成度评估
- **当前**: ~82-85%
- **P0 完成后**: ~88-90%
- **P1 完成后**: ~95-97%
- **全部完成后**: ~99%

---

## 🎯 优先级矩阵

### 评分标准
- **影响力**: 对系统功能的影响 (1-5 分)
- **紧急度**: 用户需求紧迫性 (1-5 分)
- **实现难度**: 开发复杂度 (1-5 分,越低越好)
- **ROI**: 投入产出比 = (影响力 + 紧急度) / 实现难度

| 功能 | 影响力 | 紧急度 | 难度 | ROI | 优先级 |
|-----|-------|-------|------|-----|-------|
| UIPackage 异步加载 | 5 | 5 | 4 | 2.5 | **P0** |
| ignoreEngineTimeScale | 5 | 5 | 2 | 5.0 | **P0** |
| IAnimationGear 完善 | 5 | 4 | 3 | 3.0 | **P0** |
| mouseWheelScale | 3 | 4 | 1 | 7.0 | **P1** ⬆️ |
| CustomEase | 4 | 4 | 3 | 2.7 | **P1** |
| Spine 异步加载 | 4 | 3 | 3 | 2.3 | **P1** |
| invalidateBatchingEveryFrame | 4 | 3 | 3 | 2.3 | **P1** |
| Atlas 引用计数 | 3 | 3 | 3 | 2.0 | **P1** |
| Alpha 纹理宏 | 3 | 3 | 3 | 2.0 | **P1** |

---

## 🔴 P0 级别 - 核心功能 (必须实现)

### P0.1 UIPackage 异步加载机制 ⭐⭐⭐⭐⭐
- **状态**: ❌ **确认未实现** (只有同步 addPackage)
- **影响**: **极高** - 大型项目加载卡顿
- **优先级**: **P0 (最高)**
- **预计工作量**: 5-7 天
- **ROI**: 2.5 (高影响但需要架构改造)

**Unity 实现** (Git: f2bbc54, 2020-06-16):
```csharp
// UIPackage.cs
public static void AddPackage(string assetPath,
    System.Action<UIPackage> callback)
{
    // 异步加载 AB 或资源
    // 解析 UI 包描述
    // 回调通知完成
}
```

**C++ 当前状态**:
```cpp
// UIPackage.h - 仅同步接口
static UIPackage* addPackage(const std::string& descFilePath);
```

**需要实现**:
```cpp
// UIPackage.h - 新增异步接口
typedef std::function<void(UIPackage*)> LoadCompleteCallback;

static void addPackageAsync(const std::string& descFilePath,
                            LoadCompleteCallback callback);
```

**实现要点**:
1. 设计异步加载架构 (线程 or 协程)
2. 资源加载队列管理
3. 回调机制 (主线程安全)
4. 进度通知接口
5. 错误处理和重试
6. 与现有同步加载兼容

**技术挑战**:
- axmol 资源加载线程模型
- 跨线程通信安全性
- UI 创建必须在主线程
- 加载状态管理

**测试要点**:
- [ ] 大型 UI 包加载 (>10MB)
- [ ] 并发加载多个包
- [ ] 加载失败回退
- [ ] 内存管理正确性
- [ ] 主线程不阻塞

**参考资料**:
- Unity UIPackage.cs 行 f2bbc54
- axmol AsyncTaskPool 机制

---

### P0.2 ignoreEngineTimeScale ⭐⭐⭐⭐⭐
- **状态**: ❌ 未实现
- **影响**: **极高** - UI 时间控制核心
- **优先级**: **P0**
- **预计工作量**: 2-3 天
- **ROI**: 5.0 (高影响低成本)

**Unity 实现** (Git: 多次提交):
```csharp
// Transition.cs 行49
bool _ignoreEngineTimeScale = true;  // 默认忽略引擎时间

// 行687-715: setter 递归应用
public bool ignoreEngineTimeScale {
    set {
        _ignoreEngineTimeScale = value;
        foreach (var item in _items) {
            if (item.type == TransitionActionType.Transition)
                item.trans.ignoreEngineTimeScale = value;
            else if (item.type == TransitionActionType.Animation)
                item.target.ignoreEngineTimeScale = value;
            if (item.tweener != null)
                item.tweener.SetIgnoreEngineTimeScale(value);
        }
    }
}
```

**C++ 需要实现**:
```cpp
// Transition.h
class Transition {
private:
    bool _ignoreEngineTimeScale;  // 默认 true
public:
    bool getIgnoreEngineTimeScale() const;
    void setIgnoreEngineTimeScale(bool value);
};

// Transition.cpp
void Transition::setIgnoreEngineTimeScale(bool value) {
    if (_ignoreEngineTimeScale != value) {
        _ignoreEngineTimeScale = value;
        // 递归应用到所有 item
        for (auto& item : _items) {
            // 子 Transition
            if (item->type == TransitionActionType::Transition) {
                auto trans = ((TValue_Transition*)item->value)->trans;
                if (trans) trans->setIgnoreEngineTimeScale(value);
            }
            // Animation 对象
            else if (item->type == TransitionActionType::Animation) {
                // 需要 GObject 支持 ignoreEngineTimeScale 属性
            }
            // Tweener
            if (item->tweener)
                item->tweener->setIgnoreEngineTimeScale(value);
        }
    }
}
```

**依赖项**:
1. **GTween 系统支持**: 需要 GTweener::setIgnoreEngineTimeScale()
2. **GObject 属性**: 动画对象需要支持此属性
3. **时间管理**: 需要引擎时间和真实时间分离

**实现步骤**:
1. 检查 GTween 是否支持时间缩放独立性
2. Transition 类添加成员和方法
3. 在 playItem() 设置 tweener
4. 递归应用逻辑实现
5. 单元测试: 暂停菜单场景

**关键用例**:
```cpp
// 游戏暂停时,暂停菜单仍能播放动画
Transition* pauseMenuAnim = pauseMenu->getTransition("show");
pauseMenuAnim->setIgnoreEngineTimeScale(true);  // 关键!
pauseMenuAnim->play();

// 此时 Time.timeScale = 0 (游戏暂停)
// 但 pauseMenuAnim 仍正常播放
```

**测试清单**:
- [ ] 默认值为 true
- [ ] 设置为 false 受引擎时间影响
- [ ] 递归应用到嵌套 Transition
- [ ] 递归应用到 Animation item
- [ ] 与游戏暂停交互正确

---

### P0.3 IAnimationGear 接口完善 ⭐⭐⭐⭐⭐
- **状态**: ⚠️ 部分实现 (通过 ObjectPropID)
- **影响**: **极高** - 动画系统基础
- **优先级**: **P0**
- **预计工作量**: 2-3 天
- **ROI**: 3.0

**Unity 接口定义**:
```csharp
public interface IAnimationGear {
    bool playing { get; set; }          // 播放状态
    int frame { get; set; }             // 当前帧
    float timeScale { get; set; }       // 时间缩放
    bool ignoreEngineTimeScale { get; set; }  // 忽略引擎时间
    void Advance(float time);           // 手动推进
}
```

**C++ ObjectPropID 对应**:
```cpp
enum class ObjectPropID {
    Playing,              // ✅ 已有
    Frame,                // ✅ 已有
    TimeScale,            // ⚠️ 需验证完整性
    DeltaTime,            // ⚠️ 需验证 (对应 Advance)
    // ignoreEngineTimeScale - ❌ 缺失
};
```

**需要实现**:
1. **新增 ObjectPropID 枚举值**:
   ```cpp
   enum class ObjectPropID {
       // ... 现有的
       IgnoreEngineTimeScale,  // ✅ 新增
   };
   ```

2. **所有动画类实现完整**:
   - GMovieClip
   - GLoader (Spine/CO2)
   - GLoader3D

3. **Advance 方法统一**:
   ```cpp
   // GObject.h 添加虚方法
   virtual void advance(float deltaTime);

   // GMovieClip.cpp 实现
   void GMovieClip::advance(float deltaTime) {
       // 手动推进动画时间
   }
   ```

**实现检查清单**:
- [ ] GMovieClip 所有属性
  - [ ] Playing ✅
  - [ ] Frame ✅
  - [ ] TimeScale ⚠️
  - [ ] DeltaTime ⚠️
  - [ ] IgnoreEngineTimeScale ❌
  - [ ] Advance 方法 ❌

- [ ] GLoader 所有属性
  - [ ] Playing ✅
  - [ ] Frame ✅
  - [ ] TimeScale ⚠️
  - [ ] DeltaTime ⚠️
  - [ ] IgnoreEngineTimeScale ❌

- [ ] GLoader3D 所有属性
  - [ ] Playing ✅
  - [ ] Frame ✅
  - [ ] TimeScale ❌ (空实现)
  - [ ] DeltaTime ❌ (空实现)
  - [ ] IgnoreEngineTimeScale ❌

**测试清单**:
- [ ] Transition 设置 Playing
- [ ] Transition 设置 Frame
- [ ] Transition 设置 TimeScale
- [ ] Transition 调用 Advance
- [ ] 与 P0.2 ignoreEngineTimeScale 集成测试

---

## 🟡 P1 级别 - 重要功能 (强烈建议)

### P1.1 mouseWheelScale (快速胜利!) ⭐⭐⭐⭐
- **状态**: ❌ **确认未实现**
- **影响**: **中高** - 用户体验
- **优先级**: **P1 (推荐优先实施)**
- **预计工作量**: **0.5 天** ⚡ 最快!
- **ROI**: 7.0 (超高!)
- **Git**: c9c4318 (2024-04-08)

**Unity 实现**:
```csharp
// Stage.cs
public static float mouseWheelScale = 1f;

// 滚轮事件处理
float delta = Input.mouseScrollDelta.y * mouseWheelScale;
```

**C++ 实现**:
```cpp
// GRoot.h
class GRoot {
public:
    static float mouseWheelScale;  // 默认 1.0f

    // 或者用 getter/setter
    static float getMouseWheelScale();
    static void setMouseWheelScale(float value);
};

// GRoot.cpp
float GRoot::mouseWheelScale = 1.0f;

// 在滚轮事件处理中
void GRoot::onMouseWheel(EventContext* context) {
    float delta = context->getMouseWheelDelta() * mouseWheelScale;
    // ...
}
```

**实现步骤**:
1. GRoot 添加静态成员 (5 分钟)
2. 滚轮事件处理应用缩放 (10 分钟)
3. ScrollPane 滚动应用 (20 分钟)
4. 测试验证 (30 分钟)

**总计**: 约 1 小时实现 + 3 小时测试 = 半天

**建议**: 作为第一个 P1 实现,快速见效,提升士气!

---

### P1.2 CustomEase 支持 ⭐⭐⭐⭐
- **状态**: ❌ 未实现
- **影响**: **高** - 编辑器功能完整性
- **优先级**: **P1**
- **预计工作量**: 2-3 天
- **ROI**: 2.7
- **Git**: 104db19 (2020-08-10)
- **Buffer Version**: 4

**Unity 实现**:
```csharp
// EaseType.cs
public class CustomEase {
    private BezierCurve _curve;

    public void Create(List<GPathPoint> points) {
        _curve = new BezierCurve(points);
    }

    public float Evaluate(float time) {
        return _curve.Evaluate(time);
    }
}

// Transition.cs 行1477-1485
if (buffer.version >= 4 && item.tweenConfig.easeType == EaseType.Custom) {
    var pts = buffer.ReadPath();
    if (pts.Count > 0) {
        item.tweenConfig.customEase = new CustomEase();
        item.tweenConfig.customEase.Create(pts);
    }
}
```

**C++ 需要实现**:
```cpp
// tween/CustomEase.h
class CustomEase {
public:
    void create(const std::vector<GPathPoint>& points);
    float evaluate(float time) const;

private:
    GPath* _curve;
};

// FieldTypes.h
enum class EaseType {
    // ... 现有的
    Custom,  // ✅ 确认是否已有
};

// TweenConfig 类
class TweenConfig {
public:
    CustomEase* customEase;  // ✅ 新增
    // ...
};

// Transition.cpp::setup()
if (buffer->version >= 4 && item->tweenConfig->easeType == EaseType::Custom) {
    // 读取贝塞尔曲线点
    auto points = buffer->readPath();  // ⚠️ 确认 ByteBuffer 是否有此方法
    if (!points.empty()) {
        item->tweenConfig->customEase = new CustomEase();
        item->tweenConfig->customEase->create(points);
    }
}

// Transition.cpp::playItem()
if (item->tweenConfig->customEase) {
    item->tweener->setEase(item->tweenConfig->customEase);
}
```

**实现步骤**:
1. 创建 CustomEase 类 (参考 GPath)
2. 实现贝塞尔曲线插值算法
3. ByteBuffer 添加 readPath() (如果没有)
4. Transition::setup() 解析 buffer version 4
5. TweenConfig 添加 customEase 成员
6. GTweener 支持自定义缓动
7. 与编辑器预览对比测试

**测试清单**:
- [ ] 编辑器导出自定义缓动 UI 包
- [ ] 加载并播放
- [ ] 与编辑器预览效果一致
- [ ] 多种曲线类型测试

---

### P1.3 Spine 异步加载 ⭐⭐⭐⭐
- **状态**: ❌ 未实现 (GLoader Spine 是同步加载)
- **影响**: **高** - 大型 Spine 卡顿
- **优先级**: **P1**
- **预计工作量**: 3-4 天
- **ROI**: 2.3
- **Git**: 0d15fbe (2020-07-31), d8c7851 (2024-03-18 修复)

**Unity 实现**:
```csharp
// GLoader3D.cs
void LoadSpine(string path, System.Action callback) {
    // 异步加载 Spine 资源
    StartCoroutine(LoadSpineAsync(path, callback));
}
```

**C++ 当前状态**:
```cpp
// GLoader.cpp 行499-694
// Spine 加载是同步的
if (url.compare(0, 8, "spine://") == 0) {
    // ... 同步加载
    skeletonAni = spine::SkeletonAnimation::createWithJsonFile(...);
}
```

**需要实现**:
```cpp
// GLoader.h
class GLoader {
public:
    void loadSpineAsync(const std::string& url,
                       const std::function<void()>& callback);
private:
    std::function<void()> _spineLoadCallback;
};

// GLoader.cpp
void GLoader::loadSpineAsync(const std::string& url,
                             const std::function<void()>& callback) {
    _spineLoadCallback = callback;

    // 使用 axmol AsyncTaskPool
    AsyncTaskPool::getInstance()->enqueue(
        AsyncTaskPool::TaskType::TASK_IO,
        [this, url](void*) {
            // 后台加载 spine 文件
            std::string jsonData = FileUtils::getInstance()->getStringFromFile(url);

            // 切回主线程创建 SkeletonAnimation
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]() {
                auto skeletonAni = createSpineFromData(jsonData);
                // ...
                if (_spineLoadCallback)
                    _spineLoadCallback();
            });
        },
        nullptr,
        []() {}
    );
}
```

**实现步骤**:
1. 设计异步加载接口
2. 使用 AsyncTaskPool 后台加载文件
3. 主线程创建 SkeletonAnimation
4. 回调通知完成
5. 错误处理
6. 与现有同步加载兼容

**测试清单**:
- [ ] 大型 Spine (>5MB)
- [ ] 主线程不卡顿
- [ ] 加载失败处理
- [ ] 回调正确触发
- [ ] 并发加载多个 Spine

---

### P1.4 invalidateBatchingEveryFrame ⭐⭐⭐⭐
- **状态**: ❌ 未实现
- **影响**: **高** - 批处理深度问题
- **优先级**: **P1**
- **预计工作量**: 3-4 天
- **ROI**: 2.3
- **Git**: decc011 (2016-10-20)

**Unity 实现** (详见之前分析):
```csharp
// Transition.cs 行31
public bool invalidateBatchingEveryFrame;

// 行1271-1347: 在多个动作类型中调用
if (invalidateBatchingEveryFrame)
    _owner.InvalidateBatchingState(true);
```

**C++ 需要实现**:
```cpp
// Transition.h
class Transition {
public:
    bool invalidateBatchingEveryFrame;  // 公共成员
};

// Transition.cpp::applyValue()
case TransitionActionType::XY:
case TransitionActionType::Size:
case TransitionActionType::Rotation:
case TransitionActionType::Pivot:
case TransitionActionType::Scale:
case TransitionActionType::Skew:
    // ... 应用值
    if (invalidateBatchingEveryFrame)
        _owner->invalidateBatchingState(true);
    break;
```

**依赖确认**:
- [ ] GComponent::invalidateBatchingState() 方法是否存在
- [ ] 批处理系统是否正常工作

**实现步骤**: (详见 FAIRYGUI_UNITY_SYNC.md)

---

### P1.5 Atlas 引用计数管理 ⭐⭐⭐
- **状态**: ❌ **确认未实现**
- **影响**: **中高** - 内存管理
- **优先级**: **P1**
- **预计工作量**: 2-3 天
- **ROI**: 2.0
- **Git**: 157812d (2020-08-26)

**Unity 实现**:
```csharp
// UIPackage.cs
class AtlasRef {
    public NTexture texture;
    public int refCount;
}

void AddAtlasRef(NTexture texture) {
    if (!_atlasRefs.ContainsKey(texture))
        _atlasRefs[texture] = new AtlasRef { texture = texture, refCount = 0 };
    _atlasRefs[texture].refCount++;
}

void ReleaseAtlasRef(NTexture texture) {
    if (_atlasRefs.ContainsKey(texture)) {
        _atlasRefs[texture].refCount--;
        if (_atlasRefs[texture].refCount <= 0) {
            texture.Dispose();
            _atlasRefs.Remove(texture);
        }
    }
}
```

**C++ 需要实现**:
```cpp
// UIPackage.h
class UIPackage {
private:
    struct AtlasRef {
        NTexture* texture;
        int refCount;
    };
    static std::unordered_map<NTexture*, AtlasRef> _atlasRefs;

public:
    static void addAtlasRef(NTexture* texture);
    static void releaseAtlasRef(NTexture* texture);
};
```

**实现步骤**:
1. 设计 AtlasRef 结构
2. 在加载时增加引用计数
3. 在卸载时减少引用计数
4. 引用为 0 时自动释放
5. 防止重复释放

**测试清单**:
- [ ] 多个对象使用同一图集
- [ ] 引用计数正确
- [ ] 自动卸载测试
- [ ] 内存泄漏检测

---

### P1.6 Alpha 纹理宏支持 ⭐⭐⭐
- **状态**: ⚠️ 需评估
- **影响**: **中** - 性能优化
- **优先级**: **P1**
- **预计工作量**: 2-3 天
- **ROI**: 2.0
- **Git**: 57ae5a4 (2024-03-19)

**Unity 实现**:
```csharp
// UIPackage.cs
#if FAIRYGUI_USE_ALPHA_TEXTURE
    // 启用分离 alpha 纹理
    // 减少 RGBA32 纹理内存占用
#endif
```

**需要评估**:
- axmol 是否支持分离 Alpha 纹理格式
- 是否有性能提升
- 是否需要宏开关

**实现步骤**:
1. 研究 axmol 纹理格式支持
2. 评估分离 Alpha 的收益
3. 如有收益,添加宏支持
4. UIPackage 纹理加载逻辑修改
5. 性能测试对比

---

### P1.7 IColorGear 完善 ⭐⭐⭐⭐
- **状态**: ⚠️ 部分实现
- **优先级**: **P1**
- **预计工作量**: 2-3 天
- **ROI**: 2.0

(详见 FAIRYGUI_UNITY_SYNC.md)

---

### P1.8 GLoader 外部加载钩子 ⭐⭐⭐
- **状态**: ✅ 虚函数已支持
- **优先级**: **P1**
- **预计工作量**: 1-2 天
- **ROI**: 3.5

(详见 FAIRYGUI_UNITY_SYNC.md)

---

### P1.9 Gear 自动批处理失效 ⭐⭐⭐
- **状态**: ❌ 未实现
- **优先级**: **P1**
- **预计工作量**: 1 天
- **ROI**: 3.0
- **Git**: decc011 (2016-10-20)

**Unity 实现**:
```csharp
// GearXY.cs, GearSize.cs, GearLook.cs
public override void Apply() {
    // ... 应用属性
    if (_owner.parent != null)
        _owner.parent.InvalidateBatchingState();
}
```

**C++ 需要实现**:
在对应 Gear 类的 apply() 方法末尾调用批处理失效

**实现步骤**:
1. 检查所有 Gear 类
2. 在 apply() 末尾添加调用
3. 测试验证

---

### P1.10 ScrollPane 增强功能 ⭐⭐⭐
- **状态**: ⚠️ 需确认
- **优先级**: **P1**
- **预计工作量**: 2-3 天
- **ROI**: 2.0
- **Git**: 4714a7f (2021-04-30), 3ac445f (2020-05-05)

**Unity 新增**:
```csharp
// UIConfig.cs
public static float defaultScrollSnappingThreshold = 0.1f;
public static float defaultScrollPagingThreshold = 0.3f;

// ScrollPane.cs
float dontClipMargin;  // 允许内容溢出裁剪区
```

**C++ 需要实现**:
1. UIConfig 添加滚动阈值配置
2. ScrollPane 添加 dontClipMargin 支持

---

## 🟢 P2 级别 - 增强功能 (可选)

### P2.1 showErrorSign ⭐⭐⭐
(详见 FAIRYGUI_UNITY_SYNC.md)

### P2.2 useResize ⭐⭐⭐
(详见 FAIRYGUI_UNITY_SYNC.md)

### P2.3 GLoader3D 时间控制 ⭐⭐⭐
(详见 FAIRYGUI_UNITY_SYNC.md)

### P2.4 GetTouchTarget ⭐⭐⭐
- **状态**: ✅ **已实现!**
- **确认**: GRoot::getTouchTarget() 已存在
- **无需实现**: 划掉此项 ✅

### P2.5 DrawRegularPolygon 修复 ⭐⭐
- **Git**: cbf3641 (2025-05-29) - 最新!
- **工作量**: 0.5 天

### P2.6 DragonBones 支持 ⭐⭐
(详见 FAIRYGUI_UNITY_SYNC.md,需评估必要性)

---

## ⚪ P3 级别 - 不适用/低优

### P3.1 FAIRYGUI_INPUT_SYSTEM 宏
- Unity 新输入系统,C++ 不适用

### P3.2 域重载禁用
- Unity Editor 特性,C++ 不适用

### P3.3 GoWrapper PaintMode
- Unity GameObject 特性,C++ 使用 Node,不适用

---

## 📊 更新后的完整统计

### 按优先级
| 优先级 | 功能数 | 预计天数 | 完成率 |
|-------|--------|----------|--------|
| ✅ 已完成 | 1 | - | 100% |
| 🔴 P0 | 3 | 9-13 | 0% |
| 🟡 P1 | 10 | 20-30 | 0% |
| 🟢 P2 | 7 | 13-20 | 14% (1/7) |
| ⚪ P3 | 3 | - | - |
| **总计** | **24** | **42-63** | **4%** |

### 状态分布
- ✅ 已实现: 2 项 (animationName/skinName, getTouchTarget)
- ❌ 确认未实现: 15 项
- ⚠️ 需确认: 7 项

### 功能来源
- 代码对比发现: 10 项
- Git 历史发现: 12 项
- 重复: 2 项

---

## 🚀 实施路线图 (推荐)

### 第 1 周 (P0 + 快速胜利)

**Day 1-3: P0.2 ignoreEngineTimeScale** (关键!)
- 实现 Transition 时间控制
- GTween 系统支持
- 测试暂停菜单

**Day 4-5: P0.3 IAnimationGear 完善**
- 补充缺失属性
- 实现 Advance 方法
- 单元测试

**Day 5 下午: P1.1 mouseWheelScale** (快速胜利!)
- 0.5 天快速实现
- 立即见效,提升士气!

---

### 第 2 周 (P0 + P1 核心)

**Day 1-4: P0.1 UIPackage 异步加载** (最大挑战)
- 设计异步架构
- 实现资源异步加载
- 回调机制
- 测试验证

**Day 5: P1.2 CustomEase** (开始)
- 创建 CustomEase 类
- 贝塞尔曲线实现

---

### 第 3 周 (P1 重要功能)

**Day 1-2: P1.2 CustomEase** (完成)
- Buffer 解析
- Transition 集成
- 测试

**Day 3-4: P1.4 invalidateBatchingEveryFrame**
- 实现批处理失效
- 多处调用点
- 视觉验证

**Day 5: P1.9 Gear 批处理**
- 快速实现

---

### 第 4 周 (P1 剩余)

**Day 1-3: P1.3 Spine 异步加载**
- 异步加载接口
- 回调机制
- 测试

**Day 4-5: P1.5 Atlas 引用计数**
- 引用计数系统
- 自动卸载
- 内存测试

---

### 第 5 周及以后 (P1 收尾 + P2)

- P1.6 Alpha 纹理宏 (评估后决定)
- P1.7 IColorGear 完善
- P1.8 GLoader 外部钩子
- P1.10 ScrollPane 增强
- P2 级别功能按需实现

---

## ✅ 立即行动项 (今天!)

### 1. 功能状态确认 (1-2 小时)

运行以下检查,更新状态:

```bash
cd "D:\COP\cop_mytools\axmol\extensions\fairygui"

# 检查批处理失效方法
grep -r "invalidateBatching" src/fairygui/GComponent.*

# 检查 Gear 类
ls -la src/fairygui/gears/

# 检查 ScrollPane
grep -r "snapping\|paging\|dontClip" src/fairygui/ScrollPane.*

# 检查 Alpha 纹理
grep -r "alpha.*texture\|separate.*alpha" src/

# 检查 ByteBuffer.readPath
grep -r "readPath" src/fairygui/utils/ByteBuffer.*
```

### 2. 更新 TODO.md (30 分钟)

根据本文档更新任务清单

### 3. 创建 Week 1 分支 (10 分钟)

```bash
cd "D:\COP\cop_mytools\axmol"
git checkout -b feature/fairygui-unity-sync-week1
```

### 4. 开始 P0.2 实现 (今天下午)

从 ignoreEngineTimeScale 开始,这是影响最大、难度适中的功能

---

## 🎯 成功标准

### P0 完成标准 (Week 2 结束)
- [ ] UIPackage 支持异步加载
- [ ] Transition 支持 ignoreEngineTimeScale
- [ ] IAnimationGear 所有属性完整
- [ ] 通过暂停菜单集成测试

### P1 完成标准 (Week 5 结束)
- [ ] mouseWheelScale 实现
- [ ] CustomEase 支持
- [ ] Spine 异步加载
- [ ] invalidateBatchingEveryFrame 实现
- [ ] Atlas 引用计数管理
- [ ] 通过性能基准测试
- [ ] 与 Unity 版本对比测试 95%+ 一致

### 最终目标 (2-3 个月)
- [ ] 功能对等性 99%+
- [ ] 性能不低于 Unity 版本
- [ ] 通过大型项目验证
- [ ] 文档完整

---

## 📝 每周检查清单

### 每周五下午
- [ ] 更新本文档状态
- [ ] 更新 TODO.md 完成情况
- [ ] 提交代码并打 tag
- [ ] 性能测试报告
- [ ] 下周计划确认

---

**文档维护**: 每次功能实现后更新状态
**最后更新**: 2025-11-12
