/****************************************************************************
 CO2 Effect Test Example
 ****************************************************************************/

#include "axmol.h"
#include "co2effect/CO2EffectPlayer.h"

USING_NS_AX;
using namespace co2effect;

class CO2EffectTestScene : public Scene
{
public:
    static Scene* create()
    {
        auto scene = new CO2EffectTestScene();
        if (scene->init())
        {
            scene->autorelease();
            return scene;
        }
        delete scene;
        return nullptr;
    }

    bool init()
    {
        if (!Scene::init())
            return false;

        auto visibleSize = Director::getInstance()->getVisibleSize();
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        // 背景
        auto bg = LayerColor::create(Color4B(50, 50, 50, 255));
        addChild(bg);

        // 标题
        auto title = Label::createWithSystemFont("CO2 Effect Test", "Arial", 32);
        title->setPosition(Vec2(origin.x + visibleSize.width/2,
                               origin.y + visibleSize.height - 50));
        addChild(title);

        // 创建特效播放器
        _effectPlayer = CO2EffectPlayer::create();
        _effectPlayer->setPosition(Vec2(origin.x + visibleSize.width/2,
                                       origin.y + visibleSize.height/2));
        addChild(_effectPlayer);

        // 设置资源路径
        // 注意: 需要根据实际路径调整
        _effectPlayer->setResourcePath("c3/effect/interface/fgui/slot/cjwp/cjwp_beishukuang/");

        // 加载配置
        if (!_effectPlayer->loadEffectConfig("res/+3DEffect.ini"))
        {
            AXLOG("Failed to load effect config!");
            return true;
        }

        // 创建按钮
        createButtons(origin, visibleSize);

        // 信息标签
        _infoLabel = Label::createWithSystemFont("Ready", "Arial", 20);
        _infoLabel->setPosition(Vec2(origin.x + visibleSize.width/2,
                                    origin.y + 100));
        addChild(_infoLabel);

        return true;
    }

    void createButtons(const Vec2& origin, const Size& visibleSize)
    {
        // 播放按钮
        auto playBtn = ui::Button::create();
        playBtn->setTitleText("Play Effect");
        playBtn->setTitleFontSize(24);
        playBtn->setPosition(Vec2(origin.x + visibleSize.width/2 - 150,
                                 origin.y + 50));
        playBtn->addClickEventListener([this](Ref*) {
            playEffect();
        });
        addChild(playBtn);

        // 停止按钮
        auto stopBtn = ui::Button::create();
        stopBtn->setTitleText("Stop");
        stopBtn->setTitleFontSize(24);
        stopBtn->setPosition(Vec2(origin.x + visibleSize.width/2,
                                 origin.y + 50));
        stopBtn->addClickEventListener([this](Ref*) {
            _effectPlayer->stop();
            _infoLabel->setString("Stopped");
        });
        addChild(stopBtn);

        // 暂停按钮
        auto pauseBtn = ui::Button::create();
        pauseBtn->setTitleText("Pause/Resume");
        pauseBtn->setTitleFontSize(24);
        pauseBtn->setPosition(Vec2(origin.x + visibleSize.width/2 + 150,
                                  origin.y + 50));
        pauseBtn->addClickEventListener([this](Ref*) {
            static bool paused = false;
            paused = !paused;
            _effectPlayer->setPaused(paused);
            _infoLabel->setString(paused ? "Paused" : "Resumed");
        });
        addChild(pauseBtn);
    }

    void playEffect()
    {
        // 获取所有可用特效
        // 注意: 需要根据实际配置文件中的特效名称调整
        std::string effectName = "test_effect";  // 替换为实际特效名称

        bool success = _effectPlayer->playEffect(effectName, [this]() {
            _infoLabel->setString("Effect Finished!");
            AXLOG("Effect playback completed");
        });

        if (success)
        {
            _infoLabel->setString(StringUtils::format("Playing: %s", effectName.c_str()));
        }
        else
        {
            _infoLabel->setString("Failed to play effect!");
        }
    }

private:
    CO2EffectPlayer* _effectPlayer;
    Label* _infoLabel;
};

// 在AppDelegate中使用:
// auto scene = CO2EffectTestScene::create();
// director->runWithScene(scene);
