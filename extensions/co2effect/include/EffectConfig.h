/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#ifndef __CO2EFFECT_EFFECT_CONFIG_H__
#define __CO2EFFECT_EFFECT_CONFIG_H__

#include "cocos2d.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace ax {
namespace co2effect {

/**
 * 特效配置Part(一个Part对应一个C3文件)
 */
struct EffectPart
{
    int effectId;       // C3模型ID (effect/1000.c3)
    int textureId;      // 纹理ID (texture/1000.pvr)
    int asb;            // Alpha源混合模式
    int adb;            // Alpha目标混合模式
};

/**
 * 特效配置数据
 */
struct EffectConfig
{
    std::string name;               // 特效名称
    int partCount;                  // Part数量(序列帧数量)
    std::vector<EffectPart> parts;  // Part数组

    int delay;                      // 延迟(毫秒)
    int loopTime;                   // 循环次数(-1=无限循环)
    int frameInterval;              // 帧间隔(毫秒)
    int loopInterval;               // 循环间隔(毫秒)
    int offsetX;                    // X偏移
    int offsetY;                    // Y偏移
    int offsetZ;                    // Z偏移
    int colorEnable;                // 颜色启用
    int level;                      // 等级
};

/**
 * EFFE配置文件读取器
 *
 * 读取+3DEffect.ini配置文件
 * 格式示例:
 * [effect_name]
 * Amount=3
 * EffectId0=1000
 * TextureId0=1000
 * Asb0=5
 * Adb0=6
 * ...
 * Delay=0
 * LoopTime=1
 * FrameInterval=33
 * ...
 */
class EffectConfigReader
{
public:
    EffectConfigReader();
    ~EffectConfigReader();

    /**
     * 加载EFFE配置文件
     * @param filepath INI文件路径(例如: +3DEffect.ini)
     * @return 成功返回true
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * 获取特效配置
     * @param name 特效名称
     * @return 配置指针,不存在返回nullptr
     */
    const EffectConfig* getEffectConfig(const std::string& name) const;

    /**
     * 获取所有特效名称
     */
    std::vector<std::string> getAllEffectNames() const;

    /**
     * 清空数据
     */
    void clear();

private:
    // 解析INI文件
    bool parseINI(const std::string& content);

    // 读取一个Section(特效)
    bool parseEffectSection(const std::string& sectionName,
                           const std::unordered_map<std::string, std::string>& properties);

    // 辅助函数: 获取int值
    int getInt(const std::unordered_map<std::string, std::string>& props,
               const std::string& key, int defaultValue = 0) const;

private:
    std::unordered_map<std::string, EffectConfig> _effects;
};

} // namespace co2effect
} // namespace ax

#endif // __CO2EFFECT_EFFECT_CONFIG_H__
