/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#include "EffectConfig.h"
#include "cocos2d.h"
#include <sstream>
#include <cctype>

namespace ax {
namespace co2effect {

using ax::FileUtils;

EffectConfigReader::EffectConfigReader() {}
EffectConfigReader::~EffectConfigReader() {}

void EffectConfigReader::clear()
{
    _effects.clear();
}

bool EffectConfigReader::loadFromFile(const std::string& filepath)
{
    clear();

    auto content = FileUtils::getInstance()->getStringFromFile(filepath);
    if (content.empty())
    {
        AXLOG("EffectConfigReader: Failed to read file: %s", filepath.c_str());
        return false;
    }

    return parseINI(content);
}

const EffectConfig* EffectConfigReader::getEffectConfig(const std::string& name) const
{
    auto it = _effects.find(name);
    if (it != _effects.end())
    {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> EffectConfigReader::getAllEffectNames() const
{
    std::vector<std::string> names;
    names.reserve(_effects.size());
    for (auto& pair : _effects)
    {
        names.push_back(pair.first);
    }
    return names;
}

int EffectConfigReader::getInt(const std::unordered_map<std::string, std::string>& props,
                               const std::string& key, int defaultValue) const
{
    auto it = props.find(key);
    if (it != props.end())
    {
        try {
            return std::stoi(it->second);
        }
        catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool EffectConfigReader::parseINI(const std::string& content)
{
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;
    std::unordered_map<std::string, std::string> currentProperties;

    auto processSection = [&]() {
        if (!currentSection.empty())
        {
            parseEffectSection(currentSection, currentProperties);
            currentProperties.clear();
        }
    };

    while (std::getline(stream, line))
    {
        // 去除BOM和首尾空白
        if (!line.empty() && (unsigned char)line[0] == 0xEF)
        {
            if (line.size() >= 3 && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
            {
                line = line.substr(3);
            }
        }

        // 去除首尾空白
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;

        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        // 解析Section [name]
        if (line[0] == '[')
        {
            size_t endBracket = line.find(']');
            if (endBracket != std::string::npos)
            {
                // 保存上一个Section
                processSection();

                // 开始新Section
                currentSection = line.substr(1, endBracket - 1);
            }
            continue;
        }

        // 解析Key=Value
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);

            // 去除key和value的空白
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));

            currentProperties[key] = value;
        }
    }

    // 处理最后一个Section
    processSection();

    AXLOG("EffectConfigReader: Loaded %d effects", (int)_effects.size());
    return !_effects.empty();
}

bool EffectConfigReader::parseEffectSection(const std::string& sectionName,
                                            const std::unordered_map<std::string, std::string>& properties)
{
    EffectConfig config;
    config.name = sectionName;

    // 读取Part数量
    config.partCount = getInt(properties, "Amount", 0);
    if (config.partCount <= 0)
    {
        AXLOG("EffectConfigReader: Effect '%s' has invalid Amount", sectionName.c_str());
        return false;
    }

    // 读取每个Part
    config.parts.resize(config.partCount);
    for (int i = 0; i < config.partCount; ++i)
    {
        auto& part = config.parts[i];
        part.effectId = getInt(properties, "EffectId" + std::to_string(i), 0);
        part.textureId = getInt(properties, "TextureId" + std::to_string(i), 0);
        part.asb = getInt(properties, "Asb" + std::to_string(i), 5);
        part.adb = getInt(properties, "Adb" + std::to_string(i), 6);
    }

    // 读取播放参数
    config.delay = getInt(properties, "Delay", 0);
    config.loopTime = getInt(properties, "LoopTime", 1);
    config.frameInterval = getInt(properties, "FrameInterval", 33);
    config.loopInterval = getInt(properties, "LoopInterval", 0);
    config.offsetX = getInt(properties, "OffsetX", 0);
    config.offsetY = getInt(properties, "OffsetY", 0);
    config.offsetZ = getInt(properties, "OffsetZ", 0);
    config.colorEnable = getInt(properties, "ColorEnable", 0);
    config.level = getInt(properties, "Lev", 0);

    _effects[sectionName] = std::move(config);

    AXLOG("EffectConfigReader: Loaded effect '%s', %d parts, loop=%d, interval=%dms",
          sectionName.c_str(), config.partCount, config.loopTime, config.frameInterval);

    return true;
}

} // namespace co2effect
} // namespace ax
