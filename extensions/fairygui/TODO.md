# FairyGUI Unity 功能同步任务清单

> 快速任务跟踪表,详细信息请参考 `FAIRYGUI_UNITY_SYNC.md`

---

## 🎯 当前进度概览

- ✅ 已完成: 1 项
- 🔴 P0 关键: 2 项 (5-8天)
- 🟡 P1 重要: 4 项 (8-12天)
- 🟢 P2 可选: 4 项 (9-13天)
- **总计工作量**: 22-33 天 (约 1-1.5 个月)

---

## ✅ 已完成 (1/11)

- [x] **Transition Animation 扩展** (2025-11-12)
  - animationName 和 skinName 支持
  - GLoader.setAnimationName/setSkinName
  - Buffer version 6 兼容

---

## 🔴 P0 - 关键功能 (0/2) - 必须立即实现

### P0.1 ignoreEngineTimeScale ⭐⭐⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (2-3天)
- [ ] **实施人**: _______

**子任务**:
- [ ] Transition.h 添加 `_ignoreEngineTimeScale` 成员
- [ ] 实现 getter/setter,递归应用
- [ ] GTween 系统确认支持引擎时间缩放
- [ ] playItem() 设置 tweener 属性
- [ ] 单元测试: 暂停菜单场景
- [ ] 与 Unity 行为对比测试

**关键影响**: UI 暂停菜单等功能的基础

---

### P0.2 IAnimationGear 完善 ⭐⭐⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (2-3天)
- [ ] **实施人**: _______

**子任务**:
- [ ] 审查所有 ObjectPropID 实现
  - [ ] GMovieClip: Playing, Frame, TimeScale, DeltaTime
  - [ ] GLoader: Playing, Frame, TimeScale, DeltaTime
  - [ ] GLoader3D: Playing, Frame, TimeScale, DeltaTime
- [ ] 补充缺失的枚举值
- [ ] 实现 Advance(deltaTime) 机制
- [ ] 单元测试覆盖
- [ ] 文档更新

**关键影响**: Transition 动画系统的基础

---

## 🟡 P1 - 重要功能 (0/4) - 强烈建议实现

### P1.1 invalidateBatchingEveryFrame ⭐⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (3-4天)
- [ ] **实施人**: _______

**子任务**:
- [ ] Transition.h 添加公共成员变量
- [ ] applyValue() 在 6 种动作类型中调用
  - [ ] XY
  - [ ] Size
  - [ ] Rotation
  - [ ] Pivot
  - [ ] Scale
  - [ ] Skew
- [ ] 验证 GComponent::invalidateBatchingState() 存在
- [ ] 性能测试
- [ ] 深度正确性验证

**关键影响**: 自动合批时的深度问题修复

---

### P1.2 CustomEase 支持 ⭐⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (2-3天)
- [ ] **实施人**: _______

**子任务**:
- [ ] 创建 tween/CustomEase.h/cpp
- [ ] 实现贝塞尔曲线插值算法
- [ ] FieldTypes.h 确认 EaseType::Custom 枚举
- [ ] TweenConfig 添加 customEase 成员
- [ ] Transition::setup() 解析 buffer version 4
- [ ] playItem() 应用自定义缓动
- [ ] 与编辑器预览效果对比

**关键影响**: 编辑器自定义缓动曲线支持

---

### P1.3 IColorGear 完善 ⭐⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (2-3天)
- [ ] **实施人**: _______

**子任务**:
- [ ] 审查颜色相关 ObjectPropID
  - [ ] Color (所有类)
  - [ ] OutlineColor (文本类)
- [ ] 检查实现类:
  - [ ] GImage
  - [ ] GTextField
  - [ ] GLoader
  - [ ] GGraph
  - [ ] GButton
- [ ] Transition Color 动作验证
- [ ] 单元测试

**关键影响**: Transition 颜色动画完整性

---

### P1.4 GLoader 外部加载钩子 ⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (1-2天)
- [ ] **实施人**: _______

**子任务**:
- [ ] 评估是否需要额外钩子 (当前已有虚函数)
- [ ] 设计回调注册接口 (可选)
- [ ] 实现全局回调管理器 (可选)
- [ ] 文档说明扩展方式
- [ ] 示例代码

**关键影响**: 自定义资源加载灵活性

---

## 🟢 P2 - 可选功能 (0/4) - 根据需求选择

### P2.1 showErrorSign ⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (1-2天)
- [ ] **实施人**: _______

**子任务**:
- [ ] GLoader.h 添加 showErrorSign, _errorSign 成员
- [ ] 实现 setErrorState()
- [ ] 实现 clearErrorState()
- [ ] UIConfig 添加 loaderErrorSign 配置
- [ ] 测试加载失败场景

**影响**: 开发调试友好性

---

### P2.2 useResize ⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (1天)
- [ ] **实施人**: _______

**子任务**:
- [ ] GLoader.h 添加 _useResize 成员
- [ ] updateLayout() 根据标志选择 SetSize/SetScale
- [ ] setup_beforeAdd() 读取 buffer version 7
- [ ] 验证 Component 缩放行为
- [ ] 与 Unity 效果对比

**影响**: Component 缩放行为一致性

---

### P2.3 GLoader3D 时间控制 ⭐⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (2-3天)
- [ ] **实施人**: _______

**子任务**:
- [ ] GLoader3D.h 添加时间相关成员
- [ ] 实现 TimeScale ObjectPropID
- [ ] 实现 DeltaTime ObjectPropID
- [ ] 实现 Advance 方法
- [ ] Spine 时间控制集成
- [ ] 测试验证

**影响**: 3D 动画时间控制能力

---

### P2.4 DragonBones 支持 ⭐⭐
- [ ] **任务开始时间**: _______
- [ ] **预计完成时间**: _______ (5-7天)
- [ ] **实施人**: _______
- [ ] **是否需要**: ⚠️ 待评估

**子任务**:
- [ ] 需求确认 (项目是否需要 DB)
- [ ] axmol DragonBones 库集成评估
- [ ] GLoader3D 扩展实现
- [ ] 加载/播放/释放完整流程
- [ ] 测试验证

**影响**: DragonBones 动画支持 (可选)

---

## 📊 进度统计

### 按优先级
| 优先级 | 总数 | 已完成 | 进行中 | 未开始 | 完成率 |
|-------|------|-------|--------|--------|--------|
| ✅ 已完成 | 1 | 1 | 0 | 0 | 100% |
| 🔴 P0 | 2 | 0 | 0 | 2 | 0% |
| 🟡 P1 | 4 | 0 | 0 | 4 | 0% |
| 🟢 P2 | 4 | 0 | 0 | 4 | 0% |
| **总计** | **11** | **1** | **0** | **10** | **9%** |

### 按工作量
| 阶段 | 任务数 | 预计天数 | 完成率 |
|------|--------|----------|--------|
| 已完成 | 1 | - | 100% |
| P0 关键 | 2 | 5-8 | 0% |
| P1 重要 | 4 | 8-12 | 0% |
| P2 可选 | 4 | 9-13 | 0% |
| **总计** | **11** | **22-33** | **9%** |

---

## 🚀 建议实施顺序

### 第一周 (P0 关键路径)
1. **Day 1-3**: P0.1 ignoreEngineTimeScale
2. **Day 4-5**: P0.2 IAnimationGear 完善

### 第二周 (P1 核心优化)
3. **Day 1-3**: P1.2 CustomEase 支持
4. **Day 4-5**: P1.3 IColorGear 完善

### 第三周 (P1 剩余 + P2 选择)
5. **Day 1-4**: P1.1 invalidateBatchingEveryFrame
6. **Day 5**: P1.4 GLoader 外部加载钩子 (快速)

### 第四周 (P2 可选,按需)
7. **Day 1**: P2.2 useResize (快速)
8. **Day 2-3**: P2.1 showErrorSign
9. **Day 4-5**: P2.3 GLoader3D 时间控制

### 后续 (低优先级)
10. **按需**: P2.4 DragonBones 支持 (需先评估必要性)

---

## 📝 每日更新模板

```markdown
### YYYY-MM-DD 工作日志

**任务**: [任务编号] 任务名称

**今日完成**:
- [ ] 子任务 1
- [ ] 子任务 2

**遇到的问题**:
- 问题描述
- 解决方案/待解决

**明日计划**:
- [ ] 待完成任务

**备注**:
- 其他说明
```

---

## 🔔 里程碑

- [ ] **里程碑 1**: P0 功能完成 (预计 1 周)
  - 达成标准: ignoreEngineTimeScale 和 IAnimationGear 完整测试通过

- [ ] **里程碑 2**: P1 功能完成 (预计 3 周)
  - 达成标准: 所有 P1 功能实现并通过测试

- [ ] **里程碑 3**: 功能对等验证 (预计 4 周)
  - 达成标准: 与 Unity 版本 95%+ 功能对等

---

**文档维护**: 每日更新任务状态,每周更新进度统计
