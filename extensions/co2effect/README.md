# CO2 Effect Extension for Axmol

Conquer Online 2 特效系统移植，支持加载和播放C3格式的3D序列帧特效。

## 功能特性

- ✅ **C3文件解析** - 支持PHY4(网格)、MOTI(动画)、PTCL(粒子)、SHAP(形状)等Chunk
- ✅ **EFFE配置读取** - 解析INI格式的特效配置文件
- ✅ **序列帧播放** - 支持循环、帧间隔、延迟等参数
- ✅ **混合模式** - D3D混合模式自动映射到axmol BlendFunc
- ⚠️ **3D渲染** - 基础框架已实现，需要扩展Sprite3D渲染
- ⚠️ **粒子系统** - 需要实现自定义Particle3DEmitter

## 快速开始

### 1. 添加到CMake

```cmake
# 在你的CMakeLists.txt中添加
add_subdirectory(extensions/co2effect)

# 链接到你的目标
target_link_libraries(YourTarget co2effect)
```

### 2. 基础使用

```cpp
#include "co2effect/CO2EffectPlayer.h"

using namespace ax::co2effect;

// 创建播放器
auto player = CO2EffectPlayer::create();
player->setPosition(Vec2(400, 300));
addChild(player);

// 设置资源路径
player->setResourcePath("c3/effect/interface/fgui/slot/cjwp/cjwp_beishukuang/");

// 加载配置
player->loadEffectConfig("+3DEffect.ini");

// 播放特效
player->playEffect("effect_name", []() {
    AXLOG("特效播放完成!");
});
```

### 3. 与FairyGUI集成

```cpp
// 在GLoader中添加C3协议支持
// 参考: extensions/fairygui/src/fairygui/GLoader.cpp

// 检测 c3:// 协议
if (url.compare(0, 5, "c3://") == 0)
{
    std::string effectName = url.substr(5);

    // 创建CO2特效播放器
    auto player = CO2EffectPlayer::create();
    player->setResourcePath("c3/effect/");
    player->loadEffectConfig("res/+3DEffect.ini");
    player->playEffect(effectName);

    _content = player;
    _content->setAnchorPoint(Vec2(0.5f, 0.5f));
    _displayObject->addChild(_content);

    return true;
}
```

## 文件格式

### C3文件结构

```
文件头: "MAXFILE C3 00001" (16字节)

Chunk结构 (重复):
  ChunkID: 4字节 ASCII ("PHY4", "MOTI", "PTCL", "SHAP")
  ChunkSize: 4字节 uint32
  ChunkData: 数据内容
```

### EFFE配置格式(INI)

```ini
[effect_name]
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
ColorEnable=0
Lev=0
```

## API参考

### C3FileReader

```cpp
// 读取C3文件
C3FileData data;
if (C3FileReader::readC3File("path/to/file.c3", data))
{
    // data.meshes - 网格数据
    // data.motions - 动画数据
    // data.particles - 粒子数据
    // data.shapes - 形状数据
}
```

### EffectConfigReader

```cpp
EffectConfigReader reader;
reader.loadFromFile("+3DEffect.ini");

auto config = reader.getEffectConfig("effect_name");
// config->parts - Part数组
// config->loopTime - 循环次数
// config->frameInterval - 帧间隔(ms)
```

### CO2EffectPlayer

```cpp
auto player = CO2EffectPlayer::create();
player->setResourcePath("c3/effect/");
player->loadEffectConfig("+3DEffect.ini");

// 播放特效
player->playEffect("effect_name", []() {
    // 完成回调
});

// 控制播放
player->stop();
player->setPaused(true);
bool playing = player->isPlaying();
```

## 数据结构映射

### C3顶点 → axmol格式

| C3 | axmol | 说明 |
|----|-------|------|
| x,y,z | Vec3 position | 位置 |
| color(DWORD) | Vec4 color | ARGB→RGBA |
| u,v | Vec2 texcoord | UV坐标 |
| index[2] | boneIndices[2] | 骨骼索引 |
| weight[2] | boneWeights[2] | 骨骼权重 |

### D3D混合模式 → axmol

| D3D | 值 | axmol BlendFactor |
|-----|----|--------------------|
| D3DBLEND_ZERO | 1 | ZERO |
| D3DBLEND_ONE | 2 | ONE |
| D3DBLEND_SRCALPHA | 5 | SRC_ALPHA |
| D3DBLEND_INVSRCALPHA | 6 | ONE_MINUS_SRC_ALPHA |

## 扩展开发

### 实现3D网格渲染

当前`updateFrame()`方法只是显示占位文本，需要实现实际的3D渲染：

```cpp
void CO2EffectPlayer::updateFrame()
{
    auto& frameData = _frameDataCache[_currentPartIndex];

    // 1. 将C3网格转换为Sprite3D或自定义MeshRenderer
    for (auto& mesh : frameData.meshes)
    {
        // 创建VertexBuffer
        // 创建IndexBuffer
        // 加载纹理
        // 创建Sprite3D或MeshRenderer
        // 应用混合模式
    }

    // 2. 处理粒子系统
    for (auto& particle : frameData.particles)
    {
        // 创建自定义Particle3DEmitter
        // 设置预烘焙的粒子位置/大小/年龄
    }
}
```

### 实现粒子系统

```cpp
class CO2ParticleEmitter : public Particle3DEmitter
{
private:
    C3ParticleData _particleData;
    int _currentFrame;

public:
    void update(float dt) override
    {
        // 根据时间切换到对应帧
        auto& frame = _particleData.frames[_currentFrame];

        // 更新粒子池
        for (int i = 0; i < frame.particleCount; ++i)
        {
            auto particle = getParticle(i);
            particle->position = frame.positions[i];
            particle->width = particle->height = frame.sizes[i];
            // ... 更新其他属性
        }
    }
};
```

## TODO

- [ ] 实现完整的3D网格渲染
- [ ] 实现自定义粒子系统Emitter
- [ ] 支持形状/线条渲染
- [ ] 支持骨骼动画插值
- [ ] 纹理缓存管理
- [ ] 性能优化(批处理、实例化)

## 许可证

MIT License - 与axmol主项目相同

## 参考资料

- Conquer Online 2 源码分析
- Windsoul++ 游戏引擎
- axmol 3D系统文档
