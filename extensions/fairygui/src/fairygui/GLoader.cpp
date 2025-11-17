#include "GLoader.h"
#include "GComponent.h"
#include "GMovieClip.h"
#include "UIPackage.h"
#include "display/FUISprite.h"
#include "utils/ByteBuffer.h"
#include "utils/ToolSet.h"
#include "spine/spine-cocos2dx.h"
#include "CO2EffectPlayer.h"
#include <unordered_map>

NS_FGUI_BEGIN
using namespace ax;

// Spine资源路径映射表（从spine.ini加载）
static std::unordered_map<std::string, std::string> g_spinePathMap;
static bool g_spineConfigLoaded = false;

// 加载spine.ini配置文件
static void loadSpineConfig()
{
    if (g_spineConfigLoaded)
        return;

    std::string configPath = "ini/spine.ini";
    std::string fullPath = FileUtils::getInstance()->fullPathForFilename(configPath);

    if (fullPath.empty() || fullPath == configPath)
    {
        AXLOG("[GLoader] spine.ini not found: %s", configPath.c_str());
        g_spineConfigLoaded = true;
        return;
    }

    std::string content = FileUtils::getInstance()->getStringFromFile(fullPath);
    if (content.empty())
    {
        AXLOG("[GLoader] Failed to read spine.ini: %s", fullPath.c_str());
        g_spineConfigLoaded = true;
        return;
    }

    // 解析INI文件（key=value格式）
    size_t pos = 0;
    size_t lineCount = 0;
    while (pos < content.length())
    {
        size_t endPos = content.find('\n', pos);
        if (endPos == std::string::npos)
            endPos = content.length();

        std::string line = content.substr(pos, endPos - pos);

        // 移除回车符
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // 跳过空行和注释
        if (!line.empty() && line[0] != '#' && line[0] != ';')
        {
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos)
            {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);

                // 去除首尾空格
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (!key.empty() && !value.empty())
                {
                    g_spinePathMap[key] = value;
                    lineCount++;
                }
            }
        }

        pos = endPos + 1;
    }

    AXLOG("[GLoader] Loaded spine.ini: %zu entries", lineCount);
    g_spineConfigLoaded = true;
}

// 解析spine://URL并返回实际文件路径
static std::string resolveSpineURL(const std::string& url)
{
    // 确保配置已加载
    if (!g_spineConfigLoaded)
        loadSpineConfig();

    // 解析spine://协议
    if (url.compare(0, 8, "spine://") != 0)
        return "";

    std::string resourceName = url.substr(8);
    auto it = g_spinePathMap.find(resourceName);
    if (it == g_spinePathMap.end())
    {
        AXLOG("[GLoader] Spine resource not found in config: %s", resourceName.c_str());
        return "";
    }

    // 返回路径（不需要加res/前缀，因为已经在搜索路径中）
    return it->second;
}

GLoader::GLoader()
    : _autoSize(false),
    _align(TextHAlignment::LEFT),
    _verticalAlign(TextVAlignment::TOP),
    _fill(LoaderFillType::NONE),
    _shrinkOnly(false),
    _updatingLayout(false),
    _contentItem(nullptr),
    _contentStatus(0),
    _content(nullptr),
    _content2(nullptr),
    _playAction(nullptr),
    _playing(true),
    _frame(0)
{
}

GLoader::~GLoader()
{
    AX_SAFE_RELEASE(_playAction);
    AX_SAFE_RELEASE(_content);
    AX_SAFE_RELEASE(_content2);
}

void GLoader::handleInit()
{
    _content = FUISprite::create();
    _content->retain();
    _content->setAnchorPoint(Vec2::ZERO);
    _content->setCascadeOpacityEnabled(true);

    FUIContainer* c = FUIContainer::create();
    c->retain();
    c->gOwner = this;

    _displayObject = c;
    _displayObject->addChild(_content);
}

void GLoader::setURL(const std::string& value)
{
    if (_url.compare(value) == 0)
        return;

    _url = value;
    loadContent();
    updateGear(7);
}

void GLoader::setAlign(ax::TextHAlignment value)
{
    if (_align != value)
    {
        _align = value;
        updateLayout();
    }
}

void GLoader::setVerticalAlign(ax::TextVAlignment value)
{
    if (_verticalAlign != value)
    {
        _verticalAlign = value;
        updateLayout();
    }
}

void GLoader::setAutoSize(bool value)
{
    if (_autoSize != value)
    {
        _autoSize = value;
        updateLayout();
    }
}

void GLoader::setFill(LoaderFillType value)
{
    if (_fill != value)
    {
        _fill = value;
        updateLayout();
    }
}

void GLoader::setShrinkOnly(bool value)
{
    if (_shrinkOnly != value)
    {
        _shrinkOnly = value;
        updateLayout();
    }
}

const ax::Size & GLoader::getContentSize()
{
    return _content->getContentSize();
}

ax::Color3B GLoader::getColor() const
{
    return _content->getColor();
}

void GLoader::setColor(const ax::Color3B& value)
{
    _content->setColor(value);
}

void GLoader::setPlaying(bool value)
{
    if (_playing != value)
    {
        _playing = value;
        if (_playAction)
        {
            if (_playing)
                _content->runAction(_playAction);
            else
                _content->stopAction(_playAction);
        }
        updateGear(5);
    }
}

int GLoader::getFrame() const
{
    return _frame;
}

void GLoader::setFrame(int value)
{
    if (_frame != value)
    {
        _frame = value;
        if (_playAction)
            _playAction->setFrame(value);
        updateGear(5);
    }
}

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

FillMethod GLoader::getFillMethod() const
{
    return _content->getFillMethod();
}

void GLoader::setFillMethod(FillMethod value)
{
    _content->setFillMethod(value);
}

FillOrigin GLoader::getFillOrigin() const
{
    return _content->getFillOrigin();
}

void GLoader::setFillOrigin(FillOrigin value)
{
    _content->setFillOrigin(value);
}

bool GLoader::isFillClockwise() const
{
    return _content->isFillClockwise();
}

void GLoader::setFillClockwise(bool value)
{
    _content->setFillClockwise(value);
}

float GLoader::getFillAmount() const
{
    return _content->getFillAmount();
}

void GLoader::setFillAmount(float value)
{
    _content->setFillAmount(value);
}

void GLoader::loadContent()
{
    clearContent();

    if (_url.length() == 0)
        return;

    if (_url.compare(0, 5, "ui://") == 0)
        loadFromPackage();
    else
    {
        _contentStatus = 3;
        loadExternal();
    }
}

void GLoader::loadFromPackage()
{
    _contentItem = UIPackage::getItemByURL(_url);

    if (_contentItem != nullptr)
    {
        _contentItem = _contentItem->getBranch();
        if (_contentItem == nullptr) {
            AXLOGW("FairyGUI: getBranch() returned nullptr in GLoader::loadFromPackage");
            return;
        }
        sourceSize.width = _contentItem->width;
        sourceSize.height = _contentItem->height;
        _contentItem = _contentItem->getHighResolution();
        _contentItem->load();

        if (_contentItem->type == PackageItemType::IMAGE)
        {
            _contentStatus = 1;
            _content->initWithSpriteFrame(_contentItem->spriteFrame);
            if (_contentItem->scale9Grid)
                ((FUISprite*)_content)->setScale9Grid(_contentItem->scale9Grid);
            updateLayout();
        }
        else if (_contentItem->type == PackageItemType::MOVIECLIP)
        {
            _contentStatus = 2;
            if (_playAction == nullptr)
            {
                _playAction = ActionMovieClip::create(_contentItem->animation, _contentItem->repeatDelay);
                _playAction->retain();
            }
            else
                _playAction->setAnimation(_contentItem->animation, _contentItem->repeatDelay);
            if (_playing)
                _content->runAction(_playAction);
            else
                _playAction->setFrame(_frame);

            updateLayout();
        }
        else if (_contentItem->type == PackageItemType::COMPONENT)
        {
            GObject* obj = UIPackage::createObjectFromURL(_url);
            if (obj == nullptr)
                setErrorState();
            else if (dynamic_cast<GComponent*>(obj) == nullptr)
            {
                setErrorState();
            }
            else
            {
                _content2 = obj->as<GComponent>();
                _content2->retain();
                _content2->addEventListener(UIEventType::SizeChange, [this](EventContext*) {
                    if (!_updatingLayout)
                        updateLayout();
                });
                _displayObject->addChild(_content2->displayObject());
                updateLayout();
            }
        }
        else
        {
            if (_autoSize)
                setSize(_contentItem->width, _contentItem->height);

            setErrorState();
        }
    }
    else
        setErrorState();
}

void GLoader::loadExternal()
{
    // 处理effect://协议 (CO2特效系统)
    if (_url.compare(0, 9, "effect://") == 0)
    {
        std::string effectName = _url.substr(9);
        AXLOG("[GLoader] Loading CO2 effect: %s", effectName.c_str());

        // 创建CO2特效播放器
        auto player = co2effect::CO2EffectPlayer::create();
        if (!player)
        {
            AXLOG("[GLoader] Failed to create CO2EffectPlayer");
            onExternalLoadFailed();
            return;
        }

        // 使用全局配置或默认路径
        std::string configPath = co2effect::CO2EffectPlayer::getGlobalConfigPath();
        std::string resourcePath = co2effect::CO2EffectPlayer::getGlobalResourcePath();

        if (configPath.empty())
        {
            configPath = "ini/+3DEffect.ini";
            AXLOG("[GLoader] Using default config path: %s", configPath.c_str());
        }

        if (resourcePath.empty())
        {
            resourcePath = "c3/effect/";
            AXLOG("[GLoader] Using default resource path: %s", resourcePath.c_str());
        }

        // 设置资源路径
        player->setResourcePath(resourcePath);

        // 加载特效配置
        if (!player->loadEffectConfig(configPath))
        {
            AXLOG("[GLoader] Failed to load effect config for: %s", effectName.c_str());
            onExternalLoadFailed();
            return;
        }

        // 播放特效
        if (!player->playEffect(effectName))
        {
            AXLOG("[GLoader] Failed to play effect: %s", effectName.c_str());
            onExternalLoadFailed();
            return;
        }

        _contentStatus = 6; // 使用新的状态码表示CO2特效内容

        // 隐藏默认的sprite内容
        _content->setVisible(false);

        // 添加特效播放器到displayObject
        player->setAnchorPoint(Vec2(0.5f, 0.5f));
        _displayObject->addChild(player);
        player->setTag(9998); // 使用特殊tag标识CO2特效节点

        // 设置sourceSize (使用默认尺寸或从配置读取)
        sourceSize.width = 200;  // 默认尺寸
        sourceSize.height = 200;

        updateLayout();
        AXLOG("[GLoader] CO2 effect loaded successfully: %s", effectName.c_str());
        return;
    }

    // 处理spine://协议
    if (_url.compare(0, 8, "spine://") == 0)
    {
        std::string spinePath = resolveSpineURL(_url);
        if (spinePath.empty())
        {
            AXLOG("[GLoader] resolveSpineURL returned empty for: %s", _url.c_str());
            onExternalLoadFailed();
            return;
        }

        AXLOG("[GLoader] Resolved spine path: %s", spinePath.c_str());

        // 参考GLoader3D的实现，加载Spine动画
        // 获取文件名（路径的最后一部分）
        size_t lastSlash = spinePath.find_last_of("/\\");
        std::string fileName = (lastSlash != std::string::npos) ? spinePath.substr(lastSlash + 1) : spinePath;

        // 构建完整的文件路径（假设文件在同名子目录中）
        std::string basePath = spinePath + "/" + fileName;

        AXLOG("[GLoader] Base path: %s", basePath.c_str());

        // 查找atlas文件
        size_t pos = basePath.find_last_of('.');
        std::string atlasFile;
        if (pos != std::string::npos)
        {
            atlasFile = basePath.substr(0, pos + 1) + "atlas";
        }
        else
        {
            // 没有扩展名，尝试.atlas
            atlasFile = basePath + ".atlas";
        }

        // 检查atlas.txt作为备选
        if (!ToolSet::isFileExist(atlasFile))
        {
            if (pos != std::string::npos)
                atlasFile = spinePath.substr(0, pos + 1) + "atlas.txt";
            else
                atlasFile = spinePath + ".atlas.txt";
        }

        AXLOG("[GLoader] Atlas file: %s (exists: %d)", atlasFile.c_str(), ToolSet::isFileExist(atlasFile));

        // 尝试创建Spine动画
        spine::SkeletonAnimation* skeletonAni = nullptr;

        // 检查文件存在性
        std::string jsonFile = (pos != std::string::npos) ? basePath : basePath + ".json";
        std::string skelFile = (pos != std::string::npos) ? basePath : basePath + ".skel";
        bool hasSkel = ToolSet::isFileExist(skelFile);
        bool hasJson = ToolSet::isFileExist(jsonFile);

        AXLOG("[GLoader] Files check - .skel: %d, .json: %d", hasSkel, hasJson);

        std::string spineFile;
        bool isBinaryFormat = false;

        // 确定使用哪个文件
        if (hasSkel)
        {
            spineFile = skelFile;
            isBinaryFormat = true;
            AXLOG("[GLoader] Using .skel file (binary): %s", spineFile.c_str());
        }
        else if (hasJson)
        {
            spineFile = jsonFile;

            // 检查文件头判断是否为二进制格式
            // Spine二进制格式的特征：文件开头不是 '{' 或 '['
            auto fileData = FileUtils::getInstance()->getDataFromFile(spineFile);
            if (fileData.getSize() > 0)
            {
                unsigned char firstByte = fileData.getBytes()[0];
                // JSON格式通常以 '{' (0x7B) 或 '[' (0x5B) 开头，或空白字符
                // 二进制格式通常以其他字节开头
                if (firstByte != '{' && firstByte != '[' && firstByte != ' ' &&
                    firstByte != '\t' && firstByte != '\n' && firstByte != '\r')
                {
                    isBinaryFormat = true;
                    AXLOG("[GLoader] Detected binary format in .json file (first byte: 0x%02X)", firstByte);
                }
                else
                {
                    AXLOG("[GLoader] Detected JSON text format (first byte: 0x%02X)", firstByte);
                }
            }
        }
        else
        {
            AXLOG("[GLoader] No spine file found");
            onExternalLoadFailed();
            return;
        }

        // 根据检测结果选择加载方式
        if (isBinaryFormat)
        {
            AXLOG("[GLoader] Loading as binary format...");
            try {
                skeletonAni = spine::SkeletonAnimation::createWithBinaryFile(spineFile, atlasFile);
                AXLOG("[GLoader] Binary format result: %s", skeletonAni ? "success" : "failed");
            } catch (const std::exception& e) {
                std::string errorMsg = e.what();
                AXLOG("[GLoader] Binary format exception: %s", errorMsg.c_str());

                // 检查是否是版本不匹配错误
                if (errorMsg.find("version") != std::string::npos ||
                    errorMsg.find("does not match") != std::string::npos)
                {
                    AXLOG("[GLoader] ========================================");
                    AXLOG("[GLoader] ERROR: Spine version mismatch!");
                    AXLOG("[GLoader] Please re-export spine files with Spine 4.2");
                    AXLOG("[GLoader] File: %s", spineFile.c_str());
                    AXLOG("[GLoader] URL: %s", _url.c_str());
                    AXLOG("[GLoader] ========================================");
                }
            } catch (...) {
                AXLOG("[GLoader] Binary format unknown exception");
            }
        }
        else
        {
            AXLOG("[GLoader] Loading as JSON text format...");
            try {
                skeletonAni = spine::SkeletonAnimation::createWithJsonFile(spineFile, atlasFile);
                AXLOG("[GLoader] JSON format result: %s", skeletonAni ? "success" : "failed");
            } catch (const std::exception& e) {
                std::string errorMsg = e.what();
                AXLOG("[GLoader] JSON format exception: %s", errorMsg.c_str());

                // 检查是否是版本不匹配错误
                if (errorMsg.find("version") != std::string::npos ||
                    errorMsg.find("does not match") != std::string::npos)
                {
                    AXLOG("[GLoader] ========================================");
                    AXLOG("[GLoader] ERROR: Spine version mismatch!");
                    AXLOG("[GLoader] Please re-export spine files with Spine 4.2");
                    AXLOG("[GLoader] File: %s", spineFile.c_str());
                    AXLOG("[GLoader] URL: %s", _url.c_str());
                    AXLOG("[GLoader] ========================================");
                }
            } catch (...) {
                AXLOG("[GLoader] JSON format unknown exception");
            }
        }

        if (skeletonAni)
        {
            _contentStatus = 5; // 使用新的状态码表示Spine内容

            // 隐藏默认的sprite内容
            _content->setVisible(false);

            // 添加Spine节点到displayObject
            _displayObject->addChild(skeletonAni);

            // 将Spine节点保存为用户数据，以便后续管理
            // 使用tag来标识这是一个Spine节点
            skeletonAni->setTag(9999); // 使用特殊tag标识Spine节点

            auto skeleton = skeletonAni->getSkeleton();
            if (skeleton && skeleton->getData())
            {
                // 优先应用通过 setSkinName 设置的皮肤
                if (!_skinName.empty())
                {
                    skeleton->setSkin(_skinName.c_str());
                    skeleton->setSlotsToSetupPose();
                    AXLOG("[GLoader] Applied preset skin: %s", _skinName.c_str());
                }

                // 优先应用通过 setAnimationName 设置的动画
                if (!_animationName.empty())
                {
                    skeletonAni->setAnimation(0, _animationName, true);
                    AXLOG("[GLoader] Applied preset animation: %s", _animationName.c_str());
                }
                else
                {
                    // 如果没有预设动画，自动播放第一个动画
                    auto& animations = skeleton->getData()->getAnimations();
                    if (animations.size() > 0)
                    {
                        std::string firstAnimName = animations[0]->getName().buffer();
                        skeletonAni->setAnimation(0, firstAnimName, true);
                        AXLOG("[GLoader] Auto-playing first animation: %s", firstAnimName.c_str());
                    }
                    else
                    {
                        AXLOG("[GLoader] Warning: No animations found in skeleton");
                    }
                }
            }

            // 获取骨骼尺寸作为sourceSize
            auto bounds = skeletonAni->getBoundingBox();
            sourceSize.width = bounds.size.width;
            sourceSize.height = bounds.size.height;

            updateLayout();
            AXLOG("[GLoader] Spine animation loaded successfully: %s", _url.c_str());
        }
        else
        {
            AXLOG("[GLoader] Failed to load Spine animation: %s", _url.c_str());
            onExternalLoadFailed();
        }
        return;
    }

    // 原有的图片加载逻辑
    auto tex = Director::getInstance()->getTextureCache()->addImage(_url);
    if (tex)
    {
        auto sf = SpriteFrame::createWithTexture(tex, Rect(Vec2::ZERO, tex->getContentSize()));
        onExternalLoadSuccess(sf);
    }
    else
        onExternalLoadFailed();
}

void GLoader::freeExternal(ax::SpriteFrame* spriteFrame)
{
}

void GLoader::onExternalLoadSuccess(ax::SpriteFrame* spriteFrame)
{
    _contentStatus = 4;
    _content->setSpriteFrame(spriteFrame);
    sourceSize = spriteFrame->getRectInPixels().size;
    updateLayout();
}

void GLoader::onExternalLoadFailed()
{
    setErrorState();
}

void GLoader::clearContent()
{
    clearErrorState();

    if (_contentStatus == 4)
        freeExternal(_content->getSpriteFrame());

    if (_contentStatus == 2)
    {
        _playAction->setAnimation(nullptr);
        _content->stopAction(_playAction);
    }

    // 清理Spine内容（状态码5）
    if (_contentStatus == 5)
    {
        // 查找并移除Spine节点（tag=9999）
        auto spineNode = _displayObject->getChildByTag(9999);
        if (spineNode != nullptr)
        {
            _displayObject->removeChild(spineNode);
        }

        // 恢复_content的可见性
        if (_content != nullptr)
        {
            _content->setVisible(true);
        }
    }

    if (_content2 != nullptr)
    {
        _displayObject->removeChild(_content2->displayObject());
        AX_SAFE_RELEASE_NULL(_content2);
    }

    if (_contentStatus != 5) // 只有非Spine内容才调用clearContent
        ((FUISprite*)_content)->clearContent();

    _contentItem = nullptr;
    _contentStatus = 0;
}

void GLoader::updateLayout()
{
    if (_content2 == nullptr && _contentStatus == 0)
    {
        if (_autoSize)
        {
            _updatingLayout = true;
            setSize(50, 30);
            _updatingLayout = false;
        }
        return;
    }

    Size contentSize = sourceSize;

    if (_autoSize)
    {
        _updatingLayout = true;
        if (contentSize.width == 0)
            contentSize.width = 50;
        if (contentSize.height == 0)
            contentSize.height = 30;
        setSize(contentSize.width, contentSize.height);
        _updatingLayout = false;

        if (_size.equals(contentSize))
        {
            if (_content2 != nullptr)
            {
                _content2->setScale(1, 1);
                _content2->setPosition(0, -_size.height);
            }
            else
            {
                _content->setScale(1, 1);
                _content->setAnchorPoint(Vec2::ZERO);
                _content->setPosition(0, 0);
            }
            return;
        }
    }

    float sx = 1, sy = 1;
    if (_fill != LoaderFillType::NONE)
    {
        sx = _size.width / sourceSize.width;
        sy = _size.height / sourceSize.height;

        if (sx != 1 || sy != 1)
        {
            if (_fill == LoaderFillType::SCALE_MATCH_HEIGHT)
                sx = sy;
            else if (_fill == LoaderFillType::SCALE_MATCH_WIDTH)
                sy = sx;
            else if (_fill == LoaderFillType::SCALE)
            {
                if (sx > sy)
                    sx = sy;
                else
                    sy = sx;
            }
            else if (_fill == LoaderFillType::SCALE_NO_BORDER)
            {
                if (sx > sy)
                    sy = sx;
                else
                    sx = sy;
            }

            if (_shrinkOnly)
            {
                if (sx > 1)
                    sx = 1;
                if (sy > 1)
                    sy = 1;
            }
            contentSize.width = floor(sourceSize.width * sx);
            contentSize.height = floor(sourceSize.height * sy);
        }
    }

    if (_content2 != nullptr)
    {
        _content2->setScale(sx, sy);
    }
    else
    {
        if (_contentItem != nullptr)
        {
            if (_contentItem->scale9Grid)
            {
                _content->setScale(1, 1);
                _content->setContentSize(contentSize);
            }
            else if (_contentItem->scaleByTile)
            {
                _content->setScale(1, 1);
                _content->setContentSize(sourceSize);
                _content->setTextureRect(Rect(Vec2::ZERO, contentSize));
            }
            else
            {
                _content->setContentSize(sourceSize);
                _content->setScale(sx, sy);
            }
        }
        else
        {
            _content->setContentSize(sourceSize);
            _content->setScale(sx, sy);
        }
        _content->setAnchorPoint(Vec2::ZERO);
    }

    float nx;
    float ny;
    if (_align == TextHAlignment::CENTER)
        nx = floor((_size.width - contentSize.width) / 2);
    else if (_align == TextHAlignment::RIGHT)
        nx = floor(_size.width - contentSize.width);
    else
        nx = 0;

    if (_content2 != nullptr)
    {
        if (_verticalAlign == TextVAlignment::CENTER)
            ny = floor(-contentSize.height - (_size.height - contentSize.height) / 2);
        else if (_verticalAlign == TextVAlignment::BOTTOM)
            ny = -contentSize.height;
        else
            ny = -_size.height;

        _content2->setPosition(nx, ny);
    }
    else
    {
        if (_verticalAlign == TextVAlignment::CENTER)
            ny = floor((_size.height - contentSize.height) / 2);
        else if (_verticalAlign == TextVAlignment::BOTTOM)
            ny = 0;
        else
            ny = _size.height - contentSize.height;

        _content->setPosition(nx, ny);
    }
}

void GLoader::setErrorState()
{
}

void GLoader::clearErrorState()
{
}

void GLoader::handleSizeChanged()
{
    GObject::handleSizeChanged();

    if (!_updatingLayout)
        updateLayout();
}

void GLoader::handleGrayedChanged()
{
    GObject::handleGrayedChanged();

    ((FUISprite*)_content)->setGrayed(_finalGrayed);
    if (_content2 != nullptr)
        _content2->setGrayed(_finalGrayed);
}

ax::Value GLoader::getProp(ObjectPropID propId)
{
    switch (propId)
    {
    case ObjectPropID::Color:
        return Value(ToolSet::colorToInt(getColor()));
    case ObjectPropID::Playing:
        return Value(isPlaying());
    case ObjectPropID::Frame:
        return Value(getFrame());
    case ObjectPropID::TimeScale:
        if (_playAction)
            return Value(_playAction->getTimeScale());
        else
            return Value(1);
    default:
        return GObject::getProp(propId);
    }
}

void GLoader::setProp(ObjectPropID propId, const ax::Value& value)
{
    switch (propId)
    {
    case ObjectPropID::Color:
        setColor(ToolSet::intToColor(value.asUnsignedInt()));
        break;
    case ObjectPropID::Playing:
        setPlaying(value.asBool());
        break;
    case ObjectPropID::Frame:
        setFrame(value.asInt());
        break;
    case ObjectPropID::TimeScale:
        if (_playAction)
            _playAction->setTimeScale(value.asFloat());
        break;
    case ObjectPropID::DeltaTime:
        if (_playAction)
            _playAction->advance(value.asFloat());
        break;
    default:
        GObject::setProp(propId, value);
        break;
    }
}

void GLoader::setup_beforeAdd(ByteBuffer* buffer, int beginPos)
{
    GObject::setup_beforeAdd(buffer, beginPos);

    buffer->seek(beginPos, 5);

    _url = buffer->readS();
    _align = (TextHAlignment)buffer->readByte();
    _verticalAlign = (TextVAlignment)buffer->readByte();
    _fill = (LoaderFillType)buffer->readByte();
    _shrinkOnly = buffer->readBool();
    _autoSize = buffer->readBool();
    buffer->readBool(); //_showErrorSign
    _playing = buffer->readBool();
    _frame = buffer->readInt();

    if (buffer->readBool())
        setColor((Color3B)buffer->readColor());
    int fillMethod = buffer->readByte();
    if (fillMethod != 0)
    {
        _content->setFillMethod((FillMethod)fillMethod);
        _content->setFillOrigin((FillOrigin)buffer->readByte());
        _content->setFillClockwise(buffer->readBool());
        _content->setFillAmount(buffer->readFloat());
    }

    if (_url.length() > 0)
        loadContent();
}

GObject* GLoader::hitTest(const Vec2& worldPoint, const Camera* camera)
{
    if (!_touchable || !_displayObject->isVisible() || !_displayObject->getParent())
        return nullptr;

    if (_content2 != nullptr)
    {
        GObject* obj = _content2->hitTest(worldPoint, camera);
        if (obj != nullptr)
            return obj;
    }

    Rect rect;
    rect.size = _size;
    //if (isScreenPointInRect(worldPoint, camera, _displayObject->getWorldToNodeTransform(), rect, nullptr))
    if (rect.containsPoint(_displayObject->convertToNodeSpace(worldPoint)))
        return this;
    else
        return nullptr;
}

NS_FGUI_END