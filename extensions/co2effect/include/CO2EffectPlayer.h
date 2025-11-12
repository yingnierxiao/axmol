/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#ifndef __CO2EFFECT_PLAYER_H__
#define __CO2EFFECT_PLAYER_H__

#include "cocos2d.h"
#include "C3FileReader.h"
#include "EffectConfig.h"
#include <functional>

namespace ax {
namespace co2effect {

using ax::Node;

/**
 * CO2特效播放器
 *
 * 负责加载和播放C3序列帧特效
 * 使用方式:
 * 1. 设置资源路径 setResourcePath()
 * 2. 加载配置文件 loadEffectConfig()
 * 3. 播放特效 playEffect()
 */
class CO2EffectPlayer : public Node
{
public:
    using FinishCallback = std::function<void()>;

    CREATE_FUNC(CO2EffectPlayer);

    /**
     * 设置全局默认资源路径和配置文件
     * @param configPath INI配置文件路径
     * @param resourcePath C3资源根路径
     * @param wdbPath WDB打包文件路径(可选，例如: "ini/c3.wdb")
     */
    static void setGlobalConfig(const std::string& configPath,
                               const std::string& resourcePath,
                               const std::string& wdbPath = "");

    /**
     * 获取全局配置文件路径
     */
    static const std::string& getGlobalConfigPath();

    /**
     * 获取全局资源路径
     */
    static const std::string& getGlobalResourcePath();

    /**
     * 获取全局WDB路径
     */
    static const std::string& getGlobalWDBPath();

    /**
     * 设置资源根路径
     * @param path 资源路径(例如: "c3/effect/")
     */
    void setResourcePath(const std::string& path);

    /**
     * 加载特效配置文件
     * @param configPath INI配置文件路径(例如: "+3DEffect.ini")
     * @return 成功返回true
     */
    bool loadEffectConfig(const std::string& configPath);

    /**
     * 播放特效
     * @param effectName 特效名称
     * @param callback 播放完成回调
     * @return 成功返回true
     */
    bool playEffect(const std::string& effectName, const FinishCallback& callback = nullptr);

    /**
     * 停止播放
     */
    void stop();

    /**
     * 暂停/恢复
     */
    void setPaused(bool paused);

    /**
     * 是否正在播放
     */
    bool isPlaying() const { return _isPlaying; }

    /**
     * 设置混合模式映射
     * C3使用D3D混合模式(ASB/ADB), 需要映射到axmol的BlendFunc
     */
    static backend::BlendFactor mapBlendFactor(int d3dBlendMode);

    // Node override
    virtual void update(float dt) override;

protected:
    CO2EffectPlayer();
    virtual ~CO2EffectPlayer();

    bool init() override;

private:
    // 加载一帧的C3文件
    bool loadFrameC3(int effectId, C3FileData& outData);

    // 创建/更新当前帧的渲染
    void updateFrame();

    // 清理当前帧
    void clearCurrentFrame();

private:
    std::string _resourcePath;
    EffectConfigReader _configReader;

    // 当前播放状态
    bool _isPlaying;
    bool _isPaused;
    const EffectConfig* _currentEffect;
    int _currentPartIndex;
    int _currentLoop;
    float _elapsedTime;
    FinishCallback _finishCallback;

    // 当前帧数据缓存
    std::vector<C3FileData> _frameDataCache;
    Node* _renderNode;  // 渲染节点容器
};

} // namespace co2effect
} // namespace ax

#endif // __CO2EFFECT_PLAYER_H__
