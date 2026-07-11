#include "Scenes.h"

#include "AssetManager.h"
#include "Constants.h"
#include "Game.h"
#include "Text.h"
#include "Tween.h"
#include "render/Renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace wb {
namespace {

constexpr float kSettingsTvScale = 1.9f;
constexpr Vector2 kSettingsTvPosition{480.0f, 650.0f};
constexpr float kSettingsTabOffsetY = 41.0f;
constexpr float kSettingsTabHeight = 20.0f;
constexpr float kSettingsRowsOffsetY = 66.0f;
constexpr float kSettingsRowsBottomPadding = 8.0f;

Color alpha(Color color, float value) {
    color.a = static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * static_cast<float>(color.a));
    return color;
}

render::Color toRender(Color color) {
    return render::Color{color.r, color.g, color.b, color.a};
}

render::Vec2 toRender(Vector2 value) {
    return render::Vec2{value.x, value.y};
}

render::Rect toRender(Rectangle value) {
    return render::Rect{value.x, value.y, value.width, value.height};
}

float fadeInAt(float elapsed, float delay, float duration) {
    return clamp01((elapsed - delay) / duration);
}

float fadeOutAt(float elapsed, float delay, float duration) {
    return 1.0f - clamp01((elapsed - delay) / duration);
}

void drawRendererFullscreen(render::Renderer& renderer, render::TextureHandle texture, float alphaValue = 1.0f) {
    render::DrawTextureParams params{};
    params.texture = texture;
    params.destination = {0.0f, 0.0f, static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)};
    params.tint = toRender(WHITE);
    params.alpha = alphaValue;
    renderer.DrawTextureSource(params);
}

void drawRendererTextureAnchored(render::Renderer& renderer, render::TextureHandle texture, Vector2 position, Vector2 anchor, Vector2 scale, float rotationRadians, Color tint, float alphaValue = 1.0f) {
    const render::Vec2 textureSize = renderer.TextureSize(texture);
    if (textureSize.x <= 0.0f || textureSize.y <= 0.0f) {
        return;
    }
    const float width = textureSize.x / kAssetTextureResolution * std::abs(scale.x);
    const float height = textureSize.y / kAssetTextureResolution * std::abs(scale.y);
    render::DrawTextureParams params{};
    params.texture = texture;
    params.source = {0.0f, 0.0f, textureSize.x, textureSize.y};
    params.destination = {position.x, position.y, width, height};
    params.origin = {anchor.x * width, anchor.y * height};
    params.rotationRadians = rotationRadians;
    params.tint = toRender(tint);
    params.alpha = alphaValue;
    params.flipX = scale.x < 0.0f;
    params.flipY = scale.y < 0.0f;
    renderer.DrawTextureSource(params);
}

void drawRendererAtlasFrameIndex(render::Renderer& renderer, AssetManager& assets, const std::string& atlasPath, const std::string& prefix, int index, Vector2 pos, Vector2 anchor, Vector2 scale, float alphaValue = 1.0f) {
    Atlas& atlas = assets.atlas(atlasPath);
    std::vector<std::string> frames = atlas.framesByPrefix(prefix);
    if (frames.empty()) {
        throw std::runtime_error("Atlas has no frames for prefix '" + prefix + "' in " + atlasPath);
    }
    index = std::clamp(index, 0, static_cast<int>(frames.size()) - 1);
    const AtlasFrame& frame = atlas.frame(frames[static_cast<std::size_t>(index)]);
    const render::TextureHandle texture = assets.atlasTextureHandle(atlasPath);

    render::AtlasFrameParams params{};
    params.texture = texture;
    params.frame = frame.frame;
    params.logicalFrameSize = frame.logicalFrameSize;
    params.spriteSourceSize = frame.spriteSourceSize;
    params.sourceSize = frame.sourceSize;
    params.position = toRender(pos);
    params.anchor = toRender(anchor);
    params.scale = toRender(scale);
    params.tint = toRender(WHITE);
    params.alpha = alphaValue;
    params.rotated = frame.rotated;
    renderer.DrawAtlasFrame(params);
}

float fitRendererFontSize(render::Renderer& renderer, render::FontHandle font, const std::string& text, float size, float maxWidth, float minSize = 6.0f) {
    float fitted = size;
    while (fitted > minSize && renderer.MeasureText(font, text, fitted).x > maxWidth) {
        fitted -= 0.5f;
    }
    return fitted;
}

void drawRendererText(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 position, float size, Color color, float alphaValue = 1.0f) {
    renderer.DrawText({font, text, toRender(position), size, toRender(color), alphaValue});
}

void drawRendererTextCentered(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 position, float size, Color color, float alphaValue = 1.0f) {
    const render::Vec2 measured = renderer.MeasureText(font, text, size);
    drawRendererText(renderer, font, text, {position.x - measured.x / 2.0f, position.y - measured.y / 2.0f}, size, color, alphaValue);
}

void drawRendererTextCenteredFit(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 position, float size, float maxWidth, Color color, float alphaValue = 1.0f) {
    const float fitted = fitRendererFontSize(renderer, font, text, size, maxWidth);
    drawRendererTextCentered(renderer, font, text, position, fitted, color, alphaValue);
}

void drawRendererTextLinesCentered(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 position, float size, float lineGap, Color color, float alphaValue = 1.0f) {
    std::size_t start = 0;
    float y = position.y;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        const std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        drawRendererTextCentered(renderer, font, line, {position.x, y}, size, color, alphaValue);
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
        y += size + lineGap;
    }
}

void drawTextCenteredRotated(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 position, float size, float, Color color, float alphaValue = 1.0f) {
    const render::Vec2 measured = renderer.MeasureText(font, text, size);
    drawRendererText(renderer, font, text, {position.x - measured.x / 2.0f, position.y - measured.y / 2.0f}, size, color, alphaValue);
}

Vector2 rotateAround(Vector2 point, Vector2 center, float degrees) {
    const float radians = degrees * DEG2RAD;
    const float s = std::sin(radians);
    const float c = std::cos(radians);
    const float x = point.x - center.x;
    const float y = point.y - center.y;
    return {center.x + x * c - y * s, center.y + x * s + y * c};
}

void drawRotatedLine(Vector2 center, float halfWidth, float offsetY, float rotationDegrees, Color color, float thick) {
    const Vector2 a = rotateAround({center.x - halfWidth, center.y + offsetY}, center, rotationDegrees);
    const Vector2 b = rotateAround({center.x + halfWidth, center.y + offsetY}, center, rotationDegrees);
    DrawLineEx(a, b, thick, color);
}

void drawSettingsKnob(Vector2 center, float radius, bool active) {
    const Color tickColor = active ? Color{236, 236, 236, 230} : Color{132, 132, 132, 205};
    for (int i = 0; i < 8; ++i) {
        const float angle = static_cast<float>(i) * PI / 4.0f + 0.18f;
        const Vector2 dir{std::cos(angle), std::sin(angle)};
        DrawLineEx(
            {center.x + dir.x * (radius - 1.0f), center.y + dir.y * (radius - 1.0f)},
            {center.x + dir.x * (radius + 4.5f), center.y + dir.y * (radius + 4.5f)},
            active ? 2.0f : 1.5f,
            tickColor
        );
    }
    DrawCircleV(center, radius + 2.0f, Color{18, 18, 18, 220});
    DrawCircleV(center, radius, active ? Color{92, 92, 92, 240} : Color{54, 54, 54, 230});
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, active ? WHITE : Color{106, 106, 106, 255});
    DrawCircleV(center, radius * 0.33f, active ? Color{210, 210, 210, 255} : Color{94, 94, 94, 255});
}

void drawPorterCharacter(render::Renderer& renderer, render::FontHandle font, Vector2 feet, float scale, float alphaValue, const char* shirtText) {
    const Color stroke = alpha(Color{92, 92, 92, 255}, alphaValue);
    const Color dark = alpha(Color{42, 42, 42, 255}, alphaValue);
    const Color fill = alpha(Color{242, 242, 242, 255}, alphaValue);
    const Color shirt = alpha(Color{76, 76, 76, 255}, alphaValue);
    auto p = [&](float x, float y) {
        return Vector2{feet.x + x * scale, feet.y + y * scale};
    };

    DrawRectangleRounded({p(-13.0f, -43.0f).x, p(-13.0f, -43.0f).y, 26.0f * scale, 36.0f * scale}, 0.25f, 8, shirt);
    DrawRectangleRoundedLines({p(-13.0f, -43.0f).x, p(-13.0f, -43.0f).y, 26.0f * scale, 36.0f * scale}, 0.25f, 8, stroke);

    DrawLineEx(p(-6.0f, -7.0f), p(-9.0f, 0.0f), 4.0f * scale, stroke);
    DrawLineEx(p(7.0f, -7.0f), p(10.0f, 0.0f), 4.0f * scale, stroke);
    DrawLineEx(p(-8.0f, 0.0f), p(-1.0f, 0.0f), 4.0f * scale, stroke);
    DrawLineEx(p(3.0f, 0.0f), p(11.0f, 0.0f), 4.0f * scale, stroke);

    DrawCircleV(p(0.0f, -58.0f), 19.0f * scale, fill);
    DrawCircleLines(static_cast<int>(p(0.0f, -58.0f).x), static_cast<int>(p(0.0f, -58.0f).y), 19.0f * scale, stroke);
    DrawLineEx(p(-15.0f, -73.0f), p(-4.0f, -82.0f), 8.0f * scale, dark);
    DrawLineEx(p(-4.0f, -80.0f), p(13.0f, -74.0f), 8.0f * scale, dark);
    DrawLineEx(p(9.0f, -73.0f), p(18.0f, -63.0f), 7.0f * scale, dark);

    DrawCircleV(p(-7.0f, -58.0f), 2.2f * scale, stroke);
    DrawCircleV(p(8.0f, -58.0f), 2.2f * scale, stroke);
    DrawLineEx(p(-2.0f, -48.0f), p(8.0f, -48.0f), 2.0f * scale, stroke);

    DrawLineEx(p(-21.0f, -40.0f), p(-31.0f, -29.0f), 4.0f * scale, stroke);
    DrawLineEx(p(20.0f, -40.0f), p(30.0f, -31.0f), 4.0f * scale, stroke);
    DrawCircleV(p(-32.0f, -28.0f), 3.5f * scale, fill);
    DrawCircleV(p(31.0f, -30.0f), 3.5f * scale, fill);

    drawRendererText(renderer, font, shirtText, p(-8.0f, -35.0f), 8.0f * scale, Color{214, 214, 214, 255}, alphaValue);
}

}  // namespace

PreloaderScene::PreloaderScene(Game& game) : Scene(game) {}

void PreloaderScene::enter() {
    playHoverScale_ = 1.0f;
    settingsHoverScale_ = 1.0f;
    selected_ = 0;
    game_.assets().textureHandle("sprites/bg_preload.png");
    game_.assets().textureHandle("sprites/bg_preload_2.png");
    game_.assets().fontHandle(64);
    game_.assets().fontHandle(96);
    game_.assets().atlas("sprites/misc/cursor.json");
    game_.audio().preloadSound("sounds/squeak.mp3");
}

void PreloaderScene::exit() {
    game_.assets().releaseTexture("sprites/bg_preload.png");
    game_.assets().releaseTexture("sprites/bg_preload_2.png");
}

void PreloaderScene::update(float) {
    const Vector2 mouse = game_.logicalMouse();
    const Rectangle playButton{kGameWidth / 2.0f - 399.0f, 250.0f - 42.5f, 395.0f, 85.0f};
    const Rectangle settingsButton{716.0f, 394.0f, 154.0f, 58.0f};
    const bool playHovered = CheckCollisionPointRec(mouse, playButton);
    const bool settingsHovered = CheckCollisionPointRec(mouse, settingsButton);
    if (playHovered) selected_ = 0;
    if (settingsHovered) selected_ = 1;
    if (game_.keyPressed(InputKey::Up) || game_.keyPressed(InputKey::Down)) {
        selected_ = 1 - selected_;
        game_.audio().playSound("sounds/squeak.mp3", 0.4f);
    }

    playHoverScale_ = playHoverScale_ * 0.85f + ((playHovered || selected_ == 0) ? 1.04f : 1.0f) * 0.15f;
    settingsHoverScale_ = settingsHoverScale_ * 0.85f + ((settingsHovered || selected_ == 1) ? 1.04f : 1.0f) * 0.15f;

    const bool acceptPressed = game_.keyPressed(InputKey::Space) || game_.keyPressed(InputKey::Enter);
    const bool activatePlay = (playHovered && game_.logicalMousePressed()) || (acceptPressed && selected_ == 0);
    const bool activateSettings = (settingsHovered && game_.logicalMousePressed()) || (acceptPressed && selected_ == 1);
    if (activatePlay) {
        game_.audio().playSound("sounds/squeak.mp3");
        game_.requestScene(SceneId::Quote);
    } else if (activateSettings) {
        game_.audio().playSound("sounds/squeak.mp3");
        game_.requestScene(SceneId::Settings);
    }
}

void PreloaderScene::draw() {
    render::Renderer& renderer = game_.renderer();
    const render::FontHandle font64 = game_.assets().fontHandle(64);
    const render::FontHandle font96 = game_.assets().fontHandle(96);
    drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/bg_preload.png"));
    const Localization& loc = game_.localization();

    renderer.DrawRectangle({40.0f, 34.0f, 610.0f, 172.0f}, toRender(Color{172, 172, 172, 255}));
    drawRendererText(renderer, font96, loc.tr("menu.title.line1"), {78.0f, 46.0f}, 42.0f, WHITE);
    drawRendererText(renderer, font96, loc.tr("menu.title.line2"), {78.0f, 88.0f}, 67.0f, WHITE);
    drawRendererText(renderer, font64, loc.tr("menu.subtitle"), {84.0f, 165.0f}, 20.0f, WHITE);

    const Vector2 mouse = game_.logicalMouse();
    const Rectangle button{kGameWidth / 2.0f - 399.0f, 250.0f - 42.5f, 395.0f, 85.0f};
    const bool hovered = CheckCollisionPointRec(mouse, button) || selected_ == 0;

    const Rectangle playButton{
        278.0f - 197.5f * playHoverScale_,
        250.0f - 42.5f * playHoverScale_,
        395.0f * playHoverScale_,
        85.0f * playHoverScale_
    };
    renderer.DrawRectangle(toRender(playButton), toRender(hovered ? Color{245, 36, 40, 255} : Color{211, 35, 38, 255}));
    renderer.DrawRectangleOutline(toRender(playButton), 4.0f, toRender(BLACK));
    drawRendererTextCenteredFit(renderer, font96, loc.tr("menu.play"), {278.0f, 250.0f}, 45.0f * playHoverScale_, playButton.width - 34.0f, WHITE);

    drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/bg_preload_2.png"));

    const Rectangle settingsBase{716.0f, 394.0f, 154.0f, 58.0f};
    const Vector2 settingsCenter{settingsBase.x + settingsBase.width / 2.0f, settingsBase.y + settingsBase.height / 2.0f};
    const bool settingsHovered = CheckCollisionPointRec(mouse, settingsBase) || selected_ == 1;
    const float promptRotation = 4.0f;
    const float promptSize = 14.0f * settingsHoverScale_;
    const Color promptColor = settingsHovered ? Color{238, 238, 238, 255} : Color{138, 138, 138, 230};
    const Vector2 labelCenter{778.0f, 424.0f};
    const Vector2 knobCenter{842.0f, 424.0f};
    const float knobRadius = 9.0f * settingsHoverScale_;
    const std::string& settingsText = loc.tr("menu.settings");
    drawTextCenteredRotated(renderer, font64, settingsText, {labelCenter.x - 0.8f, labelCenter.y + 0.3f}, promptSize, promptRotation, Color{35, 35, 35, 255}, 0.55f);
    drawTextCenteredRotated(renderer, font64, settingsText, {labelCenter.x + 0.7f, labelCenter.y - 0.4f}, promptSize, promptRotation, promptColor, settingsHovered ? 0.55f : 0.35f);
    drawTextCenteredRotated(renderer, font64, settingsText, labelCenter, promptSize, promptRotation, promptColor, settingsHovered ? 0.95f : 0.68f);
    drawRotatedLine(labelCenter, 37.0f, 10.0f, promptRotation, settingsHovered ? Color{210, 210, 210, 220} : Color{92, 92, 92, 185}, settingsHovered ? 1.3f : 1.0f);
    drawSettingsKnob(knobCenter, knobRadius, settingsHovered);

    drawRendererTextCentered(renderer, font64, loc.tr(txt::playingTime), {278.0f, 378.0f}, 29.0f, WHITE);
    drawRendererTextLinesCentered(renderer, font64, loc.tr(txt::warning), {278.0f, 423.0f}, 22.0f, -4.0f, Color{102, 102, 102, 255});
}

SettingsScene::SettingsScene(Game& game, ReturnTarget returnTarget)
    : Scene(game), returnTarget_(returnTarget) {}

void SettingsScene::enter() {
    section_ = Section::Audio;
    selected_ = 0;
    dragging_ = -1;
    scrollOffset_ = 0.0f;
    game_.assets().textureHandle("sprites/bg.png");
    game_.assets().textureHandle("sprites/tv.png");
    game_.assets().fontHandle(64);
    game_.audio().preloadSound("sounds/squeak.mp3");
}

void SettingsScene::exit() {
    game_.settings().save();
}

void SettingsScene::update(float) {
    const int count = rowCount();
    if (count <= 0) {
        return;
    }
    selected_ = std::clamp(selected_, 0, std::max(0, count - 1));
    const Vector2 mouse = game_.logicalMouse();
    const Rectangle screen = tvScreenRect();
    const Rectangle rowsClip = rowsClipRect();
    clampScroll();

    const float wheel = game_.mouseWheelMove();
    if (std::abs(wheel) > 0.001f && CheckCollisionPointRec(mouse, screen)) {
        scrollOffset_ = std::clamp(scrollOffset_ - wheel * rowPitch() * 2.0f, 0.0f, maxScrollOffset());
    }

    if (game_.logicalMousePressed()) {
        constexpr int tabCount = static_cast<int>(Section::Count);
        const float tabWidth = (screen.width - 32.0f) / static_cast<float>(tabCount);
        for (int i = 0; i < tabCount; ++i) {
            const Rectangle tab{screen.x + 16.0f + static_cast<float>(i) * tabWidth, screen.y + kSettingsTabOffsetY, tabWidth - 3.0f, kSettingsTabHeight};
            if (CheckCollisionPointRec(mouse, tab)) {
                section_ = static_cast<Section>(i);
                selected_ = 0;
                dragging_ = -1;
                scrollOffset_ = 0.0f;
                game_.audio().playSound("sounds/squeak.mp3", 0.35f);
                return;
            }
        }
    }

    for (int i = 0; i < count; ++i) {
        if (CheckCollisionPointRec(mouse, rowsClip) && CheckCollisionPointRec(mouse, rowRect(i))) {
            selected_ = i;
        }
    }

    if (game_.keyPressed(InputKey::Down)) {
        selected_ = (selected_ + 1) % count;
        ensureSelectedVisible();
        game_.audio().playSound("sounds/squeak.mp3", 0.35f);
    }
    if (game_.keyPressed(InputKey::Up)) {
        selected_ = (selected_ + count - 1) % count;
        ensureSelectedVisible();
        game_.audio().playSound("sounds/squeak.mp3", 0.35f);
    }
    if (game_.keyPressed(InputKey::Left)) {
        adjustRow(selected_, -1);
    }
    if (game_.keyPressed(InputKey::Right)) {
        adjustRow(selected_, 1);
    }
    if (game_.keyPressed(InputKey::Enter) || game_.keyPressed(InputKey::Space)) {
        activateRow(selected_);
    }
    if (game_.keyPressed(InputKey::Escape)) {
        game_.settings().save();
        goBack();
        return;
    }

    if (game_.logicalMousePressed()) {
        for (int i = 0; i < count; ++i) {
            if (!CheckCollisionPointRec(mouse, rowsClip) || !CheckCollisionPointRec(mouse, rowRect(i))) {
                continue;
            }
            selected_ = i;
            if (isSlider(i) && CheckCollisionPointRec(mouse, sliderRect(i))) {
                dragging_ = i;
                setSliderFromMouse(i, mouse.x);
            } else {
                activateRow(i);
            }
            break;
        }
    }

    if (dragging_ >= 0) {
        if (game_.logicalMouseDown()) {
            setSliderFromMouse(dragging_, mouse.x);
        } else {
            dragging_ = -1;
            game_.settings().save();
        }
    }
}

void SettingsScene::draw() {
    render::Renderer& renderer = game_.renderer();
    const render::FontHandle font64 = game_.assets().fontHandle(64);
    drawRendererTextureAnchored(renderer, game_.assets().textureHandle("sprites/bg.png"), {-100.0f, -100.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f, WHITE);

    drawRendererTextureAnchored(renderer, game_.assets().textureHandle("sprites/tv.png"), kSettingsTvPosition, {0.5f, 1.0f}, {kSettingsTvScale, kSettingsTvScale}, 0.0f, WHITE, 1.0f);

    const Rectangle screen = tvScreenRect();
    renderer.PushScissor(toRender(screen));
    renderer.DrawRectangle(toRender(screen), toRender(Color{20, 20, 20, 218}));
    renderer.DrawRectangleOutline(toRender(Rectangle{screen.x + 3.0f, screen.y + 3.0f, screen.width - 6.0f, screen.height - 6.0f}), 2.0f, toRender(Color{88, 88, 88, 255}));
    const Localization& loc = game_.localization();
    drawRendererTextCenteredFit(renderer, font64, loc.tr("settings.title"), {screen.x + screen.width / 2.0f, screen.y + 23.0f}, 30.0f, screen.width - 24.0f, WHITE);

    const std::array<std::string, 4> tabs{{
        loc.tr("settings.tab.audio"),
        loc.tr("settings.tab.display"),
        loc.tr("settings.tab.visuals"),
        loc.tr("settings.tab.monitor")
    }};
    const float tabWidth = (screen.width - 32.0f) / static_cast<float>(tabs.size());
    for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
        const bool active = static_cast<int>(section_) == i;
        const Rectangle tab{screen.x + 16.0f + static_cast<float>(i) * tabWidth, screen.y + kSettingsTabOffsetY, tabWidth - 3.0f, kSettingsTabHeight};
        renderer.DrawRectangle(toRender(tab), toRender(active ? Color{82, 82, 82, 235} : Color{39, 39, 39, 210}));
        renderer.DrawRectangleOutline(toRender(tab), 1.0f, toRender(active ? Color{185, 185, 185, 255} : Color{78, 78, 78, 255}));
        drawRendererTextCenteredFit(renderer, font64, tabs[static_cast<std::size_t>(i)], {tab.x + tab.width / 2.0f, tab.y + tab.height / 2.0f}, 14.0f, tab.width - 8.0f, active ? WHITE : Color{190, 190, 190, 255});
    }

    renderer.PopScissor();
    const Rectangle rowsClip = rowsClipRect();
    renderer.PushScissor(toRender(rowsClip));
    const Vector2 mouse = game_.logicalMouse();
    const int count = rowCount();
    for (int i = 0; i < count; ++i) {
        const Rectangle row = rowRect(i);
        if (row.y + row.height < rowsClip.y || row.y > rowsClip.y + rowsClip.height) {
            continue;
        }
        const bool hovered = CheckCollisionPointRec(mouse, rowsClip) && CheckCollisionPointRec(mouse, row);
        const bool selected = selected_ == i;
        if (selected || hovered) {
            renderer.DrawRectangle(toRender(row), toRender(selected ? Color{84, 84, 84, 235} : Color{58, 58, 58, 210}));
            renderer.DrawRectangleOutline(toRender(row), 1.0f, toRender(Color{185, 185, 185, 255}));
        }

        const Color textColor = (selected || hovered) ? WHITE : Color{218, 218, 218, 255};
        const std::string label = rowLabel(i);
        const float baseRowTextSize = std::min(17.0f, std::max(11.0f, row.height - 2.0f));
        const float labelSize = fitRendererFontSize(renderer, font64, label, baseRowTextSize, row.width * 0.58f);
        const float labelY = row.y + (row.height - labelSize) / 2.0f - 0.5f;
        drawRendererText(renderer, font64, label, {row.x + 8.0f, labelY}, labelSize, textColor);

        if (isSlider(i)) {
            const Rectangle slider = sliderRect(i);
            const float value = sliderValue(i);
            renderer.DrawRectangle(toRender(slider), toRender(Color{112, 112, 112, 255}));
            renderer.DrawRectangle(toRender(Rectangle{slider.x, slider.y, slider.width * value, slider.height}), toRender(Color{232, 232, 232, 255}));
            renderer.DrawRectangle(toRender(Rectangle{slider.x + slider.width * value - 2.0f, slider.y - 4.0f, 4.0f, slider.height + 8.0f}), toRender(WHITE));
        }

        const std::string value = rowValue(i);
        if (!value.empty()) {
            const float valueSize = fitRendererFontSize(renderer, font64, value, baseRowTextSize, row.width * 0.42f);
            const render::Vec2 measured = renderer.MeasureText(font64, value, valueSize);
            const float valueY = row.y + (row.height - valueSize) / 2.0f - 0.5f;
            drawRendererText(renderer, font64, value, {row.x + row.width - measured.x - 8.0f, valueY}, valueSize, textColor);
        }
    }

    const float maxScroll = maxScrollOffset();
    if (maxScroll > 0.0f) {
        const float trackX = rowsClip.x + rowsClip.width - 5.0f;
        renderer.DrawRectangle(toRender(Rectangle{trackX, rowsClip.y + 2.0f, 2.0f, rowsClip.height - 4.0f}), toRender(Color{70, 70, 70, 180}));
        const float thumbHeight = std::max(16.0f, rowsClip.height * rowsClip.height / (rowsClip.height + maxScroll));
        const float thumbY = rowsClip.y + (rowsClip.height - thumbHeight) * (scrollOffset_ / maxScroll);
        renderer.DrawRectangle(toRender(Rectangle{trackX - 1.0f, thumbY, 4.0f, thumbHeight}), toRender(Color{190, 190, 190, 220}));
    }

    renderer.PopScissor();
}

Rectangle SettingsScene::tvScreenRect() const {
    constexpr float textureW = 300.0f;
    constexpr float textureH = 360.0f;
    constexpr Rectangle screenLocal{29.0f, 66.0f, 240.0f, 137.0f};
    const float tvLeft = kSettingsTvPosition.x - textureW * kSettingsTvScale / 2.0f;
    const float tvTop = kSettingsTvPosition.y - textureH * kSettingsTvScale;
    return {
        tvLeft + screenLocal.x * kSettingsTvScale + 10.0f,
        tvTop + screenLocal.y * kSettingsTvScale + 9.0f,
        screenLocal.width * kSettingsTvScale - 20.0f,
        screenLocal.height * kSettingsTvScale - 18.0f
    };
}

Rectangle SettingsScene::rowsClipRect() const {
    const Rectangle screen = tvScreenRect();
    return {
        screen.x + 10.0f,
        screen.y + kSettingsRowsOffsetY,
        screen.width - 20.0f,
        screen.height - kSettingsRowsOffsetY - kSettingsRowsBottomPadding
    };
}

float SettingsScene::rowPitch() const {
    return 22.0f;
}

Rectangle SettingsScene::rowRect(int index) const {
    const Rectangle screen = tvScreenRect();
    constexpr float rowStartY = kSettingsRowsOffsetY + 2.0f;
    const float pitch = rowPitch();
    const float rowHeight = 20.0f;
    return {
        screen.x + 12.0f,
        screen.y + rowStartY + static_cast<float>(index) * pitch - scrollOffset_,
        screen.width - 24.0f,
        rowHeight
    };
}

Rectangle SettingsScene::sliderRect(int index) const {
    const Rectangle row = rowRect(index);
    return {row.x + row.width - 178.0f, row.y + row.height / 2.0f - 2.5f, 110.0f, 5.0f};
}

float SettingsScene::maxScrollOffset() const {
    const Rectangle rowsClip = rowsClipRect();
    const float contentHeight = static_cast<float>(rowCount()) * rowPitch();
    return std::max(0.0f, contentHeight - rowsClip.height + 2.0f);
}

void SettingsScene::clampScroll() {
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset());
}

void SettingsScene::ensureSelectedVisible() {
    const Rectangle rowsClip = rowsClipRect();
    const float rowTop = static_cast<float>(selected_) * rowPitch();
    const float rowBottom = rowTop + rowPitch();
    const float visibleHeight = rowsClip.height;
    if (rowTop < scrollOffset_) {
        scrollOffset_ = rowTop;
    } else if (rowBottom > scrollOffset_ + visibleHeight) {
        scrollOffset_ = rowBottom - visibleHeight;
    }
    clampScroll();
}

int SettingsScene::rowCount() const {
    switch (section_) {
        case Section::Audio:
            return 6;
        case Section::Display:
            return 11;
        case Section::Visuals:
            return 15;
        case Section::Monitor:
            return 14;
        case Section::Count:
            break;
    }
    return 0;
}

SettingsScene::Row SettingsScene::rowAt(int index) const {
    index = std::clamp(index, 0, std::max(0, rowCount() - 1));
    switch (section_) {
        case Section::Audio: {
            constexpr Row rows[] = {Row::Section, Row::MasterVolume, Row::MusicVolume, Row::SfxVolume, Row::Reset, Row::Back};
            return rows[index];
        }
        case Section::Display: {
            constexpr Row rows[] = {
                Row::Section, Row::Language, Row::HdrOutput, Row::HdrPaperWhite,
                Row::HdrPeakBrightness, Row::HdrHighlights, Row::MsaaSamples,
                Row::Fxaa, Row::PhotoUpscaler, Row::Reset, Row::Back
            };
            return rows[index];
        }
        case Section::Visuals: {
            constexpr Row rows[] = {
                Row::Section, Row::CrtFilter, Row::ScreenNoise, Row::Sharpness, Row::Gamma,
                Row::Brightness, Row::Contrast, Row::BlackLevel, Row::WhiteLevel,
                Row::ScreenBorder, Row::Scanlines, Row::CrtCurve, Row::ReduceFlashing,
                Row::Reset, Row::Back
            };
            return rows[index];
        }
        case Section::Monitor: {
            constexpr Row rows[] = {
                Row::Section, Row::DisplayMode, Row::Monitor, Row::WindowScale, Row::WindowSize,
                Row::IntegerScaling, Row::AspectFit, Row::OutputSmoothing, Row::SafeArea,
                Row::FrameLimit, Row::Vsync, Row::FpsCounter, Row::Reset, Row::Back
            };
            return rows[index];
        }
        case Section::Count:
            break;
    }
    return Row::Back;
}

void SettingsScene::stepSection(int direction) {
    constexpr int count = static_cast<int>(Section::Count);
    const int next = (static_cast<int>(section_) + direction + count) % count;
    section_ = static_cast<Section>(next);
    selected_ = std::clamp(selected_, 0, std::max(0, rowCount() - 1));
    dragging_ = -1;
    scrollOffset_ = 0.0f;
    game_.audio().playSound("sounds/squeak.mp3", 0.35f);
}

void SettingsScene::activateRow(int index) {
    GameSettings& settings = game_.settings();
    switch (rowAt(index)) {
        case Row::Section:
            stepSection(1);
            break;
        case Row::DisplayMode:
            settings.stepDisplayMode(1);
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Monitor:
            settings.stepMonitor(1, game_.monitorCount());
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::WindowScale:
            settings.stepWindowScale(1);
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::WindowSize:
            settings.stepWindowSize(1);
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::IntegerScaling:
            settings.integerScaling = !settings.integerScaling;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::AspectFit:
            settings.aspectFit = 1 - settings.aspectFit;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Language:
            game_.stepLanguage(1);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Vsync:
            settings.vsync = !settings.vsync;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::OutputSmoothing:
            settings.outputSmoothing = 1 - settings.outputSmoothing;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::MsaaSamples:
            settings.stepMsaaSamples(1);
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Fxaa:
            settings.fxaa = !settings.fxaa;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::PhotoUpscaler:
            settings.photoUpscaler = game_.renderer().SupportsFsr1() ? 1 - settings.photoUpscaler : 0;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::HdrOutput:
            if (game_.renderer().Stats().hdrAvailable) {
                settings.stepHdrMode(1);
                saveAndApply(false);
                game_.audio().playSound("sounds/squeak.mp3");
            }
            break;
        case Row::CrtFilter:
            settings.crtFilter = (settings.crtFilter + 1) % 3;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::ScreenNoise:
            settings.screenNoise = (settings.screenNoise + 1) % 4;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::FrameLimit:
            settings.stepFrameLimit(1);
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::FpsCounter:
            settings.fpsCounter = !settings.fpsCounter;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::ScreenBorder:
            settings.screenBorder = (settings.screenBorder + 1) % 3;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Scanlines:
            settings.scanlines = (settings.scanlines + 1) % 4;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::CrtCurve:
            settings.crtCurve = (settings.crtCurve + 1) % 3;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::ReduceFlashing:
            settings.reduceFlashing = !settings.reduceFlashing;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Reset:
            settings.resetDefaults();
            game_.localization().setLanguage(settings.languageId);
            selected_ = 0;
            scrollOffset_ = 0.0f;
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Back:
            settings.save();
            game_.audio().playSound("sounds/squeak.mp3");
            goBack();
            break;
        case Row::MasterVolume:
        case Row::MusicVolume:
        case Row::SfxVolume:
        case Row::Sharpness:
        case Row::Gamma:
        case Row::Brightness:
        case Row::Contrast:
        case Row::BlackLevel:
        case Row::WhiteLevel:
        case Row::HdrPaperWhite:
        case Row::HdrPeakBrightness:
        case Row::HdrHighlights:
        case Row::SafeArea:
            break;
    }
}

void SettingsScene::goBack() {
    if (returnTarget_ == ReturnTarget::PauseMenu) {
        game_.requestPauseSettingsClose();
    } else {
        game_.requestScene(SceneId::Preloader);
    }
}

void SettingsScene::adjustRow(int index, int direction) {
    GameSettings& settings = game_.settings();
    const float delta = static_cast<float>(direction) * 0.05f;
    switch (rowAt(index)) {
        case Row::Section:
            stepSection(direction);
            break;
        case Row::MasterVolume:
            settings.masterVolume = std::clamp(settings.masterVolume + delta, 0.0f, 1.0f);
            saveAndApply(false);
            break;
        case Row::MusicVolume:
            settings.musicVolume = std::clamp(settings.musicVolume + delta, 0.0f, 1.0f);
            saveAndApply(false);
            break;
        case Row::SfxVolume:
            settings.sfxVolume = std::clamp(settings.sfxVolume + delta, 0.0f, 1.0f);
            saveAndApply(false);
            break;
        case Row::DisplayMode:
            settings.stepDisplayMode(direction);
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Monitor:
            settings.stepMonitor(direction, game_.monitorCount());
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::WindowScale:
            settings.stepWindowScale(direction);
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::WindowSize:
            settings.stepWindowSize(direction);
            saveAndApply(true);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::IntegerScaling:
        case Row::AspectFit:
        case Row::Vsync:
        case Row::OutputSmoothing:
        case Row::Fxaa:
        case Row::PhotoUpscaler:
        case Row::FpsCounter:
        case Row::ReduceFlashing:
        case Row::HdrOutput:
            activateRow(index);
            break;
        case Row::Language:
            game_.stepLanguage(direction);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::MsaaSamples:
            settings.stepMsaaSamples(direction);
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::CrtFilter:
            settings.crtFilter = (settings.crtFilter + direction + 3) % 3;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::ScreenNoise:
            settings.screenNoise = (settings.screenNoise + direction + 4) % 4;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Sharpness:
            settings.sharpness = std::clamp(settings.sharpness + delta, 0.0f, 2.0f);
            saveAndApply(false);
            break;
        case Row::Gamma:
            settings.gamma = std::clamp(settings.gamma + delta, 0.8f, 1.2f);
            saveAndApply(false);
            break;
        case Row::Brightness:
            settings.brightness = std::clamp(settings.brightness + delta, 0.5f, 1.5f);
            saveAndApply(false);
            break;
        case Row::Contrast:
            settings.contrast = std::clamp(settings.contrast + delta, 0.5f, 1.5f);
            saveAndApply(false);
            break;
        case Row::BlackLevel:
            settings.blackLevel = std::clamp(settings.blackLevel + static_cast<float>(direction) * 0.01f, 0.0f, 0.2f);
            saveAndApply(false);
            break;
        case Row::WhiteLevel:
            settings.whiteLevel = std::clamp(settings.whiteLevel + delta, 0.8f, 1.2f);
            saveAndApply(false);
            break;
        case Row::HdrPaperWhite:
            if (game_.renderer().Stats().hdrAvailable && settings.hdrMode != 1) {
                settings.hdrPaperWhiteNits = std::clamp(
                    settings.hdrPaperWhiteNits + static_cast<float>(direction) * 10.0f,
                    80.0f,
                    500.0f
                );
                settings.hdrPeakNits = std::max(settings.hdrPeakNits, settings.hdrPaperWhiteNits);
                saveAndApply(false);
            }
            break;
        case Row::HdrPeakBrightness:
            if (game_.renderer().Stats().hdrAvailable) {
                const float minimumPeak = game_.settings().hdrMode == 1
                    ? game_.renderer().Stats().hdrPaperWhiteNits
                    : settings.hdrPaperWhiteNits;
                settings.hdrPeakNits = std::clamp(
                    settings.hdrPeakNits + static_cast<float>(direction) * 100.0f,
                    std::max(400.0f, minimumPeak),
                    4000.0f
                );
                saveAndApply(false);
            }
            break;
        case Row::HdrHighlights:
            if (game_.renderer().Stats().hdrAvailable) {
                settings.hdrHighlightStrength = std::clamp(settings.hdrHighlightStrength + delta, 0.0f, 1.0f);
                saveAndApply(false);
            }
            break;
        case Row::SafeArea:
            settings.safeArea = std::clamp(settings.safeArea + static_cast<float>(direction) * 0.01f, 0.9f, 1.0f);
            saveAndApply(false);
            break;
        case Row::FrameLimit:
            settings.stepFrameLimit(direction);
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::ScreenBorder:
            settings.screenBorder = (settings.screenBorder + direction + 3) % 3;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Scanlines:
            settings.scanlines = (settings.scanlines + direction + 4) % 4;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::CrtCurve:
            settings.crtCurve = (settings.crtCurve + direction + 3) % 3;
            saveAndApply(false);
            game_.audio().playSound("sounds/squeak.mp3");
            break;
        case Row::Reset:
        case Row::Back:
            break;
    }
}

void SettingsScene::setSliderFromMouse(int index, float x) {
    const Rectangle slider = sliderRect(index);
    const float value = std::clamp((x - slider.x) / slider.width, 0.0f, 1.0f);
    setSliderValue(index, value);
    saveAndApply(false);
}

void SettingsScene::saveAndApply(bool resizeWindow) {
    game_.applySettings(resizeWindow);
    game_.settings().save();
}

std::string SettingsScene::rowLabel(int index) const {
    const Localization& loc = game_.localization();
    switch (rowAt(index)) {
        case Row::Section: return loc.tr("settings.category");
        case Row::MasterVolume: return loc.tr("settings.master_volume");
        case Row::MusicVolume: return loc.tr("settings.music_volume");
        case Row::SfxVolume: return loc.tr("settings.sfx_volume");
        case Row::Language: return loc.tr("settings.language");
        case Row::DisplayMode: return loc.tr("settings.display_mode");
        case Row::Monitor: return loc.tr("settings.monitor");
        case Row::WindowScale: return loc.tr("settings.window_scale");
        case Row::WindowSize: return loc.tr("settings.window_size");
        case Row::IntegerScaling: return loc.tr("settings.integer_scaling");
        case Row::AspectFit: return loc.tr("settings.aspect_fit");
        case Row::Vsync: return loc.tr("settings.vsync");
        case Row::OutputSmoothing: return loc.tr("settings.output_filter");
        case Row::MsaaSamples: return loc.tr("settings.msaa");
        case Row::Fxaa: return loc.tr("settings.fxaa");
        case Row::PhotoUpscaler: return loc.tr("settings.photo_upscaler");
        case Row::CrtFilter: return loc.tr("settings.crt_filter");
        case Row::ScreenNoise: return loc.tr("settings.screen_noise");
        case Row::Sharpness: return loc.tr("settings.sharpness");
        case Row::Gamma: return loc.tr("settings.gamma");
        case Row::Brightness: return loc.tr("settings.brightness");
        case Row::Contrast: return loc.tr("settings.contrast");
        case Row::BlackLevel: return loc.tr("settings.black_level");
        case Row::WhiteLevel: return loc.tr("settings.white_level");
        case Row::SafeArea: return loc.tr("settings.safe_area");
        case Row::FrameLimit: return loc.tr("settings.frame_limit");
        case Row::FpsCounter: return loc.tr("settings.fps_counter");
        case Row::ScreenBorder: return loc.tr("settings.screen_border");
        case Row::Scanlines: return loc.tr("settings.scanlines");
        case Row::CrtCurve: return loc.tr("settings.crt_curve");
        case Row::ReduceFlashing: return loc.tr("settings.reduce_flashing");
        case Row::HdrOutput: return loc.tr("settings.hdr_output");
        case Row::HdrPaperWhite: return loc.tr("settings.hdr_paper_white");
        case Row::HdrPeakBrightness: return loc.tr("settings.hdr_peak_brightness");
        case Row::HdrHighlights: return loc.tr("settings.hdr_highlights");
        case Row::Reset: return loc.tr("settings.reset");
        case Row::Back: return loc.tr("settings.back");
    }
    return {};
}

std::string SettingsScene::rowValue(int index) const {
    char buffer[48]{};
    const Localization& loc = game_.localization();
    switch (rowAt(index)) {
        case Row::Section:
            switch (section_) {
                case Section::Audio: return loc.tr("settings.section.audio");
                case Section::Display: return loc.tr("settings.section.display");
                case Section::Visuals: return loc.tr("settings.section.visuals");
                case Section::Monitor: return loc.tr("settings.section.monitor");
                case Section::Count: break;
            }
            return "";
        case Row::MasterVolume:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().masterVolume * 100.0f)));
            return buffer;
        case Row::MusicVolume:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().musicVolume * 100.0f)));
            return buffer;
        case Row::SfxVolume:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().sfxVolume * 100.0f)));
            return buffer;
        case Row::Language:
            return game_.localization().currentLanguageDisplayLabel();
        case Row::DisplayMode:
            switch (game_.settings().displayMode) {
                case 1: return loc.tr("settings.value.borderless");
                case 2: return loc.tr("settings.value.fullscreen");
                default: return loc.tr("settings.value.windowed");
            }
        case Row::Monitor:
            return game_.monitorLabel(game_.settings().monitorIndex);
        case Row::WindowScale:
            switch (game_.settings().windowScale) {
                case 0: return "1x";
                case 1: return "2x";
                case 2: return "3x";
                case 3: return "4x";
                case 5: return loc.tr("settings.value.custom");
                default: return loc.tr("settings.value.fit");
            }
        case Row::WindowSize:
            return game_.settings().currentWindowSize().label;
        case Row::IntegerScaling:
            return game_.settings().integerScaling ? loc.tr("settings.value.on") : loc.tr("settings.value.off");
        case Row::AspectFit:
            return game_.settings().aspectFit == 1 ? loc.tr("settings.value.overscan") : loc.tr("settings.value.letterbox");
        case Row::Vsync:
            return game_.settings().vsync ? loc.tr("settings.value.on") : loc.tr("settings.value.off");
        case Row::OutputSmoothing:
            return game_.settings().outputSmoothing == 1 ? loc.tr("settings.value.linear") : loc.tr("settings.value.nearest");
        case Row::MsaaSamples:
            switch (game_.settings().msaaSamples) {
                case 2: return "2x";
                case 4: return "4x";
                case 8: return "8x";
                case 16: return "16x";
                default: return loc.tr("settings.value.off");
            }
        case Row::Fxaa:
            return game_.settings().fxaa ? loc.tr("settings.value.on") : loc.tr("settings.value.off");
        case Row::PhotoUpscaler:
            if (!game_.renderer().SupportsFsr1()) return loc.tr("settings.value.unavailable");
            return game_.settings().photoUpscaler == 1 ? "AMD FSR 1" : loc.tr("settings.value.off");
        case Row::HdrOutput:
            if (!game_.renderer().Stats().hdrAvailable) return loc.tr("settings.value.unavailable");
            if (game_.settings().hdrMode == 2) return loc.tr("settings.value.on");
            if (game_.settings().hdrMode == 1) return loc.tr("settings.value.auto");
            return loc.tr("settings.value.off");
        case Row::HdrPaperWhite: {
            const render::RendererStats stats = game_.renderer().Stats();
            if (!stats.hdrAvailable) return loc.tr("settings.value.unavailable");
            if (game_.settings().hdrMode == 1) {
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "%d nits (%s)",
                    static_cast<int>(std::lround(stats.hdrPaperWhiteNits)),
                    loc.tr("settings.value.system").c_str()
                );
            } else {
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "%d nits",
                    static_cast<int>(std::lround(game_.settings().hdrPaperWhiteNits))
                );
            }
            return buffer;
        }
        case Row::HdrPeakBrightness:
            if (!game_.renderer().Stats().hdrAvailable) return loc.tr("settings.value.unavailable");
            std::snprintf(
                buffer,
                sizeof(buffer),
                "%d nits",
                static_cast<int>(std::lround(game_.renderer().Stats().hdrPeakNits))
            );
            return buffer;
        case Row::HdrHighlights:
            if (!game_.renderer().Stats().hdrAvailable) return loc.tr("settings.value.unavailable");
            std::snprintf(
                buffer,
                sizeof(buffer),
                "%d%%",
                static_cast<int>(std::lround(game_.settings().hdrHighlightStrength * 100.0f))
            );
            return buffer;
        case Row::CrtFilter:
            switch (game_.settings().crtFilter) {
                case 1: return loc.tr("settings.value.subtle");
                case 2: return loc.tr("settings.value.strong");
                default: return loc.tr("settings.value.off");
            }
        case Row::ScreenNoise:
            switch (game_.settings().screenNoise) {
                case 1: return loc.tr("settings.value.low");
                case 2: return loc.tr("settings.value.medium");
                case 3: return loc.tr("settings.value.high");
                default: return loc.tr("settings.value.off");
            }
        case Row::Sharpness:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().sharpness * 100.0f)));
            return buffer;
        case Row::Gamma:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().gamma * 100.0f)));
            return buffer;
        case Row::Brightness:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().brightness * 100.0f)));
            return buffer;
        case Row::Contrast:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().contrast * 100.0f)));
            return buffer;
        case Row::BlackLevel:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().blackLevel * 100.0f)));
            return buffer;
        case Row::WhiteLevel:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().whiteLevel * 100.0f)));
            return buffer;
        case Row::SafeArea:
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(game_.settings().safeArea * 100.0f)));
            return buffer;
        case Row::FrameLimit:
            switch (game_.settings().frameLimit) {
                case 0: return "30 FPS";
                case 2: return "120 FPS";
                case 3: return loc.tr("settings.value.unlimited");
                default: return "60 FPS";
            }
        case Row::FpsCounter:
            return game_.settings().fpsCounter ? loc.tr("settings.value.on") : loc.tr("settings.value.off");
        case Row::ScreenBorder:
            switch (game_.settings().screenBorder) {
                case 1: return loc.tr("settings.value.subtle");
                case 2: return loc.tr("settings.value.strong");
                default: return loc.tr("settings.value.off");
            }
        case Row::Scanlines:
            switch (game_.settings().scanlines) {
                case 1: return loc.tr("settings.value.light");
                case 2: return loc.tr("settings.value.medium");
                case 3: return loc.tr("settings.value.heavy");
                default: return loc.tr("settings.value.off");
            }
        case Row::CrtCurve:
            switch (game_.settings().crtCurve) {
                case 1: return loc.tr("settings.value.light");
                case 2: return loc.tr("settings.value.medium");
                default: return loc.tr("settings.value.off");
            }
        case Row::ReduceFlashing:
            return game_.settings().reduceFlashing ? loc.tr("settings.value.on") : loc.tr("settings.value.off");
        case Row::Reset:
            return "";
        case Row::Back:
            return loc.tr("settings.value.esc");
    }
    return "";
}

bool SettingsScene::isSlider(int index) const {
    const Row row = rowAt(index);
    const bool hdrAvailable = game_.renderer().Stats().hdrAvailable;
    return row == Row::MasterVolume ||
           row == Row::MusicVolume ||
           row == Row::SfxVolume ||
           row == Row::Sharpness ||
           row == Row::Gamma ||
           row == Row::Brightness ||
           row == Row::Contrast ||
           row == Row::BlackLevel ||
           row == Row::WhiteLevel ||
           (row == Row::HdrPaperWhite && hdrAvailable && game_.settings().hdrMode != 1) ||
           (row == Row::HdrPeakBrightness && hdrAvailable) ||
           (row == Row::HdrHighlights && hdrAvailable) ||
           row == Row::SafeArea;
}

float SettingsScene::sliderValue(int index) const {
    const GameSettings& settings = game_.settings();
    switch (rowAt(index)) {
        case Row::MasterVolume:
            return settings.masterVolume;
        case Row::MusicVolume:
            return settings.musicVolume;
        case Row::SfxVolume:
            return settings.sfxVolume;
        case Row::Sharpness:
            return std::clamp(settings.sharpness / 2.0f, 0.0f, 1.0f);
        case Row::Gamma:
            return std::clamp((settings.gamma - 0.8f) / 0.4f, 0.0f, 1.0f);
        case Row::Brightness:
            return std::clamp(settings.brightness - 0.5f, 0.0f, 1.0f);
        case Row::Contrast:
            return std::clamp(settings.contrast - 0.5f, 0.0f, 1.0f);
        case Row::BlackLevel:
            return std::clamp(settings.blackLevel / 0.2f, 0.0f, 1.0f);
        case Row::WhiteLevel:
            return std::clamp((settings.whiteLevel - 0.8f) / 0.4f, 0.0f, 1.0f);
        case Row::HdrPaperWhite:
            return std::clamp((settings.hdrPaperWhiteNits - 80.0f) / 420.0f, 0.0f, 1.0f);
        case Row::HdrPeakBrightness:
            return std::clamp((game_.renderer().Stats().hdrPeakNits - 400.0f) / 3600.0f, 0.0f, 1.0f);
        case Row::HdrHighlights:
            return std::clamp(settings.hdrHighlightStrength, 0.0f, 1.0f);
        case Row::SafeArea:
            return std::clamp((settings.safeArea - 0.9f) / 0.1f, 0.0f, 1.0f);
        case Row::Section:
        case Row::Language:
        case Row::DisplayMode:
        case Row::Monitor:
        case Row::WindowScale:
        case Row::WindowSize:
        case Row::IntegerScaling:
        case Row::AspectFit:
        case Row::Vsync:
        case Row::OutputSmoothing:
        case Row::MsaaSamples:
        case Row::Fxaa:
        case Row::PhotoUpscaler:
        case Row::HdrOutput:
        case Row::CrtFilter:
        case Row::ScreenNoise:
        case Row::FrameLimit:
        case Row::FpsCounter:
        case Row::ScreenBorder:
        case Row::Scanlines:
        case Row::CrtCurve:
        case Row::ReduceFlashing:
        case Row::Reset:
        case Row::Back:
            break;
    }
    return 0.0f;
}

void SettingsScene::setSliderValue(int index, float value) {
    GameSettings& settings = game_.settings();
    switch (rowAt(index)) {
        case Row::MasterVolume:
            settings.masterVolume = value;
            break;
        case Row::MusicVolume:
            settings.musicVolume = value;
            break;
        case Row::SfxVolume:
            settings.sfxVolume = value;
            break;
        case Row::Sharpness:
            settings.sharpness = value * 2.0f;
            break;
        case Row::Gamma:
            settings.gamma = 0.8f + value * 0.4f;
            break;
        case Row::Brightness:
            settings.brightness = 0.5f + value;
            break;
        case Row::Contrast:
            settings.contrast = 0.5f + value;
            break;
        case Row::BlackLevel:
            settings.blackLevel = value * 0.2f;
            break;
        case Row::WhiteLevel:
            settings.whiteLevel = 0.8f + value * 0.4f;
            break;
        case Row::HdrPaperWhite:
            settings.hdrPaperWhiteNits = std::round((80.0f + value * 420.0f) / 10.0f) * 10.0f;
            settings.hdrPeakNits = std::max(settings.hdrPeakNits, settings.hdrPaperWhiteNits);
            break;
        case Row::HdrPeakBrightness:
            settings.hdrPeakNits = std::round((400.0f + value * 3600.0f) / 100.0f) * 100.0f;
            settings.hdrPeakNits = std::max(
                settings.hdrPeakNits,
                settings.hdrMode == 1
                    ? game_.renderer().Stats().hdrPaperWhiteNits
                    : settings.hdrPaperWhiteNits
            );
            break;
        case Row::HdrHighlights:
            settings.hdrHighlightStrength = value;
            break;
        case Row::SafeArea:
            settings.safeArea = 0.9f + value * 0.1f;
            break;
        case Row::Section:
        case Row::Language:
        case Row::DisplayMode:
        case Row::Monitor:
        case Row::WindowScale:
        case Row::WindowSize:
        case Row::IntegerScaling:
        case Row::AspectFit:
        case Row::Vsync:
        case Row::OutputSmoothing:
        case Row::MsaaSamples:
        case Row::Fxaa:
        case Row::PhotoUpscaler:
        case Row::HdrOutput:
        case Row::CrtFilter:
        case Row::ScreenNoise:
        case Row::FrameLimit:
        case Row::FpsCounter:
        case Row::ScreenBorder:
        case Row::Scanlines:
        case Row::CrtCurve:
        case Row::ReduceFlashing:
        case Row::Reset:
        case Row::Back:
            break;
    }
}

QuoteScene::QuoteScene(Game& game) : Scene(game) {}

void QuoteScene::enter() {
    elapsed_ = 0.0f;
    ambienceStarted_ = false;
    game_.assets().textureHandle("sprites/quote/quote0001.png");
    game_.assets().textureHandle("sprites/quote/quote0002.png");
    game_.assets().textureHandle("sprites/quote/quote0003.png");
    game_.assets().textureHandle("sprites/quote/quote0004.png");
    game_.assets().fontHandle(96);
    game_.audio().preloadMusic("sounds/bg_park.mp3");
}

void QuoteScene::exit() {
    for (int i = 1; i <= 4; ++i) {
        game_.assets().releaseTexture("sprites/quote/quote000" + std::to_string(i) + ".png");
    }
}

void QuoteScene::update(float dt) {
    elapsed_ += dt;
    if (!ambienceStarted_ && elapsed_ >= 9.5f) {
        ambienceStarted_ = true;
        game_.audio().playMusic("sounds/bg_park.mp3", 0.0f, true);
        game_.audio().fadeMusic("sounds/bg_park.mp3", 0.0f, 1.0f, 2.0f);
    }
    if (elapsed_ >= 12.0f || game_.keyPressed(InputKey::Space) || game_.keyPressed(InputKey::Enter)) {
        game_.requestScene(SceneId::Gameplay);
    }
}

void QuoteScene::draw() {
    render::Renderer& renderer = game_.renderer();
    const render::FontHandle font96 = game_.assets().fontHandle(96);
    drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/quote/quote0001.png"));
    const float q2 = fadeInAt(elapsed_, 1.5f, 1.0f) * fadeOutAt(elapsed_, 10.5f, 1.0f);
    const float q3 = fadeInAt(elapsed_, 6.5f, 1.0f) * fadeOutAt(elapsed_, 10.5f, 1.0f);
    const float q4 = fadeInAt(elapsed_, 8.5f, 1.0f) * fadeOutAt(elapsed_, 10.5f, 1.0f);

    drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/quote/quote0002.png"), q2);
    const Localization& loc = game_.localization();
    drawRendererTextCenteredFit(renderer, font96, loc.tr(txt::quoteTitle), {kGameWidth / 2.0f + 6.0f, kGameHeight / 2.0f - 40.0f}, 65.0f, 860.0f, WHITE, q2);
    drawRendererTextCenteredFit(renderer, font96, loc.tr(txt::quoteBody), {kGameWidth / 2.0f + 6.0f, kGameHeight / 2.0f + 20.0f}, 39.0f, 880.0f, WHITE, q2);

    drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/quote/quote0003.png"), q3);
    drawRendererTextCenteredFit(renderer, font96, loc.tr(txt::quoteAuthor), {kGameWidth / 2.0f + 4.0f, kGameHeight / 2.0f + 76.0f}, 33.0f, 720.0f, WHITE, q3);

    drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/quote/quote0004.png"), q4);
    drawRendererTextCenteredFit(renderer, font96, loc.tr(txt::misattributed), {kGameWidth / 2.0f + 4.0f, kGameHeight / 2.0f + 135.0f}, 33.0f, 720.0f, WHITE, q4);
}

CreditsScene::CreditsScene(Game& game) : Scene(game) {}

void CreditsScene::enter() {
    elapsed_ = 0.0f;
    for (int i = 1; i <= 8; ++i) {
        game_.assets().textureHandle("sprites/credits/credits000" + std::to_string(i) + ".png");
    }
    game_.assets().fontHandle(64);
    game_.assets().fontHandle(96);
    game_.audio().preloadMusic("sounds/bg_nighttime.mp3");
}

void CreditsScene::exit() {
    for (int i = 1; i <= 8; ++i) {
        game_.assets().releaseTexture("sprites/credits/credits000" + std::to_string(i) + ".png");
    }
}

void CreditsScene::update(float dt) {
    elapsed_ += dt;
    if (elapsed_ >= 22.0f) {
        game_.audio().playMusic("sounds/bg_nighttime.mp3", 0.0f, true);
        game_.audio().fadeMusic("sounds/bg_nighttime.mp3", 0.0f, 1.0f, 2.0f);
    }
    if (elapsed_ > 28.0f || game_.keyPressed(InputKey::Space) || game_.keyPressed(InputKey::Enter)) {
        game_.requestScene(SceneId::PostCredits);
    }
}

void CreditsScene::draw() {
    render::Renderer& renderer = game_.renderer();
    const render::FontHandle font64 = game_.assets().fontHandle(64);
    const render::FontHandle font96 = game_.assets().fontHandle(96);
    renderer.DrawRectangle({0.0f, 0.0f, static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)}, toRender(BLACK));
    const struct CreditSlot { int index; float start; float end; } slots[] = {
        {1, 4.0f, 8.0f}, {2, 9.0f, 13.0f}, {3, 14.0f, 16.0f}, {4, 16.0f, 18.0f},
        {5, 18.0f, 20.0f}, {6, 20.0f, 23.0f}, {7, 24.0f, 28.0f}, {8, 25.0f, 28.0f}
    };
    for (const auto& slot : slots) {
        float a = fadeInAt(elapsed_, slot.start, 0.8f) * fadeOutAt(elapsed_, slot.end, 0.8f);
        if (slot.index >= 7) a = fadeInAt(elapsed_, slot.start, 0.8f) * fadeOutAt(elapsed_, 27.0f, 1.0f);
        if (a > 0.0f) drawRendererFullscreen(renderer, game_.assets().textureHandle("sprites/credits/credits000" + std::to_string(slot.index) + ".png"), a);
    }
    if (elapsed_ >= 4.0f && elapsed_ < 8.2f) {
        const float a = fadeInAt(elapsed_, 4.0f, 0.8f) * fadeOutAt(elapsed_, 8.0f, 0.8f);
        const Localization& loc = game_.localization();
        drawRendererText(renderer, font96, loc.tr(txt::createdBy), {240.0f, 205.0f}, 46.0f, WHITE, a);
        drawRendererText(renderer, font96, loc.tr(txt::nickyCase), {240.0f, 273.0f}, 86.0f, WHITE, a);
        drawRendererText(renderer, font64, loc.tr(txt::portedByJoseph), {244.0f, 372.0f}, 35.0f, Color{210, 210, 210, 255}, a);
        drawPorterCharacter(renderer, font64, {770.0f, 331.0f}, 0.95f, a, loc.tr(txt::porterShirt).c_str());
    }
    if (elapsed_ >= 24.0f) {
        const float a = fadeInAt(elapsed_, 24.0f, 0.8f) * fadeOutAt(elapsed_, 27.0f, 1.0f);
        const Localization& loc = game_.localization();
        drawRendererText(renderer, font96, loc.tr(txt::lastButNotLeast), {330.0f, 205.0f}, 37.0f, WHITE, a);
        drawRendererTextCenteredFit(renderer, font96, loc.tr(txt::thankYouForPlaying), {kGameWidth / 2.0f + 10.0f, kGameHeight / 2.0f + 25.0f}, 55.0f, 780.0f, WHITE, a);
    }
}

PostCreditsScene::PostCreditsScene(Game& game) : Scene(game) {}

void PostCreditsScene::enter() {
    elapsed_ = 0.0f;
    fade_ = 1.0f;
    camera_ = {static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)};
    game_.assets().textureHandle("sprites/bg_dark.png");
    game_.assets().textureHandle("sprites/bg_shade.png");
    game_.assets().textureHandle("sprites/cam/cam.png");
    game_.assets().atlas("sprites/misc/candlelight.json");
    game_.assets().atlas("sprites/misc/cricket.json");
    game_.assets().atlas("sprites/misc/lovers_watching.json");
}

void PostCreditsScene::update(float dt) {
    elapsed_ += dt;
    fade_ = std::max(0.0f, fade_ - dt);
    camera_ = game_.logicalMouse();
    camera_.x = std::clamp(camera_.x, kGameWidth / 8.0f, static_cast<float>(kGameWidth) - kGameWidth / 8.0f);
    camera_.y = std::clamp(camera_.y, kGameHeight / 8.0f, static_cast<float>(kGameHeight) - kGameHeight / 8.0f);
    if (game_.logicalMousePressed() || game_.keyPressed(InputKey::Space) || game_.keyPressed(InputKey::Enter)) {
        game_.audio().stopMusic("sounds/bg_nighttime.mp3");
        game_.requestScene(SceneId::Final);
    }
}

void PostCreditsScene::drawWorld(Vector2 screenOrigin, Vector2 worldTopLeft, float scale, bool includeStream) {
    auto project = [&](Vector2 point) {
        return Vector2{
            screenOrigin.x + (point.x - worldTopLeft.x) * scale,
            screenOrigin.y + (point.y - worldTopLeft.y) * scale
        };
    };

    render::Renderer& renderer = game_.renderer();
    drawRendererTextureAnchored(renderer, game_.assets().textureHandle("sprites/bg_dark.png"), project({-100.0f, -100.0f}), {0.0f, 0.0f}, {scale, scale}, 0.0f, WHITE, 1.0f);
    drawRendererTextureAnchored(renderer, game_.assets().textureHandle("sprites/bg_shade.png"), project({0.0f, 0.0f}), {0.0f, 0.0f}, {scale, scale}, 0.0f, WHITE, 1.0f);

    if (includeStream) {
        const Rectangle stream{
            kGameWidth / 2.0f - kGameWidth / 16.0f - 2.0f,
            kGameHeight / 2.0f - kGameHeight / 16.0f + 2.0f,
            kGameWidth / 8.0f,
            kGameHeight / 8.0f
        };
        const Vector2 streamWorldTopLeft{camera_.x - kGameWidth / 8.0f, camera_.y - kGameHeight / 8.0f};
        renderer.PushScissor(toRender(stream));
        drawWorld({stream.x, stream.y}, streamWorldTopLeft, 0.5f, false);
        renderer.PopScissor();
    }

    const Vector2 candles[] = {
        {468.6f, 276.6f}, {535.7f, 281.6f}, {679.7f, 279.1f}, {612.0f, 281.6f},
        {490.1f, 314.1f}, {421.8f, 309.0f}, {363.1f, 301.1f}, {786.9f, 304.4f},
        {726.1f, 310.5f}, {656.9f, 309.5f}, {869.8f, 350.3f}, {820.0f, 373.5f},
        {768.2f, 382.8f}, {698.4f, 389.0f}, {464.6f, 386.1f}, {396.3f, 382.3f},
        {339.6f, 370.1f}, {294.3f, 350.3f}
    };
    for (const Vector2& candle : candles) {
        drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/misc/candlelight.json", "candlelight", static_cast<int>(std::fmod(elapsed_ * 6.0f + candle.x, 5.0f)), project({candle.x - 100.0f, candle.y - 100.0f}), {0.5f, 0.5f}, {scale, scale});
    }
    drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/misc/lovers_watching.json", "lovers_watching", 0, project({520.0f, 380.0f}), {0.5f, 1.0f}, {scale, scale});
    drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/misc/lovers_watching.json", "lovers_watching", 1, project({550.0f, 375.0f}), {0.5f, 1.0f}, {scale, scale});
    drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/misc/cricket.json", "cricket", 1, project({400.0f, 353.0f}), {0.5f, 1.0f}, {0.25f * scale, 0.25f * scale});
    drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/misc/cricket.json", "cricket", 1, project({420.0f, 370.0f}), {0.5f, 1.0f}, {0.25f * scale, 0.25f * scale});
    drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/misc/cricket.json", "cricket", 1, project({450.0f, 380.0f}), {0.5f, 1.0f}, {0.25f * scale, 0.25f * scale});
}

void PostCreditsScene::drawCamera() {
    drawRendererTextureAnchored(game_.renderer(), game_.assets().textureHandle("sprites/cam/cam.png"), camera_, {0.5f, 0.5f}, {0.5f, 0.5f}, 0.0f, WHITE, 1.0f);
}

void PostCreditsScene::draw() {
    drawWorld({0.0f, 0.0f}, {0.0f, 0.0f}, 1.0f, true);
    drawCamera();
    if (fade_ > 0.0f) game_.renderer().DrawRectangle({0.0f, 0.0f, static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)}, toRender(BLACK), fade_);
}

FinalScene::FinalScene(Game& game) : Scene(game) {}

void FinalScene::enter() {
    game_.assets().textureHandle("sprites/postcredits/facebook.png");
    game_.assets().textureHandle("sprites/postcredits/twitter.png");
    game_.assets().fontHandle(64);
    game_.assets().fontHandle(96);
    game_.assets().atlas("sprites/postcredits/end_button.json");
    game_.audio().preloadSound("sounds/squeak.mp3");
}

void FinalScene::update(float) {
    const Vector2 mouse = game_.logicalMouse();
    const Rectangle otherWork{250.0f - 103.5f, 325.0f - 28.5f, 207.0f, 57.0f};
    const Rectangle coffee{480.0f - 103.5f, 325.0f - 28.5f, 207.0f, 57.0f};
    const Rectangle replay{710.0f - 103.5f, 325.0f - 28.5f, 207.0f, 57.0f};
    const Rectangle facebook{kGameWidth / 2.0f - 38.0f - 16.0f, 419.0f - 16.0f, 32.0f, 32.0f};
    const Rectangle twitter{kGameWidth / 2.0f + 13.0f - 16.0f, 419.0f - 16.0f, 32.0f, 32.0f};

    if (game_.logicalMousePressed() && CheckCollisionPointRec(mouse, otherWork)) {
        game_.audio().playSound("sounds/squeak.mp3");
        OpenURL("http://afzl95.github.io/");
    } else if (game_.logicalMousePressed() && CheckCollisionPointRec(mouse, coffee)) {
        game_.audio().playSound("sounds/squeak.mp3");
        OpenURL("https://twitter.com/ali_fzl95");
    } else if (game_.logicalMousePressed() && CheckCollisionPointRec(mouse, facebook)) {
        game_.audio().playSound("sounds/squeak.mp3");
        OpenURL("https://www.facebook.com/sharer/sharer.php?u=&t=I%20just%20played%20a%20cool%20game");
    } else if (game_.logicalMousePressed() && CheckCollisionPointRec(mouse, twitter)) {
        game_.audio().playSound("sounds/squeak.mp3");
        OpenURL("https://twitter.com/intent/tweet?text=I%20just%20played%20a%20cool%20game%0a");
    } else if ((game_.logicalMousePressed() && CheckCollisionPointRec(mouse, replay)) || game_.keyPressed(InputKey::Enter) || game_.keyPressed(InputKey::Space)) {
        game_.audio().playSound("sounds/squeak.mp3");
        game_.requestScene(SceneId::Quote);
    }
}

void FinalScene::draw() {
    render::Renderer& renderer = game_.renderer();
    const render::FontHandle font64 = game_.assets().fontHandle(64);
    const render::FontHandle font96 = game_.assets().fontHandle(96);
    renderer.DrawRectangle({0.0f, 0.0f, static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)}, toRender(BLACK));
    const Localization& loc = game_.localization();
    drawRendererTextCenteredFit(renderer, font96, loc.tr("final.title"), {kGameWidth / 2.0f, 205.0f}, 50.0f, 880.0f, WHITE);
    drawRendererTextCenteredFit(renderer, font64, loc.tr("final.share_pain"), {kGameWidth / 2.0f, 270.0f}, 27.0f, 500.0f, WHITE);
    const Vector2 mouse = game_.logicalMouse();
    const struct Button { Vector2 center; const char* labelKey; Rectangle rect; } buttons[] = {
        {{250.0f, 325.0f}, "final.other_work", {250.0f - 103.5f, 325.0f - 28.5f, 207.0f, 57.0f}},
        {{480.0f, 325.0f}, "final.buy_coffee", {480.0f - 103.5f, 325.0f - 28.5f, 207.0f, 57.0f}},
        {{710.0f, 325.0f}, "final.replay", {710.0f - 103.5f, 325.0f - 28.5f, 207.0f, 57.0f}}
    };
    for (const Button& button : buttons) {
        const bool hover = CheckCollisionPointRec(mouse, button.rect);
        const float scale = hover ? 1.05f : 1.0f;
        drawRendererAtlasFrameIndex(renderer, game_.assets(), "sprites/postcredits/end_button.json", "end_button", hover ? 1 : 0, button.center, {0.5f, 0.5f}, {scale, scale});
        drawRendererTextCenteredFit(renderer, font64, loc.tr(button.labelKey), {button.center.x + 1.5f * scale, button.center.y + 1.5f * scale}, 22.0f * scale, 180.0f * scale, BLACK, 0.65f);
        drawRendererTextCenteredFit(renderer, font64, loc.tr(button.labelKey), button.center, 22.0f * scale, 180.0f * scale, WHITE);
    }

    const Rectangle facebook{kGameWidth / 2.0f - 38.0f - 16.0f, 419.0f - 16.0f, 32.0f, 32.0f};
    const Rectangle twitter{kGameWidth / 2.0f + 13.0f - 16.0f, 419.0f - 16.0f, 32.0f, 32.0f};
    const float facebookScale = CheckCollisionPointRec(mouse, facebook) ? 1.2f : 1.0f;
    const float twitterScale = CheckCollisionPointRec(mouse, twitter) ? 1.2f : 1.0f;
    drawRendererTextureAnchored(renderer, game_.assets().textureHandle("sprites/postcredits/facebook.png"), {kGameWidth / 2.0f - 38.0f, 419.0f}, {0.5f, 0.5f}, {facebookScale, facebookScale}, 0.0f, WHITE, 1.0f);
    drawRendererTextureAnchored(renderer, game_.assets().textureHandle("sprites/postcredits/twitter.png"), {kGameWidth / 2.0f + 13.0f, 419.0f}, {0.5f, 0.5f}, {twitterScale, twitterScale}, 0.0f, WHITE, 1.0f);
}

}  // namespace wb
