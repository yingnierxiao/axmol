#include "SDFGen.h"

#include <yasio/singleton.hpp>
#include <yasio/byte_buffer.hpp>
#include "base/format.h"

#include "base/ZipUtils.h"

#include "ImGuiPresenter.h"
#include "misc/cpp/imgui_stdlib.h"
#include <zlib.h>
#include "base/JsonWriter.h"
#include "yasio/utils.hpp"

NS_AX_EXT_BEGIN

struct FontAtlasGenParams
{
    std::string sourceFont;  // font relative path? choose from developer machine?
    std::string fontAsset;   // fontAsset .xasset
    std::string glyphs;      // utf-8
    int faceSize    = 32;
    int atlasDim[2] = {512, 512};  // w,h
    bool useAscii   = true;

    bool saved = false;
    float cost = 0.0f;  // milliseconds
    std::string error;  // save error message if fail
};

/**
 * scan fonts in `Content/fonts`
 * kernings don't store
 * axmol .xasset format spec, sdf only
 * axmol .xasset binary format: \X\A\S
 * {
 *   "version": "2.1.0",
 *   "type": "fontatlas",
 *   "atlasName": "xxx",
 *   "sourceFont: "xxx",
 *   "spread": 6, // reserved
 *   "faceSize": 32,
 *   "atlasSize": [512, 512],
 *   "letters": [
 *     "30": {
 *         "page": 0,
 *         "xAdvance": 33,
 *         "width": 54,
 *         "height": 33,
 *         "offset": 5,
 *         "bearingY": 3,
 *     },
 *     "20": {
 *     }
 *   ]
 *   "pages: [ // zip + base64
 *     "",
 *     ""
 *   ]
 * }
 */

namespace xasset
{
class FontAtlas : public ax::FontAtlas
{
public:
    FontAtlas(Font* theFont, int atlasWidth, int atlasHeight) : ax::FontAtlas(theFont, atlasWidth, atlasHeight) {}
    static FontAtlas* newFontAtlas(FontAtlasGenParams* params)
    {
        auto font      = FontFreeType::create(params->sourceFont, params->faceSize,
                                         !params->useAscii ? ax::GlyphCollection::CUSTOM : ax::GlyphCollection::ASCII,
                                              params->glyphs, true);
        auto fontAtlas = new xasset::FontAtlas(font, params->atlasDim[0], params->atlasDim[1]);

        fontAtlas->generate(params);

        return fontAtlas;
    }

    bool save() { return this->saveAs(_params->fontAsset); }

    bool saveAs(std::string_view path)
    {
        std::string storePath;
        auto fu = FileUtils::getInstance();
        if (fu->isAbsolutePath(path))
        {
            storePath = path;
        }
        else
        {
            storePath = fu->getDefaultResourceRootPath();
            storePath += path;
        }

        AXLOG("=========================================");
        AXLOG("保存字体图集文件:");
        AXLOG("  输入路径: %s", std::string(path).c_str());
        AXLOG("  完整路径: %s", storePath.c_str());

        auto start = yasio::highp_clock();

        JsonWriter<> xasset;

        xasset.writeStartObject();

        // 处理 sourceFont 路径：去掉 "res/" 前缀（如果有的话）
        // 因为运行时会添加 "res" 到搜索路径，所以保存时应该使用相对于搜索路径的路径
        std::string sourceFontPath = _params->sourceFont;
        if (cxx20::starts_with(sourceFontPath, "res/"))
        {
            sourceFontPath = sourceFontPath.substr(4);  // 去掉 "res/"
            AXLOG("  调整 sourceFont: %s -> %s", _params->sourceFont.c_str(), sourceFontPath.c_str());
        }

        xasset.writeString("version"sv, AX_VERSION_STR_FULL);
        xasset.writeString("type"sv, "fontatlas"sv);
        xasset.writeString("sourceFont"sv, sourceFontPath);
        xasset.writeString("atlasName"sv, _atlasName);
        xasset.writeNumber("spread"sv, 6);
        xasset.writeNumber("faceSize"sv, _params->faceSize);

        xasset.writeNumberArray("atlasDim"sv, _params->atlasDim);

        xasset.writeStartObject("letters"sv);
        std::string charCode;
        for (auto& letterInfo : getLetterDefinitions())
        {
            charCode.clear();
            fmt::format_to(charCode, "{}", (int32_t)letterInfo.first);

            xasset.writeStartObject(charCode);
            xasset.writeNumber("U", letterInfo.second.U);
            xasset.writeNumber("V", letterInfo.second.V);
            xasset.writeNumber("width", letterInfo.second.width);
            xasset.writeNumber("height", letterInfo.second.height);
            xasset.writeNumber("offsetX", letterInfo.second.offsetX);
            xasset.writeNumber("offsetY", letterInfo.second.offsetY);
            xasset.writeNumber("page", letterInfo.second.textureID);
            xasset.writeNumber("advance", letterInfo.second.xAdvance);
            xasset.writeEndObject();
        }
        xasset.writeEndObject();

        xasset.writeStartArray("pages");
        for (auto& data : _pageDatas)
        {
            auto compData = ZipUtils::compressGZ(std::span{data});
            auto pixels   = utils::base64Encode(std::span{compData});
            xasset.writeStringValue(pixels);
        }
        xasset.writeEndArray();

        xasset.writeNumber("pageX", _currentPageOrigX);
        xasset.writeNumber("pageY", _currentPageOrigY);

        xasset.writeEndObject();

        auto str = static_cast<std::string_view>(xasset);

        fu->writeStringToFile(str, storePath);

        _params->cost = (yasio::highp_clock() - start) / 1000.0;

        _params->error.clear();
        return true;
    }

    Texture2D* testTexture()
    {
        static Texture2D* texture = new Texture2D();
        static bool inited        = false;
        if (!inited && !_pageDatas.empty())
        {
            texture->initWithData(_pageDatas[0].data(), _pageDatas[0].size(), PixelFormat::R8, 512, 512, false);
            texture->retain();
            inited = true;
        }
        return inited ? texture : nullptr;
    }

protected:
    void generate(FontAtlasGenParams* params)
    {
        _params = params;

        // match with runtime
        // 注意：atlasName 中的路径应该去掉 "res/" 前缀，与运行时匹配
        std::string sourceFontPath = params->sourceFont;
        if (cxx20::starts_with(sourceFontPath, "res/"))
        {
            sourceFontPath = sourceFontPath.substr(4);  // 去掉 "res/"
        }
        _atlasName = fmt::format("df {} {}", params->faceSize, sourceFontPath);

        std::u32string utf32;
        if (StringUtils::UTF8ToUTF32(_fontFreeType->getGlyphCollection(), utf32))
            this->prepareLetterDefinitions(utf32);

        _pageDatas.emplace_back(_currentPageData, _currentPageData + _currentPageDataSize);
    }

    void addNewPage() override
    {
        if (_currentPage != -1)
            _pageDatas.emplace_back(_currentPageData, _currentPageData + _currentPageDataSize);
        ax::FontAtlas::addNewPage();
    }

    FontAtlasGenParams* _params{nullptr};  // weak ref
    std::string _atlasName;                // match with runtime
    std::vector<yasio::byte_buffer> _pageDatas;
};
};  // namespace xasset

SDFGen* SDFGen::getInstance()
{
    return yasio::singleton<SDFGen>::instance();
}
void SDFGen::destroyInstance()
{
    yasio::singleton<SDFGen>::destroy();
}

void SDFGen::open(ax::Scene* scene)
{
    AXLOG("SDFGen::open() - Starting...");
    refreshFontList();

    AXLOG("Creating atlas viewer sprite...");
    _atlasViewer = Sprite::create();
    if (!_atlasViewer)
    {
        AXLOG("ERROR: Failed to create sprite!");
        return;
    }
    AXLOG("Sprite created successfully");

    AXLOG("Setting sprite texture...");
    _atlasViewer->setTexture(Director::getInstance()->getTextureCache()->getWhiteTexture("/black-texture", 0));
    AXLOG("Texture set successfully");

    _atlasViewer->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    _atlasViewer->retain();
    AXLOG("Atlas viewer initialized successfully");

    auto defaultFontFile = FileUtils::getInstance()->fullPathForFilename(R"(fonts/arial.ttf)");

    // 打印字体路径调试信息
    AXLOG("=========================================");
    AXLOG("SDFGen::open() - Font path debugging:");
    AXLOG("  Relative path: fonts/arial.ttf");
    AXLOG("  Full path: %s", defaultFontFile.c_str());
    AXLOG("  File exists: %s", FileUtils::getInstance()->isFileExist(defaultFontFile) ? "YES" : "NO");
    AXLOG("=========================================");

    _atlasParams             = new FontAtlasGenParams();
    // 默认值应该使用相对于 Content 的路径（res/fonts/）
    _atlasParams->sourceFont = "res/fonts/arial.ttf";
    _atlasParams->fontAsset  = "res/fonts/arial-SDF.xasset";

    AXLOG("Step 1: Creating FontAtlasGenParams - OK");

    AXLOG("Step 2: Adding font to ImGui...");
    ImGuiPresenter::getInstance()->addFont(defaultFontFile);
    AXLOG("Step 2: Font added - OK");

    /* For Simplified Chinese support, please use:
    ImGuiPresenter::getInstance()->addFont(R"(C:\Windows\Fonts\msyh.ttc)", ImGuiPresenter::DEFAULT_FONT_SIZE,
                                       ImGuiPresenter::GLYPH_RANGES::CHINESE_GENERAL);
    */

    AXLOG("Step 3: Enabling DPI scale...");
    ImGuiPresenter::getInstance()->enableDPIScale();  // enable dpi scale for 4K display support, depends at least one
                                                      // valid ttf/ttc font was added.
    AXLOG("Step 3: DPI scale enabled - OK");

    AXLOG("Step 4: Adding render loop...");
    ImGuiPresenter::getInstance()->addRenderLoop("#sdfg", AX_CALLBACK_0(SDFGen::onImGuiDraw, this), scene);
    AXLOG("Step 4: Render loop added - OK");

    AXLOG("SDFGen::open() completed successfully!");
}

void SDFGen::close()
{
    ImGuiPresenter::getInstance()->removeRenderLoop("#sdfg");

    delete _atlasParams;
    _atlasParams = nullptr;

    _atlasViewer->release();
}

void SDFGen::refreshFontList()
{
    _fontList.clear();

    auto fu = FileUtils::getInstance();

    // 打印路径调试信息
    auto& contentPath = fu->getDefaultResourceRootPath();
    auto fontsFullPath = fu->fullPathForFilename("fonts");
    AXLOG("=========================================");
    AXLOG("SDFGen::refreshFontList() - Path debugging:");
    AXLOG("  Content root path: %s", contentPath.c_str());
    AXLOG("  Fonts relative path: fonts");
    AXLOG("  Fonts full path: %s", fontsFullPath.c_str());

    std::vector<std::string> fileList;
    fu->listFilesRecursively("fonts", &fileList);

    AXLOG("  Found %zu files in fonts directory", fileList.size());
    AXLOG("=========================================");

    for (auto& filePath : fileList)
    {
        if (!cxx20::ic::ends_with(filePath, ".ttf") && !cxx20::ic::ends_with(filePath, ".ttc"))
            continue;

        AXLOG("  Font file (full): %s", filePath.c_str());

        if (cxx20::starts_with(filePath, contentPath))
        {
            auto relativePath = filePath.substr(contentPath.size());
            AXLOG("  -> Relative path: %s", relativePath.c_str());
            _fontList.emplace_back(relativePath);
        }
        else
        {
            AXLOG("  -> Keep as is: %s", filePath.c_str());
            _fontList.emplace_back(std::move(filePath));
        }
    }

    AXLOG("Total font files loaded: %zu", _fontList.size());
}

void SDFGen::onImGuiDraw()
{
    ImGui::StyleColorsDark();

    auto& style                     = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.94f);

    ImGui::SetNextWindowSize(ImVec2{550, 660}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowContentSize(ImVec2{0, 0});

    static std::string title = fmt::format("Axmol SDF Font Creator {}", AX_VERSION_STR_FULL);
    if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::BeginCombo("Source Font File", _atlasParams->sourceFont.c_str()))
        {
            for (int n = 0; n < _fontList.size(); n++)
            {
                bool is_selected = (_atlasParams->sourceFont == _fontList[n]);
                if (ImGui::Selectable(_fontList[n].c_str(), is_selected))
                {
                    _atlasParams->sourceFont = _fontList[n];

                    // 自动生成 fontAsset 文件名：将 .ttf/.ttc 替换为 -SDF.xasset
                    // 保持和源文件相同的路径（包括 res/fonts/ 等）
                    auto fontPath = _fontList[n];
                    auto lastDot = fontPath.find_last_of('.');
                    if (lastDot != std::string::npos)
                    {
                        auto baseName = fontPath.substr(0, lastDot);
                        _atlasParams->fontAsset = baseName + "-SDF.xasset";
                    }
                    else
                    {
                        _atlasParams->fontAsset = fontPath + "-SDF.xasset";
                    }

                    AXLOG("Source font selected: %s", _atlasParams->sourceFont.c_str());
                    AXLOG("Auto-generated asset: %s", _atlasParams->fontAsset.c_str());
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Refresh Font List"))
        {
            refreshFontList();
        }
        ImGui::DragInt("Sampling Point Size", &_atlasParams->faceSize, 1, 1, 144);
        ImGui::DragInt2("Atlas Resolution", _atlasParams->atlasDim, 32, 64, 4096);

        bool modified = ImGui::Checkbox("Use ASCII", &_atlasParams->useAscii);
        ImGui::SameLine();
        ImGui::Text("%s", "Character Set");
        if (!_atlasParams->useAscii)
        {
            // 如果切换到 Custom 模式且 glyphs 为空，填充常用中文字符集
            if (modified && _atlasParams->glyphs.empty())
            {
                // 常用中文字符：3500个常用汉字 + 标点符号
                _atlasParams->glyphs =
                    "的一是在不了有和人这中大为上个国我以要他时来用们生到作地于出就分对成会可主发年动同工也能下过子说产种面而方后多定行学法所民得经十三之进着等部度家电力里如水化高自二理起小物现实加量都两体制机当使点从业本去把性好应开它合还因由其些然前外天政四日那社义事平形相全表间样与关各重新线内数正心反你明看原又么利比或但质气第向道命此变条只没结解问意建月公无系军很情者最立代想已通并提直题党程展五果料象员革位入常文总次品式活设及管特件长求老头基资边流路级少图山统接知较将组见计别她手角期根论运农指几九区强放决西被干做必战先回则任取据处队南给色光门即保治北造百规热领七海口东导器压志世金增争济阶油思术极交受联什认六共权收证改清己美再采转更单风切打白教速花带安场身车例真务具万每目至达走积示议声报斗完类八离华名确才科张信马节话米整空元况今集温传土许步群广石记需段研界拉林律叫且究观越织装影算低持音众书布复容儿须际商非验连断深难近矿千周委素技备半办青省列习响约支般史感劳便团往酸历市克何除消构府称太准精值号率族维划选标写存候毛亲快效斯院查江型眼王按格养易置派层片始却专状育厂京识适属圆包火住调满县局照参红细引听该铁价严";

                AXLOG("切换到 Custom 模式，已填充常用中文字符集（%zu 字符）", _atlasParams->glyphs.size());
            }
            ImGui::InputTextMultiline("glyphs", &_atlasParams->glyphs);
        }
        else
        {
            if (modified)
                _atlasParams->glyphs.clear();
        }

        if (ImGui::Button("Generate Font Atlas"))
        {
            AX_SAFE_RELEASE_NULL(_fontAtlas);
            _fontAtlas = xasset::FontAtlas::newFontAtlas(_atlasParams);

            // TODO: display multi-pages with listview?
            auto textureAtlasPage0 = _fontAtlas->getTexture(0);
            _atlasViewer->setTexture(textureAtlasPage0);

            Rect rect = Rect::ZERO;
            rect.size = textureAtlasPage0->getContentSize();
            _atlasViewer->setTextureRect(rect);
        }
        ImGui::InputText("Font Asset", &_atlasParams->fontAsset);

        // 显示完整保存路径
        auto fu = FileUtils::getInstance();
        std::string fullPath;
        if (fu->isAbsolutePath(_atlasParams->fontAsset))
        {
            fullPath = _atlasParams->fontAsset;
        }
        else
        {
            fullPath = fu->getDefaultResourceRootPath();
            fullPath += _atlasParams->fontAsset;
        }
        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "Full path: %s", fullPath.c_str());

        if (ImGui::Button("Save"))
        {
            if (_fontAtlas)
            {
                AXLOG("=========================================");
                AXLOG("Saving font atlas to: %s", fullPath.c_str());
                AXLOG("=========================================");
                _fontAtlas->save();
            }
            else
                _atlasParams->error = "Please generate first!";

            _atlasParams->saved = true;
        }

        if (_atlasParams->saved)
        {
            ImGui::SameLine();
            if (!_atlasParams->error.empty())
                ImGui::TextColored(ImVec4{1.0, 0.0, 0.0, 1.0}, "%s", _atlasParams->error.c_str());
            else
                ImGui::TextColored(ImVec4{0.0, 1.0, 0.0, 1.0}, "Save succeed, cost %.3f (ms)", _atlasParams->cost);
        }

        auto viewerSize = _atlasViewer->getContentSize();
        if (viewerSize.fuzzyEquals(Vec2::ZERO, 1e-3))
        {
            viewerSize.width  = 512;
            viewerSize.height = 512;
        }

        ImGui::Separator();
        ImGui::Text("Atlas View:");
        ImGuiPresenter::getInstance()->image(_atlasViewer, ImVec2(viewerSize.width, viewerSize.height));
    }

    ImGui::End();
}

NS_AX_EXT_END
