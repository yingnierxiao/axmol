/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#include "CO2EffectPlayer.h"
#include "cocos2d.h"

namespace ax {
namespace co2effect {

using ax::Director;
using ax::FileUtils;
using ax::Label;
using ax::Vec2;
using ax::backend::BlendFactor;

// 全局配置
static std::string s_globalConfigPath;
static std::string s_globalResourcePath;
static std::string s_globalWDBPath;

void CO2EffectPlayer::setGlobalConfig(const std::string& configPath,
                                     const std::string& resourcePath,
                                     const std::string& wdbPath)
{
    s_globalConfigPath = configPath;
    s_globalResourcePath = resourcePath;
    s_globalWDBPath = wdbPath;
    AXLOG("CO2EffectPlayer: Global config set - config=%s, resource=%s, wdb=%s",
          configPath.c_str(), resourcePath.c_str(), wdbPath.c_str());
}

const std::string& CO2EffectPlayer::getGlobalConfigPath()
{
    return s_globalConfigPath;
}

const std::string& CO2EffectPlayer::getGlobalResourcePath()
{
    return s_globalResourcePath;
}

const std::string& CO2EffectPlayer::getGlobalWDBPath()
{
    return s_globalWDBPath;
}

CO2EffectPlayer::CO2EffectPlayer()
    : _isPlaying(false)
    , _isPaused(false)
    , _currentEffect(nullptr)
    , _currentPartIndex(0)
    , _currentLoop(0)
    , _elapsedTime(0.0f)
    , _renderNode(nullptr)
{
}

CO2EffectPlayer::~CO2EffectPlayer()
{
}

bool CO2EffectPlayer::init()
{
    if (!Node::init())
        return false;

    // 创建渲染容器
    _renderNode = Node::create();
    addChild(_renderNode);

    // 启用update
    scheduleUpdate();

    return true;
}

void CO2EffectPlayer::setResourcePath(const std::string& path)
{
    _resourcePath = path;
    if (!_resourcePath.empty() && _resourcePath.back() != '/')
    {
        _resourcePath += '/';
    }
}

bool CO2EffectPlayer::loadEffectConfig(const std::string& configPath)
{
    // 使用全局WDB路径(如果有配置)
    std::string wdbPath = s_globalWDBPath;

    if (!_configReader.loadFromFile(configPath, wdbPath))
    {
        AXLOG("CO2EffectPlayer: Failed to load config: %s", configPath.c_str());
        return false;
    }

    AXLOG("CO2EffectPlayer: Loaded effect config, %d effects available",
          (int)_configReader.getAllEffectNames().size());
    return true;
}

bool CO2EffectPlayer::playEffect(const std::string& effectName, const FinishCallback& callback)
{
    // 停止当前播放
    stop();

    // 获取配置
    _currentEffect = _configReader.getEffectConfig(effectName);
    if (!_currentEffect)
    {
        AXLOG("CO2EffectPlayer: Effect '%s' not found", effectName.c_str());
        return false;
    }

    // 初始化播放状态
    _currentPartIndex = 0;
    _currentLoop = 0;
    _elapsedTime = 0.0f;
    _finishCallback = callback;
    _isPlaying = true;
    _isPaused = false;

    // 预加载所有帧数据
    _frameDataCache.clear();
    _frameDataCache.resize(_currentEffect->partCount);

    for (int i = 0; i < _currentEffect->partCount; ++i)
    {
        if (!loadFrameC3(_currentEffect->parts[i].effectId, _frameDataCache[i]))
        {
            AXLOG("CO2EffectPlayer: Failed to load frame %d (effectId=%d)",
                  i, _currentEffect->parts[i].effectId);
            stop();
            return false;
        }
    }

    // 显示第一帧
    updateFrame();

    AXLOG("CO2EffectPlayer: Started playing effect '%s', %d frames, loop=%d",
          effectName.c_str(), _currentEffect->partCount, _currentEffect->loopTime);

    return true;
}

void CO2EffectPlayer::stop()
{
    _isPlaying = false;
    _isPaused = false;
    _currentEffect = nullptr;
    _currentPartIndex = 0;
    _currentLoop = 0;
    _elapsedTime = 0.0f;
    _frameDataCache.clear();

    clearCurrentFrame();

    if (_finishCallback)
    {
        _finishCallback();
        _finishCallback = nullptr;
    }
}

void CO2EffectPlayer::setPaused(bool paused)
{
    _isPaused = paused;
}

void CO2EffectPlayer::update(float dt)
{
    Node::update(dt);

    if (!_isPlaying || _isPaused || !_currentEffect)
        return;

    _elapsedTime += dt * 1000.0f; // 转换为毫秒

    // 计算应该播放的帧
    float frameInterval = static_cast<float>(_currentEffect->frameInterval);
    if (frameInterval <= 0)
        frameInterval = 33.0f; // 默认30fps

    if (_elapsedTime >= frameInterval)
    {
        _elapsedTime -= frameInterval;

        // 切换到下一帧
        _currentPartIndex++;

        if (_currentPartIndex >= _currentEffect->partCount)
        {
            // 一轮播放完毕
            _currentPartIndex = 0;
            _currentLoop++;

            // 检查是否需要停止
            if (_currentEffect->loopTime > 0 && _currentLoop >= _currentEffect->loopTime)
            {
                stop();
                return;
            }

            // 循环间隔
            if (_currentEffect->loopInterval > 0)
            {
                _elapsedTime -= _currentEffect->loopInterval;
            }
        }

        updateFrame();
    }
}

bool CO2EffectPlayer::loadFrameC3(int effectId, C3FileData& outData)
{
    // 构建C3文件路径: resourcePath + effectId + ".c3"
    std::string filepath = _resourcePath + std::to_string(effectId) + ".c3";

    if (!C3FileReader::readC3File(filepath, outData))
    {
        AXLOG("CO2EffectPlayer: Failed to load C3 file: %s", filepath.c_str());
        return false;
    }

    return true;
}

void CO2EffectPlayer::clearCurrentFrame()
{
    if (_renderNode)
    {
        _renderNode->removeAllChildren();
    }
}

void CO2EffectPlayer::updateFrame()
{
    if (!_currentEffect || _currentPartIndex >= (int)_frameDataCache.size())
        return;

    clearCurrentFrame();

    auto& frameData = _frameDataCache[_currentPartIndex];
    auto& part = _currentEffect->parts[_currentPartIndex];

    // 设置混合模式
    ax::BlendFunc blendFunc;
    blendFunc.src = mapBlendFactor(part.asb);
    blendFunc.dst = mapBlendFactor(part.adb);

    // TODO: 这里需要根据C3数据创建实际的渲染对象
    // 目前先实现一个简单的占位渲染(显示文本)
    // 完整实现需要:
    // 1. 将C3网格数据转换为Sprite3D或MeshRenderer
    // 2. 加载纹理
    // 3. 应用混合模式
    // 4. 处理粒子系统(如果有)

    // 临时实现: 显示帧信息
    auto label = Label::createWithSystemFont(
        ax::StringUtils::format("Frame %d/%d\nEffect: %s",
                          _currentPartIndex + 1,
                          _currentEffect->partCount,
                          _currentEffect->name.c_str()),
        "Arial", 20);

    label->setPosition(Vec2::ZERO);
    _renderNode->addChild(label);

    AXLOG("CO2EffectPlayer: Updated to frame %d, effectId=%d, asb=%d, adb=%d",
          _currentPartIndex, part.effectId, part.asb, part.adb);
}

// D3D混合模式到axmol映射
backend::BlendFactor CO2EffectPlayer::mapBlendFactor(int d3dBlendMode)
{
    // D3D混合模式常量:
    // D3DBLEND_ZERO = 1
    // D3DBLEND_ONE = 2
    // D3DBLEND_SRCCOLOR = 3
    // D3DBLEND_INVSRCCOLOR = 4
    // D3DBLEND_SRCALPHA = 5
    // D3DBLEND_INVSRCALPHA = 6
    // D3DBLEND_DESTALPHA = 7
    // D3DBLEND_INVDESTALPHA = 8
    // D3DBLEND_DESTCOLOR = 9
    // D3DBLEND_INVDESTCOLOR = 10

    switch (d3dBlendMode)
    {
    case 1:  return backend::BlendFactor::ZERO;
    case 2:  return backend::BlendFactor::ONE;
    case 3:  return backend::BlendFactor::SRC_COLOR;
    case 4:  return backend::BlendFactor::ONE_MINUS_SRC_COLOR;
    case 5:  return backend::BlendFactor::SRC_ALPHA;
    case 6:  return backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
    case 7:  return backend::BlendFactor::DST_ALPHA;
    case 8:  return backend::BlendFactor::ONE_MINUS_DST_ALPHA;
    case 9:  return backend::BlendFactor::DST_COLOR;
    case 10: return backend::BlendFactor::ONE_MINUS_DST_COLOR;
    default: return backend::BlendFactor::SRC_ALPHA;
    }
}

} // namespace co2effect
} // namespace ax
