#include "Scenes.h"

#include "AssetManager.h"
#include "AudioManager.h"
#include "Constants.h"
#include "Game.h"
#include "Text.h"
#include "Tween.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <vector>

namespace wb {
namespace {

constexpr float kTau = PI * 2.0f;
constexpr float kCameraWidth = kGameWidth / 4.0f;
constexpr float kCameraHeight = kGameHeight / 4.0f;
constexpr float kDrawingScale = 0.65f;

class Gameplay;
class Prop;
class Peep;
class NormalPeep;
class AngryPeep;
class PanicPeep;

struct View {
    Vector2 pivot{static_cast<float>(kGameWidth) / 2.0f, static_cast<float>(kGameHeight) / 2.0f};
    Vector2 center{static_cast<float>(kGameWidth) / 2.0f, static_cast<float>(kGameHeight) / 2.0f};
    Vector2 sceneOffset{0.0f, 0.0f};
    float scale = 1.0f;
};

Vector2 applyView(const View& view, Vector2 point) {
    return {
        view.sceneOffset.x + view.center.x + (point.x - view.pivot.x) * view.scale,
        view.sceneOffset.y + view.center.y + (point.y - view.pivot.y) * view.scale
    };
}

float rnd(std::mt19937& rng, float minValue = 0.0f, float maxValue = 1.0f) {
    std::uniform_real_distribution<float> dist(minValue, maxValue);
    return dist(rng);
}

bool chance(std::mt19937& rng, float probability) {
    return rnd(rng) < probability;
}

int frameTicks(float seconds) {
    return std::max(0, static_cast<int>(std::ceil(seconds * kFrameRate)));
}

float signNonZero(float value) {
    return value < 0.0f ? -1.0f : 1.0f;
}

render::Color toRender(Color value) {
    return {value.r, value.g, value.b, value.a};
}

render::Vec2 toRender(Vector2 value) {
    return {value.x, value.y};
}

render::Rect toRender(Rectangle value) {
    return {value.x, value.y, value.width, value.height};
}

float fitRendererFontSize(render::Renderer& renderer, render::FontHandle font, const std::string& text, float size, float maxWidth, float minSize = 8.0f) {
    float fitted = size;
    while (fitted > minSize && renderer.MeasureText(font, text, fitted).x > maxWidth) {
        fitted -= 0.5f;
    }
    return fitted;
}

void drawRendererTextureAnchored(render::Renderer& renderer, render::TextureHandle texture, Vector2 position, Vector2 anchor, Vector2 scale, float rotationRadians, Color tint, float alpha) {
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
    if (scale.x < 0.0f) {
        params.origin.x = width - params.origin.x;
    }
    if (scale.y < 0.0f) {
        params.origin.y = height - params.origin.y;
    }
    params.rotationRadians = rotationRadians;
    params.tint = toRender(tint);
    params.alpha = alpha;
    params.flipX = scale.x < 0.0f;
    params.flipY = scale.y < 0.0f;
    renderer.DrawTextureSource(params);
}

void drawRendererText(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 position, float size, Color color, float alpha = 1.0f) {
    renderer.DrawText({font, text, toRender(position), size, toRender(color), alpha});
}

void drawRendererOutlinedTextCentered(render::Renderer& renderer, render::FontHandle font, const std::string& text, Vector2 center, float size, float maxWidth, float outline, Color fill, Color stroke, float alpha) {
    const float fitted = fitRendererFontSize(renderer, font, text, size, maxWidth, std::max(8.0f, size * 0.55f));
    const render::Vec2 measured = renderer.MeasureText(font, text, fitted);
    const Vector2 pos{center.x - measured.x / 2.0f, center.y - measured.y / 2.0f};
    for (int ox = -1; ox <= 1; ++ox) {
        for (int oy = -1; oy <= 1; ++oy) {
            if (ox == 0 && oy == 0) {
                continue;
            }
            drawRendererText(renderer, font, text, {pos.x + static_cast<float>(ox) * outline, pos.y + static_cast<float>(oy) * outline}, fitted, stroke, alpha);
        }
    }
    drawRendererText(renderer, font, text, pos, fitted, fill, alpha);
}

bool pixelsLookBlack(const std::vector<std::uint8_t>& pixels) {
    if (pixels.empty()) {
        return true;
    }
    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
        if (pixels[i + 3] > 8 && (pixels[i] > 8 || pixels[i + 1] > 8 || pixels[i + 2] > 8)) {
            return false;
        }
    }
    return true;
}

struct Movie {
    Atlas* atlas = nullptr;
    render::TextureHandle texture{};
    std::vector<std::string> frames;
    int frame = 0;
    Vector2 anchor{0.5f, 1.0f};
    Vector2 scale{kDrawingScale, kDrawingScale};
    Vector2 local{0.0f, 0.0f};
    float rotation = 0.0f;
    float alpha = 1.0f;
    Color tint = WHITE;
    bool visible = true;

    int totalFrames() const {
        return static_cast<int>(frames.size());
    }

    void gotoAndStop(int nextFrame) {
        if (frames.empty()) {
            frame = 0;
            return;
        }
        frame = std::clamp(nextFrame, 0, static_cast<int>(frames.size()) - 1);
    }

    void draw(render::Renderer& renderer, const View& view, Vector2 parentPosition, float parentFlip, float parentScaleX, float parentScaleY, float parentRotation, float parentAlpha = 1.0f, Vector2 parentPivot = {0.0f, 0.0f}) const {
        if (!visible || !atlas || frames.empty()) {
            return;
        }

        (void)parentFlip;
        std::array<Vector2, 4> worldVertices = worldCorners(parentPosition, parentScaleX, parentScaleY, parentRotation, parentPivot);
        const bool flipX = parentScaleX * scale.x < 0.0f;
        const bool flipY = parentScaleY * scale.y < 0.0f;
        if (flipX) {
            std::swap(worldVertices[0], worldVertices[3]);
            std::swap(worldVertices[1], worldVertices[2]);
        }
        if (flipY) {
            std::swap(worldVertices[0], worldVertices[1]);
            std::swap(worldVertices[3], worldVertices[2]);
        }

        std::array<Vector2, 4> vertices{};
        for (std::size_t i = 0; i < worldVertices.size(); ++i) {
            vertices[i] = applyView(view, worldVertices[i]);
        }

        const AtlasFrame& atlasFrame = atlas->frame(frames[frame]);
        render::DrawTextureQuadParams params{};
        params.texture = texture;
        params.source = atlasFrame.frame;
        params.vertices = {{
            toRender(vertices[0]),
            toRender(vertices[1]),
            toRender(vertices[2]),
            toRender(vertices[3])
        }};
        params.tint = toRender(tint);
        params.alpha = parentAlpha * alpha;
        params.flipX = flipX;
        params.flipY = flipY;
        renderer.DrawTextureQuad(params);
    }

    std::array<Vector2, 4> worldCorners(Vector2 parentPosition, float parentScaleX, float parentScaleY, float parentRotation, Vector2 parentPivot = {0.0f, 0.0f}) const {
        const AtlasFrame& atlasFrame = atlas->frame(frames[frame]);
        const float left = atlasFrame.spriteSourceSize.x - anchor.x * atlasFrame.sourceSize.x;
        const float top = atlasFrame.spriteSourceSize.y - anchor.y * atlasFrame.sourceSize.y;
        const float right = left + atlasFrame.logicalFrameSize.x;
        const float bottom = top + atlasFrame.logicalFrameSize.y;
        const std::array<Vector2, 4> corners{{
            {left, top},
            {left, bottom},
            {right, bottom},
            {right, top}
        }};

        const float childCos = std::cos(rotation);
        const float childSin = std::sin(rotation);
        const float parentCos = std::cos(parentRotation);
        const float parentSin = std::sin(parentRotation);

        std::array<Vector2, 4> vertices{};
        for (std::size_t i = 0; i < corners.size(); ++i) {
            const float childX = corners[i].x * scale.x;
            const float childY = corners[i].y * scale.y;
            const Vector2 parentLocal{
                local.x + childX * childCos - childY * childSin,
                local.y + childX * childSin + childY * childCos
            };
            const float parentX = (parentLocal.x - parentPivot.x) * parentScaleX;
            const float parentY = (parentLocal.y - parentPivot.y) * parentScaleY;
            const Vector2 world{
                parentPosition.x + parentX * parentCos - parentY * parentSin,
                parentPosition.y + parentX * parentSin + parentY * parentCos
            };
            vertices[i] = world;
        }
        return vertices;
    }
};

std::string atlasPathFor(const std::string& resource) {
    static const std::unordered_map<std::string, std::string> paths = {
        {"candlelight", "sprites/misc/candlelight.json"},
        {"cricket", "sprites/misc/cricket.json"},
        {"cursor", "sprites/misc/cursor.json"},
        {"lovers_watching", "sprites/misc/lovers_watching.json"},
        {"preload_play", "sprites/misc/preload_play.json"},
        {"end_button", "sprites/postcredits/end_button.json"},
        {"blood", "sprites/peeps/blood.json"},
        {"body", "sprites/peeps/body.json"},
        {"body_red", "sprites/peeps/body_red.json"},
        {"face", "sprites/peeps/face.json"},
        {"face_angry", "sprites/peeps/face_angry.json"},
        {"face_murder", "sprites/peeps/face_murder.json"},
        {"face_nervous", "sprites/peeps/face_nervous.json"},
        {"face_snobby", "sprites/peeps/face_snobby.json"},
        {"face_snobby_hmph", "sprites/peeps/face_snobby_hmph.json"},
        {"gore", "sprites/peeps/gore.json"},
        {"gore_bodies", "sprites/peeps/gore_bodies.json"},
        {"gun", "sprites/peeps/gun.json"},
        {"hangry", "sprites/peeps/hangry.json"},
        {"happy_weirdo", "sprites/peeps/happy_weirdo.json"},
        {"hat", "sprites/peeps/hat.json"},
        {"hatguy", "sprites/peeps/hatguy.json"},
        {"helping", "sprites/peeps/helping.json"},
        {"lovehat", "sprites/peeps/lovehat.json"},
        {"lover_panic", "sprites/peeps/lover_panic.json"},
        {"lover_shirt", "sprites/peeps/lover_shirt.json"},
        {"peace", "sprites/peeps/peace.json"},
        {"weapon_axe", "sprites/peeps/weapon_axe.json"},
        {"weapon_bat", "sprites/peeps/weapon_bat.json"},
        {"weapon_gun", "sprites/peeps/weapon_gun.json"},
        {"weapon_shotgun", "sprites/peeps/weapon_shotgun.json"}
    };

    auto it = paths.find(resource);
    if (it == paths.end()) {
        return {};
    }
    return it->second;
}

Movie makeMovie(AssetManager& assets, const std::string& resource) {
    Movie movie;
    const std::string atlasPath = atlasPathFor(resource);
    if (atlasPath.empty()) {
        throw std::runtime_error("No atlas path for movie resource: " + resource);
    }
    Atlas& atlas = assets.atlas(atlasPath);
    movie.atlas = &atlas;
    movie.texture = assets.atlasTextureHandle(atlasPath);
    movie.frames = atlas.framesByPrefix(resource);
    if (movie.frames.empty()) {
        throw std::runtime_error("Atlas has no frames for movie resource '" + resource + "' in " + atlasPath);
    }
    return movie;
}

struct AvoidSpot {
    float x = 0.0f;
    float y = 0.0f;
    float radius = 0.0f;
};

class Prop {
public:
    explicit Prop(Gameplay& gameplay, std::string className);
    virtual ~Prop() = default;

    virtual void update(float dt) { timers.update(dt); }
    virtual void draw(const View& view) const = 0;
    virtual Rectangle cameraBounds() const { return {x - width / 2.0f, y + z - height, width, height}; }
    virtual void kill();

    Gameplay& gameplay;
    std::string className;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float width = 40.0f;
    float height = 40.0f;
    bool dead = false;
    TimerQueue timers;
};

class World {
public:
    explicit World(Gameplay& gameplay);

    void update(float dt);
    void draw(const View& view) const;
    void drawToPhoto(const View& view) const;

    template <typename T, typename... Args>
    T& addProp(Args&&... args) {
        auto value = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *value;
        storeProp(std::move(value));
        return ref;
    }

    template <typename T, typename... Args>
    T& addPeep(Args&&... args) {
        T& ref = addProp<T>(std::forward<Args>(args)...);
        storePeep(&ref);
        return ref;
    }

    void clearPeeps();
    void addBalancedPeeps(int count);
    Peep& replacePeep(Peep& oldPeep, std::unique_ptr<Peep> replacement);
    void replaceWatcher(const std::string& type, std::unique_ptr<Peep> replacement);
    void sortForDepth();
    void cleanup();

    Gameplay& gameplay;
    std::string background = "sprites/bg.png";
    std::vector<std::unique_ptr<Prop>> props;
    std::vector<Peep*> peeps;
    std::vector<Prop*> bgProps;

private:
    void storeProp(std::unique_ptr<Prop> value);
    void storePeep(Peep* peep);
    void flushPendingAdds();

    bool updatingProps_ = false;
    std::vector<std::unique_ptr<Prop>> pendingProps_;
    std::vector<Peep*> pendingPeeps_;
};

class CameraRig {
public:
    explicit CameraRig(Gameplay& gameplay);
    ~CameraRig();

    void update(float dt);
    void draw(const View& view) const;
    void takePhoto();
    void capturePhoto();
    bool ensurePhotoTargetResolution();
    bool ensurePhotoSourceTargetResolution(std::uint32_t width, std::uint32_t height);
    void hide();
    void reset();
    bool isOverTV(bool smaller) const;
    bool hasCapturedPhoto() const { return photoValid; }
    bool capturedPhotoLooksBlack() const { return photoLooksBlack; }
    render::TextureHandle photoTexture() const { return photoTextureHandle; }

    Gameplay& gameplay;
    float x = kGameWidth / 2.0f;
    float y = kGameHeight / 2.0f;
    float displayX = kGameWidth / 2.0f;
    float displayY = kGameHeight / 2.0f;
    float displayScale = 1.0f;
    bool frozen = false;
    bool visible = true;
    bool noSounds = false;
    float flashAlpha = 0.0f;
    float instructionTimer = 0.0f;
    float instructionAlpha = 0.0f;
    bool instructionDismissed = false;
    float photoAlpha = 0.0f;
    bool photoValid = false;
    bool photoLooksBlack = true;
    bool warnedInvalidPhoto = false;
    bool warnedBlackPhoto = false;
    bool warnedCaptureUnavailable = false;
    bool loggedCaptureInfo = false;
    render::RenderTargetHandle photoTarget{};
    render::TextureHandle photoTextureHandle{};
    render::RenderTargetHandle photoSourceTarget{};
    render::TextureHandle photoSourceTextureHandle{};
};

class TV : public Prop {
public:
    explicit TV(Gameplay& gameplay);
    void update(float dt) override;
    void draw(const View& view) const override;
    void placePhoto(render::TextureHandle texture, const std::string& text, bool fail, bool nothing);

    Vector2 offset{0.0f, -113.5f};
    float offsetScale = 8.0f;
    render::TextureHandle photo{};
    std::string chyronText;
    std::string chyronTexture = "sprites/chyron.png";
    float chyronAlpha = 0.0f;
    float chyronX = 15.0f;
    bool showNothing = false;
};

struct PhotoData {
    int audience = 0;
    int audienceCircles = 0;
    int audienceSquares = 0;
    bool hijack = false;
    bool forceChyron = false;
    bool itsNothing = false;
    bool caughtCricket = false;
    bool caughtHat = false;
    bool caughtLovers = false;
    bool caughtCrazy = false;
    bool caughtNervous = false;
    bool caughtSnobby = false;
    bool caughtAngry = false;
    bool caughtAngryCircle = false;
    bool caughtAngrySquare = false;
};

enum class Stage {
    Hat,
    Lovers,
    Screamer,
    Nervous,
    Snobby,
    Angry,
    Evil,
    Panic
};

class Director {
public:
    explicit Director(Gameplay& gameplay);

    void update(float dt);
    void takePhoto();
    std::vector<Prop*> propsInCamera(const std::function<bool(Prop&)>& filter = {}) const;
    Prop* firstInCamera(const std::function<bool(Prop&)>& filter = {}) const;
    bool cameraContains(Prop& prop) const;

    void movePhoto();
    void cutToTV();
    void zoomOut1();
    void zoomOut2();
    void reset();
    void audienceMovePhoto();
    bool audienceCutToTV(const std::function<void(NormalPeep&)>& action = {}, const std::function<bool(NormalPeep&)>& filter = {});
    void cutViewportTo(Vector2 pivot, float scale);
    void tweenViewportTo(Vector2 pivot, float scale, float duration);
    void analyzePhoto();
    void cutStage();

    std::string chyron;
    PhotoData photoData;
    Stage stage = Stage::Hat;
    bool isWatchingTV = false;
    bool noSounds = false;

private:
    void chyLovers();
    bool chyHats();
    void chyPeeps();
    std::string spoutManifesto();

    Gameplay& gameplay;
    TimerQueue sequence;
    bool viewportTweening = false;
    bool warnedMissingPhoto = false;
    float viewportTime = 0.0f;
    float viewportDuration = 1.0f;
    Vector2 viewportStartPivot{};
    Vector2 viewportEndPivot{};
    float viewportStartScale = 1.0f;
    float viewportEndScale = 1.0f;
    int manifestoIndex = -1;
};

class ScreenShake {
public:
    explicit ScreenShake(Gameplay& gameplay) : gameplay(gameplay) {}
    void shake(float value);
    void update(float dt);
    void draw(const View& view) const;

    Gameplay& gameplay;
    float intensity = 0.0f;
    float snowAlpha = 0.0f;
    float baseAlpha = 0.15f;
};

class ScreenZoomOut {
public:
    explicit ScreenZoomOut(Gameplay& gameplay) : gameplay(gameplay) {}
    void init();
    void update(float dt);

    Gameplay& gameplay;
    bool started = false;
    bool completed = false;
    float scale = 1.0f;
    float timer = 40.0f;
    float fullTimer = 40.0f;
    std::function<void()> onComplete;
};

class Gameplay {
public:
    explicit Gameplay(Game& game);
    ~Gameplay();

    void enter();
    void exit();
    void update(float dt);
    void draw();
    void renderWorldToPhoto(render::RenderTargetHandle target,
                            const CameraRig& camera);

    void stageStart();
    void stageHat();
    void stageLovers();
    void stageScreamer();
    void stageNervous(bool hack = false);
    void stageSnobby(bool hack = false);
    void stageAngry(bool hack = false);
    void stageEvil(bool hack = false);
    void stagePanic();

    AssetManager& assets() { return game.assets(); }
    AudioManager& audio() { return game.audio(); }
    std::string tr(const char* key) const { return game.localization().tr(key); }
    float random(float minValue = 0.0f, float maxValue = 1.0f) { return rnd(rng, minValue, maxValue); }
    bool randomChance(float probability) { return chance(rng, probability); }

    Game& game;
    std::mt19937 rng{1337};
    std::unique_ptr<World> world;
    std::unique_ptr<CameraRig> camera;
    std::unique_ptr<Director> director;
    TV* tv = nullptr;
    std::unique_ptr<ScreenShake> shaker;
    std::unique_ptr<ScreenZoomOut> zoomer;
    std::vector<AvoidSpot> avoidSpots;
    bool noYellingYet = false;
    float sceneScale = 1.0f;
    float sceneX = 0.0f;
    float sceneY = 0.0f;
    float offX = 0.0f;
    float offY = 0.0f;
    Vector2 worldPivot{static_cast<float>(kGameWidth) / 2.0f, static_cast<float>(kGameHeight) / 2.0f};
    float worldScale = 1.0f;
    float blackoutAlpha = 1.0f;
    int panicWeaponIndex = 0;
};

Prop::Prop(Gameplay& gameplayRef, std::string name) : gameplay(gameplayRef), className(std::move(name)) {}

void Prop::kill() {
    dead = true;
}

class Peep : public Prop {
public:
    explicit Peep(Gameplay& gameplay, std::string name = "Peep") : Prop(gameplay, std::move(name)) {
        x = gameplay.random(0.0f, static_cast<float>(kGameWidth));
        y = gameplay.random(0.0f, static_cast<float>(kGameHeight));
        width = 80.0f * kDrawingScale;
        height = 120.0f * kDrawingScale;
        hop = gameplay.random();
        lastHop = hop;
        startWalking();
    }

    void update(float dt) override {
        constexpr float fixedStep = 1.0f / kFrameRate;
        constexpr int maxSteps = 4;
        updateAccumulator += std::clamp(dt, 0.0f, fixedStep * static_cast<float>(maxSteps));

        int steps = 0;
        while (updateAccumulator + 0.000001f >= fixedStep && steps < maxSteps && !dead) {
            updateFixedStep(fixedStep);
            updateAccumulator -= fixedStep;
            ++steps;
        }
        if (steps == maxSteps && updateAccumulator >= fixedStep) {
            updateAccumulator = 0.0f;
        }
    }

    void updateFixedStep(float dt) {
        timers.update(dt);
        if (dead) {
            return;
        }
        constexpr float frameScale = 1.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        if (isWalking) {
            vx = std::cos(direction) * speed;
            vy = std::sin(direction) * speed;
        }

        vel.x = vel.x * 0.9f + vx * 0.1f;
        vel.y = vel.y * 0.9f + vy * 0.1f;
        x += vel.x * frameScale;
        y += vel.y * frameScale;

        if (loop) {
            constexpr float margin = 50.0f;
            if (x < -margin) x = kGameWidth + margin;
            if (x > kGameWidth + margin) x = -margin;
            if (y < 0.0f) y = kGameHeight + margin * 2.0f;
            if (y > kGameHeight + margin * 2.0f) y = 0.0f;
        }

        if (!goThroughSpots) {
            for (const auto& spot : gameplay.avoidSpots) {
                const float dx = spot.x - x;
                const float dy = spot.y - y;
                if (dx * dx + dy * dy < spot.radius * spot.radius) {
                    direction = std::atan2(dy, dx) + PI;
                }
            }
        }

        if (isWalking) {
            walkAnim(frameScale);
        } else {
            standAnim(frameScale);
        }

        bounceVel += (1.0f - bounce) * bounceAcc * frameScale;
        bounce += bounceVel * frameScale;
        bounceVel *= std::pow(bounceDamp, frameScale);
        lastHop = hop;

        onBehavior(dt);
    }

    void draw(const View& view) const override {
        const float parentScaleX = bounce * flip;
        const float parentScaleY = 1.0f / std::max(0.01f, bounce);
        for (const auto& movie : movies) {
            movie->draw(gameplay.game.renderer(), view, {x, y}, flip, parentScaleX, parentScaleY, rotation, 1.0f, {0.0f, pivotY});
        }
    }

    Rectangle cameraBounds() const override {
        bool hasBounds = false;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        const float parentScaleX = bounce * flip;
        const float parentScaleY = 1.0f / std::max(0.01f, bounce);

        for (const auto& movie : movies) {
            if (!movie || !movie->visible || !movie->atlas || movie->frames.empty()) {
                continue;
            }
            const std::array<Vector2, 4> corners = movie->worldCorners({x, y}, parentScaleX, parentScaleY, rotation, {0.0f, pivotY});
            for (const Vector2& corner : corners) {
                if (!hasBounds) {
                    minX = maxX = corner.x;
                    minY = maxY = corner.y;
                    hasBounds = true;
                } else {
                    minX = std::min(minX, corner.x);
                    minY = std::min(minY, corner.y);
                    maxX = std::max(maxX, corner.x);
                    maxY = std::max(maxY, corner.y);
                }
            }
        }

        if (!hasBounds) {
            return Prop::cameraBounds();
        }
        return {minX, minY, maxX - minX, maxY - minY};
    }

    Movie& addMovie(const std::string& resource) {
        movies.push_back(std::make_unique<Movie>(makeMovie(gameplay.assets(), resource)));
        return *movies.back();
    }

    virtual void startWalking() {
        if (stunned) {
            return;
        }
        isWalking = true;
        speed = 1.0f + gameplay.random(0.0f, 0.5f);
        direction = gameplay.random(0.0f, kTau);
    }

    void stopWalking(bool halt = false) {
        if (halt) {
            vel = {0.0f, 0.0f};
        }
        isWalking = false;
        hop = 0.0f;
    }

    virtual void watchTV() {
        stopWalking(true);
        flip = gameplay.tv && gameplay.tv->x > x ? 1.0f : -1.0f;
        isWatching = true;
        timers.after(DirectorZoomOut1Time + DirectorSeeViewersTime + gameplay.random(0.0f, 0.4f), [this]() {
            isWatching = false;
            startWalking();
        });
    }

    virtual void getOuttaTV() {}
    virtual void setType(const std::string& nextType) { type = nextType; }

    std::vector<Peep*> touchingPeeps(float radius, const std::function<bool(Peep&)>& filter = {}, bool oneSided = false) {
        std::vector<Peep*> result;
        for (Peep* other : gameplay.world->peeps) {
            if (!other || other == this || other->dead) {
                continue;
            }
            const float dx = other->x - x;
            const float dy = (other->y - y) * 2.0f;
            if (dx * dx + dy * dy < radius * radius) {
                if (oneSided) {
                    if (flip > 0.0f && other->x <= x) continue;
                    if (flip < 0.0f && other->x >= x) continue;
                }
                if (!filter || filter(*other)) {
                    result.push_back(other);
                }
            }
        }
        return result;
    }

    void stayWithinRect(Rectangle rect, float turn) {
        if (x < rect.x && vel.x < 0.0f) direction += turn;
        if (x > rect.x + rect.width && vel.x > 0.0f) direction += turn;
        if (y < rect.y && vel.y < 0.0f) direction += turn;
        if (y > rect.y + rect.height && vel.y > 0.0f) direction += turn;
    }

    virtual void beShocked() {}
    virtual void beConfused(Peep&) {}
    virtual void beOffended(Peep&) {}
    virtual void takeOffHat(bool) {}
    virtual void wearHat() {}
    virtual bool isScreaming() const { return false; }
    virtual bool isScared() const { return false; }

    static constexpr float DirectorZoomOut1Time = 2.0f;
    static constexpr float DirectorSeeViewersTime = 2.0f;

    std::vector<std::unique_ptr<Movie>> movies;
    bool isWalking = true;
    bool loop = true;
    bool goThroughSpots = false;
    bool stunned = false;
    bool isWatching = false;
    bool shocked = false;
    bool confused = false;
    bool offended = false;
    bool wearingHat = false;
    float speed = 1.0f;
    float direction = 0.0f;
    Vector2 vel{0.0f, 0.0f};
    float flip = 1.0f;
    float hop = 0.0f;
    float lastHop = 0.0f;
    float bounce = 1.0f;
    float bounceVel = 0.0f;
    float bounceAcc = 0.2f;
    float bounceDamp = 0.6f;
    float rotation = 0.0f;
    float pivotY = 0.0f;
    float updateAccumulator = 0.0f;

protected:
    virtual void onBehavior(float) {}

    virtual void walkAnim(float frameScale) {
        hop += speed / 40.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        const float t = hop * kTau;
        rotation = std::sin(t) * 0.1f;
        pivotY = std::abs(std::sin(t)) * 10.0f;
        if (lastHop < 0.5f && hop >= 0.5f) bounce = 1.2f;
        if (lastHop > 0.9f && hop <= 0.1f) bounce = 1.2f;
    }

    virtual void standAnim(float) {
        rotation = 0.0f;
        pivotY = 0.0f;
    }
};

class NormalPeep : public Peep {
public:
    explicit NormalPeep(Gameplay& gameplay) : Peep(gameplay, "NormalPeep") {
        hat = &addMovie("hat");
        body = &addMovie("body");
        face = &addMovie("face");
        setType(gameplay.randomChance(0.5f) ? "circle" : "square");
    }

    void setType(const std::string& nextType) override {
        type = nextType;
        body->gotoAndStop(type == "circle" ? 0 : 1);
    }

    void onBehavior(float) override {
        doubles = (doubles + 1) % 3;
        if (doubles == 0) {
            if (wearingHat && hat->frame < 15) {
                hat->gotoAndStop(hat->frame + 1);
                if (hat->frame == 11) {
                    bounce = 1.6f;
                    gameplay.audio().playSound("sounds/squeak.mp3");
                }
                if (hat->frame == 15) face->gotoAndStop(6);
            }
            if (!wearingHat && hat->frame >= 15) {
                hat->gotoAndStop(hat->frame + 1);
                if (hat->frame == 26) hat->gotoAndStop(0);
            }
        }
    }

    void beStunned() {
        stunned = true;
        stopWalking(true);
        face->gotoAndStop(3);
    }

    void beShocked() override {
        if (stunned) return;
        shocked = true;
        stopWalking();
        bounce = 2.0f;
        face->gotoAndStop(3);
        timers.after(2.0f, [this]() {
            face->gotoAndStop(0);
            shocked = false;
            startWalking();
        });
    }

    void beConfused(Peep& target) override {
        flip = target.x > x ? 1.0f : -1.0f;
        confused = true;
        stopWalking();
        bounce = 1.1f;
        face->gotoAndStop(2);
        timers.after(0.2f, [this]() { face->gotoAndStop(4); });
        timers.after(2.2f, [this]() {
            face->gotoAndStop(2);
            timers.after(0.2f, [this]() { face->gotoAndStop(0); });
            confused = false;
            startWalking();
        });
    }

    void beOffended(Peep& target) override {
        flip = target.x > x ? 1.0f : -1.0f;
        offended = true;
        stopWalking();
        timers.clear();
        bounce = 1.2f;
        face->gotoAndStop(2);
        timers.after(0.15f, [this]() { face->gotoAndStop(0); });
        timers.after(1.2f, [this]() {
            bounce = 1.2f;
            face->gotoAndStop(2);
            timers.after(0.15f, [this]() { face->gotoAndStop(5); });
        });
        timers.after(3.0f, [this]() { startWalking(); });
        timers.after(8.0f, [this]() {
            face->gotoAndStop(2);
            timers.after(0.2f, [this]() {
                face->gotoAndStop(0);
                offended = false;
            });
        });
    }

    void wearHat() override {
        timers.clear();
        stopWalking(true);
        face->gotoAndStop(1);
        flip = gameplay.tv && gameplay.tv->x > x ? 1.0f : -1.0f;
        isWatching = true;
        const float wait = DirectorZoomOut1Time + DirectorSeeViewersTime + gameplay.random(0.0f, 0.4f);
        const float hatTime = DirectorZoomOut1Time + (std::abs(x - gameplay.tv->x) - 60.0f) / 100.0f;
        timers.after(std::max(0.0f, hatTime), [this]() { wearingHat = true; });
        timers.after(wait + 1.0f, [this]() {
            isWatching = false;
            bounce = 1.2f;
            startWalking();
        });
    }

    void takeOffHat(bool instant) override {
        timers.clear();
        if (instant) {
            face->gotoAndStop(0);
            hat->gotoAndStop(0);
            wearingHat = false;
            return;
        }

        stopWalking(true);
        face->gotoAndStop(1);
        flip = gameplay.tv && gameplay.tv->x > x ? 1.0f : -1.0f;
        isWatching = true;
        const float wait = 4.0f + gameplay.random(0.0f, 0.4f);
        timers.after(1.75f + gameplay.random(0.0f, 0.75f), [this]() {
            wearingHat = false;
            bounce = 1.1f;
            face->gotoAndStop(2);
            timers.after(0.2f, [this]() { face->gotoAndStop(7); });
        });
        timers.after(wait + 0.06f, [this]() {
            isWatching = false;
            bounce = 1.2f;
            face->gotoAndStop(0);
            startWalking();
        });
    }

    void getOuttaTV() override {
        shocked = false;
        confused = false;
        Rectangle bounds{kGameWidth / 2.0f - 220.0f, kGameHeight / 2.0f - 110.0f, 440.0f, 300.0f};
        if (CheckCollisionPointRec({x, y}, bounds)) {
            const float leftDistance = std::abs(x - bounds.x);
            const float rightDistance = std::abs((bounds.x + bounds.width) - x);
            const float topDistance = std::abs(y - bounds.y);
            const float bottomDistance = std::abs((bounds.y + bounds.height) - y);
            const float closest = std::min({leftDistance, rightDistance, topDistance, bottomDistance});
            const float margin = std::max(width, height) * 0.75f;

            if (closest == leftDistance) {
                x = bounds.x - margin;
            } else if (closest == rightDistance) {
                x = bounds.x + bounds.width + margin;
            } else if (closest == topDistance) {
                y = bounds.y - margin;
            } else {
                y = bounds.y + bounds.height + margin;
            }
        }
        stopWalking(true);
        timers.after(DirectorZoomOut1Time + DirectorSeeViewersTime, [this]() { startWalking(); });
    }

    void watchTV() override {
        timers.clear();
        stopWalking(true);
        face->gotoAndStop(1);
        flip = gameplay.tv && gameplay.tv->x > x ? 1.0f : -1.0f;
        const float wait = DirectorZoomOut1Time + DirectorSeeViewersTime + gameplay.random(0.0f, 0.4f);
        isWatching = true;
        timers.after(wait, [this]() {
            isWatching = false;
            face->gotoAndStop(2);
            bounce = 1.2f;
        });
        timers.after(wait + 0.06f, [this]() {
            face->gotoAndStop(0);
            startWalking();
        });
    }

    Movie* hat = nullptr;
    Movie* body = nullptr;
    Movie* face = nullptr;
    int doubles = 0;
};

class HatPeep : public Peep {
public:
    explicit HatPeep(Gameplay& gameplay) : Peep(gameplay, "HatPeep") {
        body = &addMovie("hatguy");
        width = 80.0f * kDrawingScale;
        height = 120.0f * kDrawingScale;
    }

    void onBehavior(float) override {
        stayWithinRect({100.0f, 100.0f, 760.0f, 380.0f}, 0.05f);
    }

    void walkAnim(float frameScale) override {
        hop += speed / 40.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        const float t = hop * kTau;
        rotation = std::sin(t) * 0.2f;
        pivotY = std::abs(std::sin(t)) * 7.0f;
    }

    Movie* body = nullptr;
};

class LoverPeep : public NormalPeep {
public:
    explicit LoverPeep(Gameplay& gameplay) : NormalPeep(gameplay) {
        className = "LoverPeep";
        loveHat = &addMovie("lovehat");
        shirt = &addMovie("lover_shirt");
        face->gotoAndStop(8);
        x = 1000.0f;
        y = 580.0f;
        direction = 3.3f;
        speed = 1.3f;
        stopWalking();
        timers.after(4.0f, [this]() { startWalking(); });
    }

    void setType(const std::string& nextType) override {
        NormalPeep::setType(nextType);
        if (shirt) {
            shirt->gotoAndStop(type == "circle" ? 0 : 1);
        }
    }

    void follow(LoverPeep& target) {
        follows = &target;
        x = target.x;
        y = target.y;
    }

    void onBehavior(float) override {
        if (follows && !follows->dead) {
            float tx = follows->x - std::cos(follows->direction) * 20.0f;
            float ty = follows->y - std::sin(follows->direction) * 20.0f;
            constexpr float margin = 50.0f;
            float dx = tx - x;
            while (dx > 300.0f) {
                tx -= kGameWidth + margin * 2.0f;
                dx = tx - x;
            }
            while (dx < -300.0f) {
                tx += kGameWidth + margin * 2.0f;
                dx = tx - x;
            }
            float dy = ty - y;
            while (dy > 300.0f) {
                ty -= kGameHeight + margin * 2.0f;
                dy = ty - y;
            }
            while (dy < -300.0f) {
                ty += kGameHeight + margin * 2.0f;
                dy = ty - y;
            }
            direction = std::atan2(dy, dx);
        }

        if (follows && isEmbarrassed && x > 1100.0f) {
            follows->kill();
            kill();
        }

        if (follows && x < -500.0f) {
            follows->kill();
            kill();
        }
    }

    void startWalking() override {
        NormalPeep::startWalking();
        direction = 3.3f;
        speed = 1.3f;
    }

    void walkAnim(float frameScale) override {
        if (follows) hop += speed / 120.0f * frameScale;
        NormalPeep::walkAnim(frameScale);
    }

    void makeEmbarrassed() {
        timers.clear();
        x = gameplay.tv->x + (type == "square" ? 80.0f : 120.0f);
        y = gameplay.tv->y - 5.0f - gameplay.random();
        stopWalking(true);
        face->gotoAndStop(9);
        flip = gameplay.tv->x > x ? 1.0f : -1.0f;
        isWatching = true;
        timers.after(3.7f, [this]() {
            isWatching = false;
            face->gotoAndStop(10);
            bounce = 1.2f;
        });
        timers.after(3.76f, [this]() {
            isEmbarrassed = true;
            face->gotoAndStop(11);
            startWalking();
            loop = false;
            direction = 0.0f;
            speed = 3.0f;
        });
    }

    Movie* loveHat = nullptr;
    Movie* shirt = nullptr;
    LoverPeep* follows = nullptr;
    bool isEmbarrassed = false;
};

class CrazyPeep : public Peep {
public:
    explicit CrazyPeep(Gameplay& gameplay) : Peep(gameplay, "CrazyPeep") {
        body = &addMovie("hangry");
        body->anchor.x = 0.33f;
        x = 1200.0f;
        y = 330.0f;
        direction = PI;
        loop = false;
        speed = 0.0f;
        metaGrace = 3.5f;
        wanderGrace = 2.0f;
        grace = 4.5f;
    }

    void onBehavior(float dt) override {
        if (metaGrace > 0.0f) {
            metaGrace -= dt;
            if (metaGrace <= 0.0f) speed = 2.0f;
            return;
        }

        grace -= dt;
        wanderGrace -= dt;
        const float frameScale = dt * kFrameRate;
        if (wanderGrace <= 0.0f) {
            direction += wander * frameScale;
            const float changeChance = 1.0f - std::pow(0.95f, std::max(0.0f, frameScale));
            if (gameplay.randomChance(changeChance)) wander = gameplay.random(-0.05f, 0.05f);
            stayWithinRect({100.0f, 100.0f, 760.0f, 380.0f}, 0.15f * frameScale);
        }

        if (grace <= 0.0f) {
            auto close = touchingPeeps(50.0f, [](Peep& peep) { return peep.isWalking; });
            if (!close.empty() && isWalking) {
                isWalking = false;
                body->gotoAndStop(10);
                loopScreaming = 2;
                animationTick = 0.0f;
                direction += PI;
                gameplay.audio().playSound("sounds/scream.mp3");
                grace = 1.0f;
                for (Peep* other : close) {
                    if (!other->isWalking) continue;
                    other->vel.x = flip * 10.0f;
                    other->flip = -flip;
                    other->beShocked();
                }
            }
        }
    }

    bool isScreaming() const override {
        return loopScreaming >= 0;
    }

    void walkAnim(float frameScale) override {
        hop += speed / 50.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        pivotY = std::abs(std::sin(hop * PI)) * 20.0f;
        rotation = 0.0f;
        animationTick += frameScale;
        const float frameStep = metaGrace > 0.0f ? 1.0f : 2.0f;
        while (animationTick >= frameStep) {
            animationTick -= frameStep;
            int next = body->frame + 1;
            if (next > 9) next = 0;
            body->gotoAndStop(next);
        }
    }

    void standAnim(float frameScale) override {
        rotation = 0.0f;
        pivotY = 0.0f;
        animationTick += frameScale;
        while (animationTick >= 2.0f && !isWalking) {
            animationTick -= 2.0f;
            int next = body->frame + 1;
            if (loopScreaming > 0) {
                if (next > 19) {
                    next = 16;
                    --loopScreaming;
                }
            } else {
                if (next > 20) {
                    next = 0;
                    isWalking = true;
                    --loopScreaming;
                }
            }
            body->gotoAndStop(next);
        }
    }

    Movie* body = nullptr;
    int loopScreaming = -1;
    float animationTick = 0.0f;
    float metaGrace = 0.0f;
    float wanderGrace = 0.0f;
    float grace = 0.0f;
    float wander = 0.0f;
};

class NervousPeep : public Peep {
public:
    explicit NervousPeep(Gameplay& gameplay) : Peep(gameplay, "NervousPeep") {
        body = &addMovie("body");
        face = &addMovie("face_nervous");
        face->gotoAndStop(0);
        type = "circle";
        body->gotoAndStop(0);
    }

    void jumpStart() {
        mode = Mode::Blink;
        face->gotoAndStop(6);
    }

    void onBehavior(float) override {
        doubles = (doubles + 1) % 3;
        stayWithinRect({150.0f, 150.0f, 660.0f, 300.0f}, 0.05f);
        if (doubles == 0) animateFace();

        if (!isShocked) {
            auto close = touchingPeeps(90.0f, [](Peep& peep) { return peep.isWalking && peep.type == "square"; });
            if (!close.empty() && isWalking) {
                mode = Mode::Shocked;
                isWalking = false;
                face->gotoAndStop(17);
                isShocked = true;
                gameplay.audio().playSound("sounds/peep_gasp.mp3");
                flip = close.front()->x > x ? 1.0f : -1.0f;
                bounce = 1.5f;
                vel.x = -flip * 5.0f;
                for (Peep* other : close) {
                    if (other->isWalking) other->beConfused(*this);
                }
                timers.after(1.0f, [this]() {
                    gameplay.audio().playSound("sounds/peep_hngh.mp3", 0.6f);
                    mode = Mode::RunAway;
                    flip *= -1.0f;
                    bounce = 1.5f;
                    startWalking();
                    speed = 3.5f;
                    direction = flip > 0.0f ? 0.0f : PI;
                    timers.after(3.0f, [this]() {
                        mode = Mode::CalmDown;
                        speed = 0.8f;
                    });
                });
            }
        }
    }

    void startWalking() override {
        Peep::startWalking();
        speed = 0.8f;
    }

    void watchTV() override {
        timers.clear();
        stopWalking(true);
        flip = gameplay.tv->x > x ? 1.0f : -1.0f;
        const float wait = DirectorZoomOut1Time + DirectorSeeViewersTime;
        timers.after(2.0f, [this]() {
            bounce = 1.6f;
            mode = Mode::Stare;
            gameplay.audio().playSound("sounds/squeak.mp3");
        });
        timers.after(wait, [this]() {
            bounce = 1.3f;
            mode = Mode::Blink;
        });
        timers.after(wait + 1.0f, [this]() { startWalking(); });
    }

    bool isScared() const override {
        return isShocked;
    }

    void animateFace() {
        const int frame = face->frame;
        switch (mode) {
            case Mode::Stare:
                if (frame < 3) face->gotoAndStop(frame + 1);
                break;
            case Mode::Blink:
                if (frame < 6) face->gotoAndStop(frame + 1);
                else if (frame == 6 && gameplay.randomChance(0.05f)) face->gotoAndStop(frame + 1);
                else if (frame > 6 && frame < 10) face->gotoAndStop(frame + 1);
                else if (frame == 10 && gameplay.randomChance(0.05f)) face->gotoAndStop(frame + 1);
                else if (frame > 10 && frame < 16) face->gotoAndStop(frame + 1);
                else if (frame == 16) face->gotoAndStop(6);
                break;
            case Mode::Shocked:
                if (frame < 22) face->gotoAndStop(frame + 1);
                break;
            case Mode::RunAway:
                if (frame < 25) face->gotoAndStop(frame + 1);
                break;
            case Mode::CalmDown:
                if (frame < 29) face->gotoAndStop(frame + 1);
                else {
                    face->gotoAndStop(6);
                    mode = Mode::Blink;
                    isShocked = false;
                }
                break;
            case Mode::None:
                break;
        }
    }

    enum class Mode { None, Stare, Blink, Shocked, RunAway, CalmDown };
    Movie* body = nullptr;
    Movie* face = nullptr;
    Mode mode = Mode::None;
    int doubles = 0;
    bool isShocked = false;
};

class SnobbyPeep : public Peep {
public:
    explicit SnobbyPeep(Gameplay& gameplay) : Peep(gameplay, "SnobbyPeep") {
        type = "square";
        body = &addMovie("body");
        body->gotoAndStop(1);
        face = &addMovie("face_snobby");
        face->anchor.y = 1.25f;
        word = &addMovie("face_snobby_hmph");
        word->anchor.y = 1.25f;
        word->gotoAndStop(0);
    }

    void jumpStart() {
        mode = Mode::Blink;
        face->gotoAndStop(7);
    }

    void onBehavior(float) override {
        doubles = (doubles + 1) % 3;
        stayWithinRect({100.0f, 100.0f, 760.0f, 380.0f}, 0.05f);
        word->scale.x = kDrawingScale;
        if (doubles == 0) animateFace();

        if (!isSmug) {
            if (grace <= 0.0f) {
                auto close = touchingPeeps(90.0f, [](Peep& peep) { return !peep.offended && peep.type == "circle"; });
                if (!close.empty() && isWalking) {
                    flip = close.front()->x > x ? 1.0f : -1.0f;
                    bounce = 1.5f;
                    mode = Mode::Smug;
                    isWalking = false;
                    gameplay.audio().playSound("sounds/peep_huh.mp3");
                    for (Peep* other : close) {
                        other->beOffended(*this);
                    }
                    timers.after(0.9f, [this]() {
                        mode = Mode::Hmph;
                        isSmug = true;
                        gameplay.audio().playSound("sounds/peep_hmph.mp3");
                        timers.after(1.4f, [this]() {
                            mode = Mode::Pop;
                            timers.after(0.8f, [this]() {
                                flip *= -1.0f;
                                startWalking();
                                direction = flip > 0.0f ? 0.0f : PI;
                                direction += gameplay.random(-0.1f, 0.1f);
                                mode = Mode::Away;
                                grace = 1.0f;
                            });
                        });
                    });
                }
            } else {
                grace -= 1.0f / kFrameRate;
            }
        }
    }

    void watchTV() override {
        timers.clear();
        stopWalking(true);
        flip = gameplay.tv->x > x ? 1.0f : -1.0f;
        const float wait = DirectorZoomOut1Time + DirectorSeeViewersTime;
        timers.after(2.0f, [this]() {
            bounce = 1.6f;
            mode = Mode::Stare;
            gameplay.audio().playSound("sounds/squeak.mp3");
        });
        timers.after(wait, [this]() {
            bounce = 1.3f;
            mode = Mode::Blink;
        });
        timers.after(wait + 1.0f, [this]() { startWalking(); });
    }

    void animateFace() {
        int frame = face->frame;
        switch (mode) {
            case Mode::Stare:
                if (frame < 3) face->gotoAndStop(frame + 1);
                break;
            case Mode::Blink:
                if (frame < 7) face->gotoAndStop(frame + 1);
                break;
            case Mode::Smug:
                if (frame < 14) face->gotoAndStop(frame + 1);
                break;
            case Mode::Hmph:
                if (frame < 20) {
                    face->gotoAndStop(frame + 1);
                    if (frame >= 18) {
                        int next = word->frame == 0 ? 1 : word->frame + 1;
                        if (next > 3) next = 1;
                        word->gotoAndStop(next);
                    }
                }
                break;
            case Mode::Pop:
                word->gotoAndStop(0);
                if (frame < 25) face->gotoAndStop(frame + 1);
                break;
            case Mode::Away:
                if (frame < 29) face->gotoAndStop(frame + 1);
                else {
                    face->gotoAndStop(7);
                    mode = Mode::Blink;
                    isSmug = false;
                }
                break;
            case Mode::None:
                break;
        }
    }

    enum class Mode { None, Stare, Blink, Smug, Hmph, Pop, Away };
    Movie* body = nullptr;
    Movie* face = nullptr;
    Movie* word = nullptr;
    Mode mode = Mode::None;
    int doubles = 0;
    float grace = -1.0f;
    bool isSmug = false;
};

class AngryPeep : public Peep {
public:
    AngryPeep(Gameplay& gameplay, const std::string& peepType) : Peep(gameplay, "AngryPeep") {
        body = &addMovie("body");
        bodyRed = &addMovie("body_red");
        bodyRed->alpha = 0.0f;
        face = &addMovie("face_angry");
        face->anchor.x = 0.333333f;
        setType(peepType);
        speed = 1.7f;
    }

    void setType(const std::string& nextType) override {
        type = nextType;
        const int frame = type == "circle" ? 0 : 1;
        body->gotoAndStop(frame);
        bodyRed->gotoAndStop(frame);
    }

    void jumpStart() {
        mode = Mode::Blink;
        face->gotoAndStop(10);
    }

    void onBehavior(float) override {
        doubles = (doubles + 1) % 3;
        direction += wander;
        if (gameplay.randomChance(0.05f)) wander = gameplay.random(-0.05f, 0.05f);
        stayWithinRect({100.0f, 100.0f, 760.0f, 380.0f}, 0.15f);

        if (doubles == 0) {
            const int frame = face->frame;
            switch (mode) {
                case Mode::Stare:
                    if (frame < 5) face->gotoAndStop(frame + 1);
                    break;
                case Mode::Blink:
                    if (frame < 10) face->gotoAndStop(frame + 1);
                    break;
                case Mode::Shout:
                    if (frame < 25) face->gotoAndStop(frame + 1);
                    else {
                        face->gotoAndStop(10);
                        mode = Mode::Blink;
                        isShouting = false;
                        startWalking();
                    }
                    break;
                case Mode::None:
                    break;
            }
        }

        if (face->frame >= 10) {
            bodyRed->alpha = bodyRed->alpha * 0.9f + 0.1f;
        }

        if (grace <= 0.0f && !gameplay.noYellingYet) {
            const std::string opposite = type == "circle" ? "square" : "circle";
            auto close = touchingPeeps(90.0f, [&](Peep& peep) {
                return !peep.offended && peep.isWalking && peep.type == opposite;
            });
            if (isWalking && !close.empty()) {
                Peep& other = *close.front();
                flip = other.x > x ? 1.0f : -1.0f;
                bounce = 1.7f;
                vel.y = 0.0f;
                vel.x = 3.0f * flip;
                mode = Mode::Shout;
                isWalking = false;
                isShouting = true;

                int angryCount = 0;
                for (Peep* peep : gameplay.world->peeps) {
                    if (peep && peep->className == "AngryPeep") ++angryCount;
                }
                float volume = angryCount < 5 ? 1.0f : angryCount < 10 ? 0.70f : angryCount < 15 ? 0.42f : 0.27f;
                gameplay.audio().playSound("sounds/shout.mp3", volume);
                grace = 2.5f;

                if (other.className == "NormalPeep" && !other.shocked) {
                    other.vel.x = flip * 5.0f;
                    other.flip = -flip;
                    other.beShocked();
                }
            }
        } else {
            grace -= 1.0f / kFrameRate;
        }
    }

    void startWalking() override {
        Peep::startWalking();
        speed = 1.7f;
    }

    void walkAnim(float frameScale) override {
        hop += speed / 40.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        const float t = hop * kTau;
        pivotY = std::abs(std::sin(t)) * 15.0f;
        rotation = 0.0f;
        if (lastHop < 0.5f && hop >= 0.5f) bounce = 1.2f;
        if (lastHop > 0.9f && hop <= 0.1f) bounce = 1.2f;
    }

    void watchTV() override {
        timers.clear();
        stopWalking(true);
        flip = gameplay.tv->x > x ? 1.0f : -1.0f;
        const float offset = (std::abs(x - gameplay.tv->x) - 60.0f) / 100.0f;
        const float wait = DirectorZoomOut1Time + DirectorSeeViewersTime;
        timers.after(std::max(0.0f, 2.0f + offset), [this]() {
            bounce = 1.6f;
            mode = Mode::Stare;
            gameplay.audio().playSound("sounds/squeak.mp3");
        });
        timers.after(std::max(0.0f, wait + offset), [this]() {
            bounce = 1.3f;
            mode = Mode::Blink;
        });
        timers.after(std::max(0.0f, wait + 1.0f + offset), [this]() { startWalking(); });
    }

    enum class Mode { None, Stare, Blink, Shout };
    Movie* body = nullptr;
    Movie* bodyRed = nullptr;
    Movie* face = nullptr;
    Mode mode = Mode::None;
    int doubles = 0;
    float grace = -1.0f;
    float wander = 0.0f;
    bool isShouting = false;
};

class HappyWeirdoPeep : public Peep {
public:
    explicit HappyWeirdoPeep(Gameplay& gameplay) : Peep(gameplay, "HappyWeirdoPeep") {
        x = 115.0f;
        y = 170.0f;
        body = &addMovie("happy_weirdo");
        body->anchor.x = 0.35f;
        hop = 0.0f;
        lastHop = 0.99f;
        direction = 1.0f;
        speed = 2.0f;
    }

    void smile() { mode = Mode::Smile; }
    void frown() { mode = Mode::Frown; }

    void prepareForMurder() {
        stopWalking();
        x = 540.0f;
        y = 470.0f;
        vel = {-1.0f, 0.0f};
        flip = -1.0f;
        gameplay.avoidSpots.push_back({480.0f, 430.0f, 150.0f});
    }

    void onBehavior(float) override {
        direction += wander;
        if (gameplay.randomChance(0.05f)) wander = gameplay.random(-0.05f, 0.05f);
        stayWithinRect({100.0f, 100.0f, 760.0f, 380.0f}, 0.15f);
        doubles = (doubles + 1) % 4;
        if (doubles == 0) {
            const int frame = body->frame;
            if (mode == Mode::Smile && frame < 2) body->gotoAndStop(frame + 1);
            if (mode == Mode::Frown && frame < 5) body->gotoAndStop(frame + 1);
        }
    }

    void walkAnim(float frameScale) override {
        hop += speed / 50.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        pivotY = std::abs(std::sin(hop * PI)) * 20.0f;
        rotation = 0.0f;
        if (lastHop > 0.9f && hop <= 0.1f) bounce = 1.2f;
    }

    void standAnim(float frameScale) override {
        hop += speed / 150.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        if (mode != Mode::Frown) {
            rotation = std::sin(hop * kTau) * 0.1f;
        }
        pivotY = 0.0f;
    }

    enum class Mode { None, Smile, Frown };
    Movie* body = nullptr;
    Mode mode = Mode::None;
    int doubles = 0;
    float wander = 0.0f;
};

class EvilHatPeep : public Peep {
public:
    explicit EvilHatPeep(Gameplay& gameplay) : Peep(gameplay, "EvilHatPeep") {
        gun = &addMovie("gun");
        body = &addMovie("hatguy");
        loop = false;
        x = -300.0f;
        y = 470.0f;
        direction = 0.0f;
        goThroughSpots = true;
        speed = 1.25f;
    }

    void onBehavior(float) override {
        if (x > 420.0f && !isMurdering) {
            itsMurderTime();
        }

        doubles = (doubles + 1) % 3;
        if (mode == Mode::Gun && doubles == 0 && gun->frame < 17) {
            gun->gotoAndStop(gun->frame + 1);
        }

        if (mode == Mode::Walk) {
            speed = gameplay.director->isWatchingTV ? 0.0f : 1.25f;
        }
    }

    void walkAnim(float frameScale) override {
        hop += speed / 40.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        rotation = std::sin(hop * kTau) * 0.2f;
        pivotY = std::abs(std::sin(hop * kTau)) * 7.0f;
    }

    void itsMurderTime() {
        isMurdering = true;
        stopWalking();
        bounce = 1.2f;
        timers.after(0.5f, [this]() {
            if (victim) victim->smile();
        });
        timers.after(2.0f, [this]() {
            hasGunOut = true;
            gameplay.audio().stopMusic("sounds/bg_park.mp3");
            gameplay.audio().playSound("sounds/gun_cock.mp3");
            mode = Mode::Gun;
        });
        timers.after(2.5f, [this]() {
            if (freezeEveryone) freezeEveryone();
            if (victim) victim->frown();
        });
        timers.after(6.0f, [this]() {
            gameplay.audio().playSound("sounds/gunshot.mp3");
            if (bang) bang();
        });
    }

    enum class Mode { Walk, Gun };
    Movie* gun = nullptr;
    Movie* body = nullptr;
    HappyWeirdoPeep* victim = nullptr;
    std::function<void()> freezeEveryone;
    std::function<void()> bang;
    Mode mode = Mode::Walk;
    int doubles = 0;
    bool isMurdering = false;
    bool hasGunOut = false;
};

class Blood : public Prop {
public:
    explicit Blood(Gameplay& gameplay) : Prop(gameplay, "Blood") {
        mc = makeMovie(gameplay.assets(), "blood");
        mc.anchor = {0.5f, 0.5f};
        mc.gotoAndStop(static_cast<int>(gameplay.random(0.0f, 2.99f)));
        mc.scale = {0.0f, 0.0f};
        width = 60.0f;
        height = 60.0f;
    }

    void init(float ix, float iy, float targetScale) {
        x = ix;
        y = iy;
        gotoScale = targetScale;
    }

    void update(float dt) override {
        Prop::update(dt);
        if (done) return;
        if (std::abs(scale - gotoScale) < 0.01f) done = true;
        scale = scale * 0.8f + gotoScale * 0.2f;
        mc.scale = {scale, scale};
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y}, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    Movie mc;
    float scale = 0.0f;
    float gotoScale = 0.0f;
    bool done = false;
};

class Gore : public Prop {
public:
    explicit Gore(Gameplay& gameplay) : Prop(gameplay, "Gore") {
        mc = makeMovie(gameplay.assets(), "gore");
        mc.anchor = {0.5f, 0.5f};
        mc.gotoAndStop(static_cast<int>(gameplay.random(0.0f, 2.99f)));
        width = 30.0f;
        height = 30.0f;
    }

    void init(float dir, float velocity, float ix, float iy, float iz) {
        direction = dir;
        vx = std::cos(direction) * velocity;
        vz = std::sin(direction) * velocity;
        vy = gameplay.random(-0.5f, 0.5f);
        vr = gameplay.random(-0.5f, 0.5f);
        x = ix;
        y = iy;
        z = iz;
    }

    void update(float dt) override {
        Prop::update(dt);
        const float frameScale = dt * kFrameRate;
        x += vx * frameScale;
        y += vy * frameScale;
        z += vz * frameScale;
        rotation += vr * frameScale;
        vz += gravity * frameScale;
        if (z >= 0.0f) {
            z = 0.0f;
            vx *= 0.8f;
            vy *= 0.8f;
            vr *= 0.8f;
            if (std::abs(vz) > 1.0f) {
                auto& blood = gameplay.world->addProp<Blood>(gameplay);
                blood.init(x, y, std::abs(vz) * 0.02f);
                gameplay.world->bgProps.push_back(&blood);
                vz *= -0.2f;
            } else {
                vz = 0.0f;
            }
        }
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y + z}, 1.0f, 1.0f, 1.0f, rotation);
    }

    Movie mc;
    float direction = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float vr = 0.0f;
    float rotation = 0.0f;
    float gravity = 0.5f;
};

class DeadBody : public Prop {
public:
    explicit DeadBody(Gameplay& gameplay) : Prop(gameplay, "DeadBody") {
        mc = makeMovie(gameplay.assets(), "gore_bodies");
        mc.anchor = {132.0f / 160.0f, 91.0f / 120.0f};
        width = 100.0f;
        height = 80.0f;
    }

    void init(float dir, float velocity, float ix, float iy, float iflip, int frame) {
        direction = dir;
        vx = std::cos(direction) * velocity;
        vz = std::sin(direction) * velocity;
        x = ix;
        y = iy;
        flip = iflip;
        mc.gotoAndStop(frame);
        auto& blood = gameplay.world->addProp<Blood>(gameplay);
        blood.init(x, y, 1.0f);
        gameplay.world->bgProps.push_back(&blood);
    }

    void update(float dt) override {
        Prop::update(dt);
        const float frameScale = dt * kFrameRate;
        x += vx * frameScale;
        z += vz * frameScale;
        vz += gravity * frameScale;
        if (z >= 0.0f) {
            z = 0.0f;
            vx *= 0.8f;
            if (std::abs(vz) > 1.0f) {
                auto& blood = gameplay.world->addProp<Blood>(gameplay);
                blood.init(x, y, 0.5f + std::abs(vz) * 0.1f);
                gameplay.world->bgProps.push_back(&blood);
                vz *= -0.3f;
            } else {
                vz = 0.0f;
            }
        }
        rotation = std::min((std::abs(z) / 50.0f) * (kTau / 4.0f), kTau * 0.2f);
        mc.rotation = rotation;
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y + z}, flip, flip > 0.0f ? 1.0f : -1.0f, 1.0f, 0.0f);
    }

    Movie mc;
    float direction = 0.0f;
    float vx = 0.0f;
    float vz = 0.0f;
    float gravity = 0.2f;
    float flip = 1.0f;
    float rotation = 0.0f;
};

class PanicPeep : public NormalPeep {
public:
    explicit PanicPeep(Gameplay& gameplay) : NormalPeep(gameplay) {
        className = "PanicPeep";
        face->gotoAndStop(12);
        startWalking();
    }

    void startWalking() override {
        NormalPeep::startWalking();
        speed = 3.0f + gameplay.random(0.0f, 2.0f);
    }

    void setLover(const std::string& loverType) {
        setType(loverType);
        if (loverType == "circle") {
            x = 635.0f;
            y = 200.0f;
            direction = kTau * -0.12f;
        } else {
            x = 665.0f;
            y = 200.0f;
            direction = kTau * -0.10f;
        }
        lover = &addMovie("lover_panic");
        lover->gotoAndStop(loverType == "circle" ? 0 : 1);
        loop = false;
        isLover = true;
    }

    void onBehavior(float dt) override {
        NormalPeep::onBehavior(dt);
        if (isLover && y < -500.0f) {
            kill();
        }
    }

    void walkAnim(float frameScale) override {
        hop += speed / 60.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        const float t = hop * kTau;
        rotation = std::sin(t) * 0.3f;
        pivotY = std::abs(std::sin(t)) * 15.0f;
        if (lastHop < 0.5f && hop >= 0.5f) bounce = 1.2f;
        if (lastHop > 0.9f && hop <= 0.1f) bounce = 1.2f;
    }

    void getKilledBy(const std::string& weaponType, Peep& killer) {
        int corpseFrame = 0;
        float corpseVelocity = 2.0f;
        int goreAmount = 5;
        if (weaponType == "bat") {
            corpseFrame = 1;
            corpseVelocity = 5.0f;
            goreAmount = 15;
        } else if (weaponType == "shotgun") {
            corpseFrame = 2;
            corpseVelocity = 10.0f;
            goreAmount = 30;
        } else if (weaponType == "axe") {
            corpseFrame = 3;
            corpseVelocity = 5.0f;
            goreAmount = 15;
        }

        gameplay.shaker->shake(30.0f);
        const float bodyFlip = killer.x < x ? -1.0f : 1.0f;
        const int frameOffset = type == "circle" ? 0 : 1;
        auto& bodyProp = gameplay.world->addProp<DeadBody>(gameplay);
        bodyProp.init(-kTau / 4.0f - bodyFlip * 0.7f, corpseVelocity, x, y, bodyFlip, (corpseFrame + 2) * 2 + frameOffset);

        for (int i = 0; i < goreAmount; ++i) {
            auto& gore = gameplay.world->addProp<Gore>(gameplay);
            gore.init(-kTau / 4.0f - bodyFlip * gameplay.random(0.0f, 0.5f), corpseVelocity + gameplay.random(0.0f, 7.0f), x, y, gameplay.random(-30.0f, 0.0f));
        }

        const std::string oldType = type;
        kill();
        auto& replacement = gameplay.world->addPeep<PanicPeep>(gameplay);
        replacement.setType(oldType);
        replacement.x = gameplay.randomChance(0.5f) ? -50.0f : kGameWidth + 50.0f;
        replacement.y = gameplay.random(0.0f, static_cast<float>(kGameHeight));
    }

    Movie* lover = nullptr;
    bool isLover = false;
};

class MurderPeep : public Peep {
public:
    explicit MurderPeep(Gameplay& gameplay) : Peep(gameplay, "MurderPeep") {
        body = &addMovie("body");
        face = &addMovie("face_murder");
    }

    void init(const std::string& shapeType, const std::string& weapon) {
        type = shapeType;
        body->gotoAndStop(type == "circle" ? 0 : 1);
        weaponType = weapon;
        weaponMovie = &addMovie("weapon_" + weaponType);
        weaponMovie->gotoAndStop(0);
        x = gameplay.tv->x + (type == "circle" ? -60.0f : 60.0f);
        y = gameplay.tv->y + gameplay.random();
        flip = type == "circle" ? 1.0f : -1.0f;
        grace = type == "circle" ? 0.5f : 0.84f;
        watchTV();
    }

    void onBehavior(float) override {
        doubles = (doubles + 1) % 4;
        stayWithinRect({100.0f, 100.0f, 760.0f, 380.0f}, 0.15f);
        if (doubles == 0) {
            int frame = face->frame;
            if (mode == Mode::Stare) {
                face->gotoAndStop(frame < 4 ? frame + 1 : 0);
            } else if (mode == Mode::Crazy) {
                face->gotoAndStop(frame < 13 ? frame + 1 : 9);
            }

            frame = weaponMovie->frame;
            if (mode == Mode::Crazy || mode == Mode::Kill) {
                if (frame < 5) weaponMovie->gotoAndStop(frame + 1);
                if (frame >= 6) {
                    if (frame < 15) weaponMovie->gotoAndStop(frame + 1);
                    else if (standingTime <= 0) {
                        startWalking();
                        weaponMovie->gotoAndStop(5);
                    } else {
                        --standingTime;
                    }
                }
            }
        }

        if (grace <= 0.0f && !isShocked) {
            const std::string otherType = type == "circle" ? "square" : "circle";
            auto close = touchingPeeps(120.0f, [&](Peep& peep) {
                return peep.className == "PanicPeep" && peep.type == otherType;
            });
            if (!close.empty() && isWalking) {
                flip = close.front()->x > x ? 1.0f : -1.0f;
                mode = Mode::Kill;
                weaponMovie->gotoAndStop(6);
                grace = 2.0f;
                bounce = 1.6f;
                standingTime = 5;
                stopWalking();
                if (weaponType == "gun") gameplay.audio().playSound("sounds/gunshot.mp3", 0.2f);
                else if (weaponType == "shotgun") gameplay.audio().playSound("sounds/shotgun.mp3", 0.2f);
                else gameplay.audio().playSound("sounds/impact.mp3", 0.2f);
                if (auto* panic = dynamic_cast<PanicPeep*>(close.front())) {
                    panic->getKilledBy(weaponType, *this);
                }
            }
        } else if (isWalking) {
            grace -= 1.0f / kFrameRate;
        }
    }

    void startWalking() override {
        Peep::startWalking();
        speed = 3.0f;
    }

    void walkAnim(float frameScale) override {
        hop += speed / 40.0f * frameScale;
        if (hop > 1.0f) hop -= 1.0f;
        flip = vel.x < 0.0f ? -1.0f : 1.0f;
        pivotY = std::abs(std::sin(hop * kTau)) * 15.0f;
        rotation = gameplay.random(-0.05f, 0.05f);
    }

    void standAnim(float) override {
        rotation = gameplay.random(-0.05f, 0.05f);
        pivotY = 0.0f;
    }

    void watchTV() override {
        timers.clear();
        stopWalking(true);
        flip = gameplay.tv->x > x ? 1.0f : -1.0f;
        const float wait = DirectorZoomOut1Time + DirectorSeeViewersTime;
        timers.after(2.0f, [this]() {
            bounce = 1.6f;
            mode = Mode::Crazy;
        });
        timers.after(wait + 1.0f, [this]() {
            startWalking();
            direction = type == "circle" ? PI + gameplay.random(-1.0f, 1.0f) : gameplay.random(-1.0f, 1.0f);
        });
    }

    enum class Mode { Stare, Crazy, Kill };
    Movie* body = nullptr;
    Movie* face = nullptr;
    Movie* weaponMovie = nullptr;
    std::string weaponType;
    Mode mode = Mode::Stare;
    int doubles = 0;
    int standingTime = -1;
    float grace = -1.0f;
    bool isShocked = false;
};

class AnimationProp : public Prop {
public:
    AnimationProp(Gameplay& gameplay, std::string name, float ix, float iy, const std::string& resource)
        : Prop(gameplay, std::move(name)), mc(makeMovie(gameplay.assets(), resource)) {
        x = ix;
        y = iy;
        width = 120.0f;
        height = 120.0f;
    }

    void update(float dt) override {
        Prop::update(dt);
        constexpr float fixedStep = 1.0f / kFrameRate;
        constexpr int maxSteps = 4;
        updateAccumulator += std::clamp(dt, 0.0f, fixedStep * static_cast<float>(maxSteps));

        int steps = 0;
        while (updateAccumulator + 0.000001f >= fixedStep && steps < maxSteps && !dead) {
            updateAnimation(fixedStep);
            updateAccumulator -= fixedStep;
            ++steps;
        }
        if (steps == maxSteps && updateAccumulator >= fixedStep) {
            updateAccumulator = 0.0f;
        }
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y}, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    virtual void updateAnimation(float) {}

    Movie mc;
    float updateAccumulator = 0.0f;
};

class HelpingAnim : public AnimationProp {
public:
    explicit HelpingAnim(Gameplay& gameplay) : AnimationProp(gameplay, "HelpingAnim", 180.0f, 170.0f, "helping") {
        gameplay.avoidSpots.push_back({x, y - 40.0f, 170.0f});
        gameplay.noYellingYet = true;
        grace = 2.0f * 0.8f;
    }

    void updateAnimation(float) override {
        if (gameplay.director->isWatchingTV) grace = 2.0f * 0.9f;
        if (grace > 0.0f) {
            grace -= 1.0f / kFrameRate;
            return;
        }
        triples = (triples + 1) % 3;
        if (triples != 0 || gameplay.director->isWatchingTV) return;

        if (mc.frame < 110) {
            mc.gotoAndStop(mc.frame + 1);
            if (mc.frame == 12) gameplay.audio().playSound("sounds/scream.mp3");
            if (mc.frame == 70) {
                hasHelped = true;
                gameplay.audio().playSound("sounds/squeak.mp3");
            }
        } else {
            ++frame2;
            if (frame2 > 20) {
                mc.gotoAndStop(111);
                byeLovers();
            }
            if (frame2 > 170) {
                byeSelf();
            }
        }
    }

    void byeLovers() {
        if (loversGone) return;
        loversGone = true;
        auto& lover1 = gameplay.world->addPeep<LoverPeep>(gameplay);
        lover1.x = 188.0f;
        lover1.y = 165.0f;
        lover1.setType("circle");
        lover1.startWalking();
        if (lover1.loveHat) lover1.loveHat->visible = false;
        lover1.face->gotoAndStop(6);
        lover1.bounceVel = 0.22f;
        lover1.loop = false;

        auto& lover2 = gameplay.world->addPeep<LoverPeep>(gameplay);
        lover2.follow(lover1);
        lover2.x = 235.0f;
        lover2.y = 165.0f;
        lover2.setType("square");
        lover2.startWalking();
        lover2.face->gotoAndStop(6);
        lover2.bounceVel = 0.22f;
        lover2.loop = false;

        gameplay.avoidSpots.erase(std::remove_if(gameplay.avoidSpots.begin(), gameplay.avoidSpots.end(), [this](const AvoidSpot& spot) {
            return std::abs(spot.x - x) < 0.01f && std::abs(spot.y - (y - 40.0f)) < 0.01f && std::abs(spot.radius - 170.0f) < 0.01f;
        }), gameplay.avoidSpots.end());
        gameplay.noYellingYet = false;
    }

    void byeSelf() {
        gameplay.world->addPeep<HappyWeirdoPeep>(gameplay);
        kill();
    }

    bool hasHelped = false;
    bool loversGone = false;
    int triples = 0;
    int frame2 = 0;
    float grace = 0.0f;
};

class ProtestAnim : public AnimationProp {
public:
    explicit ProtestAnim(Gameplay& gameplay) : AnimationProp(gameplay, "ProtestAnim", 650.0f, 200.0f, "peace") {
        gameplay.avoidSpots.push_back({640.0f, 150.0f, 150.0f});
    }

    void updateAnimation(float) override {
        triples = (triples + 1) % 3;
        if (triples != 0) return;
        if (stunned) {
            mc.gotoAndStop(31);
        } else {
            mc.gotoAndStop(mc.frame < 30 ? mc.frame + 1 : 0);
        }
    }

    void beStunned() {
        stunned = true;
    }

    int triples = 0;
    bool stunned = false;
};

class Cricket : public Prop {
public:
    explicit Cricket(Gameplay& gameplay) : Prop(gameplay, "Cricket"), mc(makeMovie(gameplay.assets(), "cricket")) {
        mc.scale = {0.25f, 0.25f};
        width = 137.0f * 0.25f;
        height = 137.0f * 0.25f;
        breathe = static_cast<int>(gameplay.random(0.0f, static_cast<float>(period)));
    }

    void update(float dt) override {
        Prop::update(dt);
        constexpr float fixedStep = 1.0f / kFrameRate;
        constexpr int maxSteps = 4;
        updateAccumulator += std::clamp(dt, 0.0f, fixedStep * static_cast<float>(maxSteps));

        int steps = 0;
        while (updateAccumulator + 0.000001f >= fixedStep && steps < maxSteps && !dead) {
            updateFixedStep();
            updateAccumulator -= fixedStep;
            ++steps;
        }
        if (steps == maxSteps && updateAccumulator >= fixedStep) {
            updateAccumulator = 0.0f;
        }
    }

    void updateFixedStep() {
        if (mode == Mode::Chirp) {
            ++breathe;
            if (breathe > period + 10) breathe = 0;
            if (breathe > period) {
                float s = 1.0f;
                if (breathe % 4 == 0) s = 1.1f;
                if (breathe % 4 == 2) s = 0.9f;
                mc.scale = {0.25f * s, 0.25f / s};
            } else {
                mc.scale = {0.25f, 0.25f};
            }
        }
        if (hopAwayTicks > 0) {
            --hopAwayTicks;
            if (hopAwayTicks == 0) mode = Mode::Hop;
        }
        if (mode == Mode::Hop) {
            flip = 1.0f;
            x += 3.5f;
            hop += 0.1570795f;
            z = -std::abs(std::sin(hop)) * 100.0f;
            y = gameplay.tv->y;
        }
        if (x > kGameWidth + 50.0f) kill();
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y + z}, flip, flip, 1.0f, 0.0f);
    }

    void watchTV() {
        x = gameplay.tv->x + 100.0f;
        y = gameplay.tv->y;
        flip = -1.0f;
        hopAwayTicks = frameTicks(Peep::DirectorZoomOut1Time + Peep::DirectorSeeViewersTime + 2.3f);
    }

    enum class Mode { Chirp, Hop };
    Movie mc;
    Mode mode = Mode::Chirp;
    float flip = 1.0f;
    int period = 10;
    int breathe = 0;
    float hop = 0.0f;
    int hopAwayTicks = -1;
    float updateAccumulator = 0.0f;
};

class Candlelight : public Prop {
public:
    Candlelight(Gameplay& gameplay, Vector2 position) : Prop(gameplay, "Candlelight"), mc(makeMovie(gameplay.assets(), "candlelight")) {
        x = position.x - 100.0f;
        y = position.y - 100.0f;
        mc.anchor = {0.5f, 0.5f};
    }

    void update(float dt) override {
        Prop::update(dt);
        if (gameplay.randomChance(0.2f)) {
            mc.gotoAndStop(static_cast<int>(gameplay.random(0.0f, static_cast<float>(mc.totalFrames() - 0.01f))));
        }
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y}, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    Movie mc;
};

class LoversWatching : public Prop {
public:
    LoversWatching(Gameplay& gameplay, const std::string& which) : Prop(gameplay, "LoversWatching"), mc(makeMovie(gameplay.assets(), "lovers_watching")) {
        if (which == "circle") {
            x = 520.0f;
            y = 380.0f;
            breatheSpeed = 0.025f;
            mc.gotoAndStop(0);
        } else {
            x = 550.0f;
            y = 375.0f;
            breatheSpeed = 0.020f;
            mc.gotoAndStop(1);
        }
    }

    void update(float dt) override {
        Prop::update(dt);
        const float scale = 1.0f + std::sin(breathe) * 0.02f;
        mc.scale = {scale, 1.0f / scale};
        breathe += breatheSpeed * dt * kFrameRate;
    }

    void draw(const View& view) const override {
        mc.draw(gameplay.game.renderer(), view, {x, y}, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    Movie mc;
    float breathe = 0.0f;
    float breatheSpeed = 0.0f;
};

World::World(Gameplay& gameplayRef) : gameplay(gameplayRef) {}

void World::storeProp(std::unique_ptr<Prop> value) {
    if (updatingProps_) {
        pendingProps_.push_back(std::move(value));
    } else {
        props.push_back(std::move(value));
    }
}

void World::storePeep(Peep* peep) {
    if (updatingProps_) {
        pendingPeeps_.push_back(peep);
    } else {
        peeps.push_back(peep);
    }
}

void World::flushPendingAdds() {
    if (!pendingPeeps_.empty()) {
        peeps.insert(peeps.end(), pendingPeeps_.begin(), pendingPeeps_.end());
        pendingPeeps_.clear();
    }
    if (!pendingProps_.empty()) {
        props.reserve(props.size() + pendingProps_.size());
        for (auto& prop : pendingProps_) {
            props.push_back(std::move(prop));
        }
        pendingProps_.clear();
    }
}

void World::update(float dt) {
    updatingProps_ = true;
    const std::size_t propCount = props.size();
    for (std::size_t i = 0; i < propCount; ++i) {
        auto& prop = props[i];
        if (!prop->dead) {
            prop->update(dt);
        }
    }
    updatingProps_ = false;
    flushPendingAdds();

    sortForDepth();
    cleanup();
}

void World::draw(const View& view) const {
    drawRendererTextureAnchored(
        gameplay.game.renderer(),
        gameplay.assets().textureHandle(background),
        applyView(view, {-100.0f, -100.0f}),
        {0.0f, 0.0f},
        {view.scale, view.scale},
        0.0f,
        WHITE,
        1.0f
    );

    for (Prop* bgProp : bgProps) {
        if (bgProp && !bgProp->dead) {
            bgProp->draw(view);
        }
    }

    for (const auto& prop : props) {
        if (!prop->dead && std::find(bgProps.begin(), bgProps.end(), prop.get()) == bgProps.end()) {
            prop->draw(view);
        }
    }
}

void World::drawToPhoto(const View& view) const {
    draw(view);
}

void World::clearPeeps() {
    flushPendingAdds();
    for (Peep* peep : peeps) {
        if (peep) peep->kill();
    }
    cleanup();
}

void World::addBalancedPeeps(int count) {
    int circles = 0;
    int squares = 0;
    for (Peep* peep : peeps) {
        if (!peep || peep->dead || peep->className != "NormalPeep") continue;
        if (peep->type == "circle") ++circles;
        if (peep->type == "square") ++squares;
    }

    for (int i = 0; i < count; ++i) {
        auto& peep = addPeep<NormalPeep>(gameplay);
        if (circles < squares) {
            peep.setType("circle");
            ++circles;
        } else {
            peep.setType("square");
            ++squares;
        }
    }
}

Peep& World::replacePeep(Peep& oldPeep, std::unique_ptr<Peep> replacement) {
    replacement->x = oldPeep.x;
    replacement->y = oldPeep.y;
    Peep& ref = *replacement;
    storeProp(std::move(replacement));
    storePeep(&ref);
    oldPeep.kill();
    return ref;
}

void World::replaceWatcher(const std::string& wantedType, std::unique_ptr<Peep> replacement) {
    std::vector<Peep*> watchers;
    for (Peep* peep : peeps) {
        if (peep && !peep->dead && peep->type == wantedType && peep->isWatching) {
            watchers.push_back(peep);
        }
    }
    if (watchers.empty()) {
        replacement->setType(wantedType);
        replacement->x = gameplay.tv->x + (wantedType == "circle" ? -60.0f : 60.0f);
        replacement->y = gameplay.tv->y;
        Peep& ref = *replacement;
        storeProp(std::move(replacement));
        storePeep(&ref);
        ref.watchTV();
        sortForDepth();
        return;
    }

    Peep* watcher = watchers[static_cast<std::size_t>(gameplay.random(0.0f, static_cast<float>(watchers.size() - 0.01f)))];
    Peep& ref = replacePeep(*watcher, std::move(replacement));
    ref.watchTV();
    sortForDepth();
}

void World::sortForDepth() {
    flushPendingAdds();
    std::stable_sort(props.begin(), props.end(), [](const std::unique_ptr<Prop>& a, const std::unique_ptr<Prop>& b) {
        return (a->y + a->z) < (b->y + b->z);
    });
}

void World::cleanup() {
    flushPendingAdds();
    peeps.erase(std::remove_if(peeps.begin(), peeps.end(), [](Peep* peep) {
        return !peep || peep->dead;
    }), peeps.end());
    bgProps.erase(std::remove_if(bgProps.begin(), bgProps.end(), [](Prop* prop) {
        return !prop || prop->dead;
    }), bgProps.end());
    props.erase(std::remove_if(props.begin(), props.end(), [](const std::unique_ptr<Prop>& prop) {
        return prop->dead;
    }), props.end());
}

CameraRig::CameraRig(Gameplay& gameplayRef) : gameplay(gameplayRef) {
    ensurePhotoTargetResolution();
}

CameraRig::~CameraRig() {
    if (photoSourceTarget.id != 0) {
        gameplay.game.renderer().DestroyRenderTarget(photoSourceTarget);
    }
    if (photoTarget.id != 0) {
        gameplay.game.renderer().DestroyRenderTarget(photoTarget);
    }
}

void CameraRig::update(float dt) {
    ensurePhotoTargetResolution();
    if (!frozen) {
        const Vector2 mouse = gameplay.game.logicalMouse();
        x = mouse.x;
        y = mouse.y;
    }

    x = std::clamp(x, kCameraWidth / 2.0f, static_cast<float>(kGameWidth) - kCameraWidth / 2.0f);
    y = std::clamp(y, kCameraHeight / 2.0f, static_cast<float>(kGameHeight) - kCameraHeight / 2.0f);
    if (!frozen) {
        displayX = x;
        displayY = y;
    }
    flashAlpha = std::max(0.0f, flashAlpha - dt * 4.0f);
    if (!frozen && !instructionDismissed) {
        instructionTimer += dt;
        instructionAlpha = clamp01((instructionTimer - kBeat) / (kBeat * 0.5f));
    }
}

void CameraRig::draw(const View&) const {
    if (!visible) return;
    auto scenePoint = [this](Vector2 point) {
        return Vector2{
            gameplay.sceneX + gameplay.offX + point.x * gameplay.sceneScale,
            gameplay.sceneY + gameplay.offY + point.y * gameplay.sceneScale
        };
    };
    const Vector2 pos = scenePoint({displayX, displayY});
    const float sceneDisplayScale = displayScale * gameplay.sceneScale;
    if (photoAlpha > 0.0f) {
        const Rectangle dest{pos.x, pos.y, kCameraWidth * sceneDisplayScale, kCameraHeight * sceneDisplayScale};
        render::DrawTextureParams params{};
        params.texture = photoTextureHandle;
        params.destination = toRender(dest);
        params.origin = {dest.width / 2.0f, dest.height / 2.0f};
        params.tint = {255, 255, 255, 255};
        params.alpha = photoAlpha;
        params.flipY = true;
        gameplay.game.renderer().DrawTextureSource(params);
    }

    drawRendererTextureAnchored(gameplay.game.renderer(), gameplay.assets().textureHandle("sprites/cam/cam-flash.png"), pos, {0.5f, 0.5f}, {0.5f * sceneDisplayScale, 0.5f * sceneDisplayScale}, 0.0f, WHITE, flashAlpha);
    drawRendererTextureAnchored(gameplay.game.renderer(), gameplay.assets().textureHandle("sprites/cam/cam.png"), pos, {0.5f, 0.5f}, {0.5f * sceneDisplayScale, 0.5f * sceneDisplayScale}, 0.0f, WHITE, 1.0f);
    if (instructionAlpha > 0.0f && !frozen) {
        const Vector2 instructionsPos = scenePoint({displayX, displayY + 67.5f * displayScale});
        const Vector2 instructionsCenter{instructionsPos.x, instructionsPos.y + 17.0f * sceneDisplayScale};
        drawRendererOutlinedTextCentered(
            gameplay.game.renderer(),
            gameplay.assets().fontHandle(64),
            gameplay.tr(txt::pointAndClick),
            instructionsCenter,
            32.0f * sceneDisplayScale,
            260.0f * sceneDisplayScale,
            2.3f * sceneDisplayScale,
            WHITE,
            BLACK,
            instructionAlpha
        );
    }
}

void CameraRig::takePhoto() {
    if (frozen) return;
    frozen = true;
    capturePhoto();
    flashAlpha = 1.0f;
    instructionDismissed = true;
    instructionAlpha = 0.0f;
    photoAlpha = 1.0f;
    if (!noSounds) {
        gameplay.audio().playSound("sounds/cam_snap.mp3");
    }
    gameplay.director->takePhoto();
}

void CameraRig::capturePhoto() {
    ensurePhotoTargetResolution();
    photoValid = false;
    photoLooksBlack = true;
    if (photoTarget.id == 0 || photoTextureHandle.id == 0) {
        if (!warnedInvalidPhoto) {
            TraceLog(LOG_WARNING, "Camera photo render texture is not valid; TV cut will not have captured content");
            warnedInvalidPhoto = true;
        }
        return;
    }

    if (!gameplay.game.renderer().SupportsRenderTargetCapture()) {
        if (!warnedCaptureUnavailable) {
            TraceLog(LOG_WARNING, "Renderer capture/readback is unavailable");
            warnedCaptureUnavailable = true;
        }
        return;
    }

    render::Renderer& renderer = gameplay.game.renderer();
    const render::Vec2 photoSize = renderer.TextureSize(photoTextureHandle);
    const bool fsrRequested = gameplay.game.settings().photoUpscaler == 1 && renderer.SupportsFsr1();
    bool usedFsr = false;
    if (fsrRequested) {
        constexpr float kFsrQualityScale = 2.0f / 3.0f;
        const std::uint32_t sourceWidth = std::max(
            1u,
            static_cast<std::uint32_t>(std::lround(photoSize.x * kFsrQualityScale))
        );
        const std::uint32_t sourceHeight = std::max(
            1u,
            static_cast<std::uint32_t>(std::lround(photoSize.y * kFsrQualityScale))
        );
        if (ensurePhotoSourceTargetResolution(sourceWidth, sourceHeight)) {
            gameplay.renderWorldToPhoto(photoSourceTarget, *this);
            usedFsr = renderer.UpscaleRenderTargetFsr1(photoSourceTarget, photoTarget, 1.0f);
        }
    }
    if (!usedFsr) {
        gameplay.renderWorldToPhoto(photoTarget, *this);
    }
    if (!loggedCaptureInfo) {
        TraceLog(
            LOG_INFO,
            "Camera photo capture: %ix%i texture for %.0fx%.0f logical camera crop via %s",
            static_cast<int>(photoSize.x),
            static_cast<int>(photoSize.y),
            kCameraWidth,
            kCameraHeight,
            usedFsr ? "AMD FSR 1" : "high-quality texture sampling"
        );
        loggedCaptureInfo = true;
    }
    const std::vector<std::uint8_t> pixels = renderer.CaptureRenderTargetRGBA(photoTarget);
    if (pixels.empty()) {
        if (!warnedCaptureUnavailable) {
            TraceLog(LOG_WARNING, "Renderer capture/readback returned no pixels");
            warnedCaptureUnavailable = true;
        }
        return;
    }
    photoValid = true;
    photoLooksBlack = pixelsLookBlack(pixels);
    if (photoLooksBlack && !warnedBlackPhoto) {
        TraceLog(LOG_WARNING, "Camera photo render texture is black after capture; TV cut may show empty content");
        warnedBlackPhoto = true;
    }
}

bool CameraRig::ensurePhotoTargetResolution() {
    render::Renderer& renderer = gameplay.game.renderer();
    const render::RendererStats stats = renderer.Stats();
    const std::uint32_t desiredWidth = std::max(1u, stats.renderWidth);
    const std::uint32_t desiredHeight = std::max(1u, stats.renderHeight);
    const render::Vec2 currentSize = renderer.TextureSize(photoTextureHandle);
    if (photoTarget.id != 0 && photoTextureHandle.id != 0 &&
        static_cast<std::uint32_t>(std::lround(currentSize.x)) == desiredWidth &&
        static_cast<std::uint32_t>(std::lround(currentSize.y)) == desiredHeight) {
        return true;
    }

    const render::RenderTargetHandle newTarget = renderer.CreateRenderTarget(desiredWidth, desiredHeight);
    if (newTarget.id == 0) {
        return false;
    }
    const render::TextureHandle newTexture = renderer.RenderTargetTexture(newTarget);
    if (newTexture.id == 0) {
        renderer.DestroyRenderTarget(newTarget);
        return false;
    }
    renderer.SetTextureFilter(newTexture, render::TextureFilter::Linear);

    const render::RenderTargetHandle oldTarget = photoTarget;
    const render::TextureHandle oldTexture = photoTextureHandle;
    const bool preservePhoto = photoValid && oldTarget.id != 0 && oldTexture.id != 0;
    if (preservePhoto) {
        renderer.BeginRenderTarget(newTarget);
        renderer.DrawRectangle(
            {0.0f, 0.0f, static_cast<float>(desiredWidth), static_cast<float>(desiredHeight)},
            toRender(BLACK)
        );
        render::DrawTextureParams copy{};
        copy.texture = oldTexture;
        copy.source = {0.0f, 0.0f, currentSize.x, currentSize.y};
        copy.destination = {0.0f, 0.0f, static_cast<float>(desiredWidth), static_cast<float>(desiredHeight)};
        copy.tint = {255, 255, 255, 255};
        copy.flipY = true;
        renderer.DrawTextureSource(copy);
        renderer.EndRenderTarget();
    }

    photoTarget = newTarget;
    photoTextureHandle = newTexture;
    if (gameplay.tv != nullptr && gameplay.tv->photo.id == oldTexture.id) {
        gameplay.tv->photo = newTexture;
    }
    if (oldTarget.id != 0) {
        renderer.DestroyRenderTarget(oldTarget);
    }
    loggedCaptureInfo = false;
    TraceLog(LOG_INFO, "Camera/TV capture target: %ux%u", desiredWidth, desiredHeight);
    return true;
}

bool CameraRig::ensurePhotoSourceTargetResolution(std::uint32_t width, std::uint32_t height) {
    render::Renderer& renderer = gameplay.game.renderer();
    const render::Vec2 currentSize = renderer.TextureSize(photoSourceTextureHandle);
    if (photoSourceTarget.id != 0 && photoSourceTextureHandle.id != 0 &&
        static_cast<std::uint32_t>(std::lround(currentSize.x)) == width &&
        static_cast<std::uint32_t>(std::lround(currentSize.y)) == height) {
        return true;
    }

    const render::RenderTargetHandle newTarget = renderer.CreateRenderTarget(width, height);
    if (newTarget.id == 0) {
        return false;
    }
    const render::TextureHandle newTexture = renderer.RenderTargetTexture(newTarget);
    if (newTexture.id == 0) {
        renderer.DestroyRenderTarget(newTarget);
        return false;
    }
    renderer.SetTextureFilter(newTexture, render::TextureFilter::Linear);

    const render::RenderTargetHandle oldTarget = photoSourceTarget;
    photoSourceTarget = newTarget;
    photoSourceTextureHandle = newTexture;
    if (oldTarget.id != 0) {
        renderer.DestroyRenderTarget(oldTarget);
    }
    TraceLog(LOG_INFO, "Camera FSR source target: %ux%u", width, height);
    return true;
}

void CameraRig::hide() {
    visible = false;
    photoAlpha = 0.0f;
}

void CameraRig::reset() {
    visible = true;
    frozen = false;
    displayScale = 1.0f;
    displayX = x;
    displayY = y;
}

bool CameraRig::isOverTV(bool smaller) const {
    const float w = smaller ? 140.0f : kCameraWidth;
    const float h = smaller ? 90.0f : kCameraHeight;
    const float left = kGameWidth / 2.0f - w / 2.0f;
    const float right = kGameWidth / 2.0f + w / 2.0f;
    const float top = kGameHeight / 2.0f - h / 2.0f;
    const float bottom = kGameHeight / 2.0f + h / 2.0f;
    return left < x && x < right && top < y && y < bottom;
}

TV::TV(Gameplay& gameplayRef) : Prop(gameplayRef, "TV") {
    x = kGameWidth / 2.0f;
    y = kGameHeight / 2.0f + 80.0f;
    width = 150.0f;
    height = 180.0f;
}

void TV::update(float dt) {
    Prop::update(dt);
    chyronAlpha = std::min(1.0f, chyronAlpha + dt * 2.0f);
    chyronX = chyronX * 0.9f;
}

void TV::draw(const View& view) const {
    drawRendererTextureAnchored(gameplay.game.renderer(), gameplay.assets().textureHandle("sprites/tv.png"), applyView(view, {x, y}), {0.5f, 1.0f}, {0.5f * view.scale, 0.5f * view.scale}, 0.0f, WHITE, 1.0f);

    const render::Vec2 photoSize = gameplay.game.renderer().TextureSize(photo);
    if (photo.id != 0 && photoSize.x > 0.0f && photoSize.y > 0.0f) {
        constexpr float conversion = 0.5f;
        const Vector2 photoTopWorld{
            x + offset.x - kCameraWidth * 0.5f * conversion,
            y + offset.y - kCameraHeight * 0.5f * conversion
        };
        Vector2 topLeft = applyView(view, photoTopWorld);
        const Rectangle dest{topLeft.x, topLeft.y, kCameraWidth * conversion * view.scale, kCameraHeight * conversion * view.scale};
        render::DrawTextureParams photoParams{};
        photoParams.texture = photo;
        photoParams.destination = toRender(dest);
        photoParams.tint = {255, 255, 255, 255};
        photoParams.flipY = true;
        gameplay.game.renderer().DrawTextureSource(photoParams);
        Vector2 chyronPos = applyView(view, {photoTopWorld.x + chyronX * conversion, photoTopWorld.y});
        drawRendererTextureAnchored(gameplay.game.renderer(), gameplay.assets().textureHandle(chyronTexture), chyronPos, {0.0f, 0.0f}, {0.125f * conversion * view.scale, 0.125f * conversion * view.scale}, 0.0f, WHITE, chyronAlpha);
        if (!chyronText.empty()) {
            const float maxTextWidth = 430.0f * conversion * view.scale;
            const render::FontHandle font64 = gameplay.assets().fontHandle(64);
            const float fontSize = fitRendererFontSize(gameplay.game.renderer(), font64, chyronText, 5.0f * view.scale, maxTextWidth, 3.2f * view.scale);
            Vector2 textPos = applyView(view, {
                photoTopWorld.x + (chyronX + 30.0f) * conversion,
                photoTopWorld.y + 120.0f * conversion
            });
            const render::Vec2 measured = gameplay.game.renderer().MeasureText(font64, chyronText, fontSize);
            textPos.y -= measured.y / 2.0f;
            drawRendererText(gameplay.game.renderer(), font64, chyronText, textPos, fontSize, WHITE, chyronAlpha);
        }
    }
}

void TV::placePhoto(render::TextureHandle texture, const std::string& text, bool fail, bool nothing) {
    photo = texture;
    chyronText = text;
    showNothing = nothing;
    (void)nothing;
    chyronTexture = fail ? "sprites/chyron2.png" : "sprites/chyron.png";
    chyronAlpha = 0.0f;
    chyronX = 15.0f;
}

Director::Director(Gameplay& gameplayRef) : gameplay(gameplayRef) {}

void Director::update(float dt) {
    sequence.update(dt);
    if (viewportTweening) {
        viewportTime += dt;
        const float t = easeCubicInOut(viewportTime / viewportDuration);
        gameplay.worldPivot = {lerp(viewportStartPivot.x, viewportEndPivot.x, t), lerp(viewportStartPivot.y, viewportEndPivot.y, t)};
        gameplay.worldScale = lerp(viewportStartScale, viewportEndScale, t);
        if (viewportTime >= viewportDuration) {
            viewportTweening = false;
        }
    }
}

void Director::takePhoto() {
    photoData = PhotoData{};
    chyron.clear();
    analyzePhoto();
    sequence.clear();
    sequence.after(0.25f, [this]() { movePhoto(); });
    sequence.after(1.25f, [this]() { cutToTV(); });
    sequence.after(2.75f, [this]() { zoomOut1(); });
    sequence.after(6.25f, [this]() { zoomOut2(); });
    sequence.after(8.25f, [this]() { reset(); });
}

std::vector<Prop*> Director::propsInCamera(const std::function<bool(Prop&)>& filter) const {
    std::vector<Prop*> caught;
    for (const auto& prop : gameplay.world->props) {
        if (prop->dead) continue;
        if (cameraContains(*prop) && (!filter || filter(*prop))) {
            caught.push_back(prop.get());
        }
    }
    return caught;
}

Prop* Director::firstInCamera(const std::function<bool(Prop&)>& filter) const {
    auto caught = propsInCamera(filter);
    return caught.empty() ? nullptr : caught.front();
}

bool Director::cameraContains(Prop& prop) const {
    const CameraRig& cam = *gameplay.camera;
    const float cl = cam.x - kCameraWidth / 2.0f;
    const float cr = cam.x + kCameraWidth / 2.0f;
    const float ct = cam.y - kCameraHeight / 2.0f;
    const float cb = cam.y + kCameraHeight / 2.0f;
    const Rectangle bounds = prop.cameraBounds();
    if (bounds.width <= 0.0f || bounds.height <= 0.0f) return false;
    const float pl = bounds.x;
    const float pr = bounds.x + bounds.width;
    const float pt = bounds.y;
    const float pb = bounds.y + bounds.height;

    if (pr < cl || pl > cr || pb < ct || pt > cb) return false;
    const float l = std::max(cl, pl);
    const float r = std::min(cr, pr);
    const float t = std::max(ct, pt);
    const float b = std::min(cb, pb);
    const float overlap = std::max(0.0f, r - l) * std::max(0.0f, b - t);
    const float total = std::max(1.0f, bounds.width * bounds.height);

    if (dynamic_cast<Peep*>(&prop)) {
        const float storyBottom = ct + kCameraHeight * 0.66f;
        const float storyT = t;
        const float storyB = std::min(b, storyBottom);
        float visibleOverlap = std::max(0.0f, r - l) * std::max(0.0f, storyB - storyT);
        const bool tvDrawsOverPeep = gameplay.tv && gameplay.tv != &prop && !gameplay.tv->dead && (prop.y + prop.z) < (gameplay.tv->y + gameplay.tv->z);
        Rectangle tvBounds{};
        if (tvDrawsOverPeep) {
            tvBounds = gameplay.tv->cameraBounds();
            const float hiddenL = std::max(l, tvBounds.x);
            const float hiddenR = std::min(r, tvBounds.x + tvBounds.width);
            const float hiddenT = std::max(storyT, tvBounds.y);
            const float hiddenB = std::min(storyB, tvBounds.y + tvBounds.height);
            visibleOverlap -= std::max(0.0f, hiddenR - hiddenL) * std::max(0.0f, hiddenB - hiddenT);
        }

        if (std::max(0.0f, visibleOverlap) / total <= 0.55f) return false;

        const float focusX = bounds.x + bounds.width * 0.5f;
        const float focusY = bounds.y + bounds.height * 0.45f;
        if (tvDrawsOverPeep && CheckCollisionPointRec({focusX, focusY}, tvBounds)) return false;
        return cl < focusX && focusX < cr && ct < focusY && focusY < storyBottom;
    }

    return overlap / total > 0.33f;
}

void Director::movePhoto() {
    CameraRig& cam = *gameplay.camera;
    cam.displayX = kGameWidth / 2.0f;
    cam.displayY = kGameHeight / 2.0f;
    cam.displayScale = kGameWidth / kCameraWidth;
    audienceMovePhoto();
}

void Director::cutToTV() {
    isWatchingTV = true;
    gameplay.camera->hide();
    if (!gameplay.camera->hasCapturedPhoto() && !warnedMissingPhoto) {
        TraceLog(LOG_WARNING, "TV cut started without a valid captured camera texture");
        warnedMissingPhoto = true;
    }
    bool fail = false;
    if (!photoData.forceChyron && photoData.audience == 0 && photoData.audienceCircles == 0 && photoData.audienceSquares == 0) {
        if (!noSounds) gameplay.audio().playSound("sounds/crickets.mp3");
        fail = true;
        auto& cricket = gameplay.world->addProp<Cricket>(gameplay);
        cricket.watchTV();
        if (photoData.caughtCricket) {
            auto& cricket2 = gameplay.world->addProp<Cricket>(gameplay);
            cricket2.watchTV();
            cricket2.x += 30.0f;
            cricket2.hopAwayTicks += 15;
            auto& cricket3 = gameplay.world->addProp<Cricket>(gameplay);
            cricket3.watchTV();
            cricket3.x += 60.0f;
            cricket3.hopAwayTicks += 30;
        }
    }

    render::TextureHandle photoTexture = gameplay.camera->hasCapturedPhoto() ? gameplay.camera->photoTexture() : render::TextureHandle{};
    gameplay.tv->placePhoto(photoTexture, chyron, fail, photoData.itsNothing);
    cutViewportTo({gameplay.tv->x + gameplay.tv->offset.x, gameplay.tv->y + gameplay.tv->offset.y}, gameplay.tv->offsetScale);

    for (Peep* peep : gameplay.world->peeps) {
        if (peep && peep->className == "NormalPeep") peep->getOuttaTV();
    }
}

void Director::zoomOut1() {
    const float x = (gameplay.tv->x * 3.0f + kGameWidth / 2.0f) / 4.0f;
    const float y = gameplay.tv->y + gameplay.tv->offset.y * 3.0f / 4.0f;
    tweenViewportTo({x, y}, 2.5f, 2.0f);
    cutStage();
}

void Director::zoomOut2() {
    isWatchingTV = false;
    tweenViewportTo({kGameWidth / 2.0f, kGameHeight / 2.0f}, 1.0f, 2.0f);
}

void Director::reset() {
    gameplay.camera->reset();
    cutViewportTo({kGameWidth / 2.0f, kGameHeight / 2.0f}, 1.0f);
}

void Director::audienceMovePhoto() {
    if (noSounds) return;
    if (photoData.audience > 0 || photoData.audienceCircles > 0 || photoData.audienceSquares > 0 || photoData.forceChyron) {
        gameplay.audio().playSound("sounds/breaking_news.mp3");
    }
}

bool Director::audienceCutToTV(const std::function<void(NormalPeep&)>& action, const std::function<bool(NormalPeep&)>& filter) {
    int circlesLeft = photoData.audience;
    int squaresLeft = photoData.audience;
    if (photoData.hijack) {
        circlesLeft = photoData.audienceCircles;
        squaresLeft = photoData.audienceSquares;
        photoData.audience = circlesLeft + squaresLeft;
    } else {
        photoData.audienceCircles = photoData.audience;
        photoData.audienceSquares = photoData.audience;
    }

    for (Peep* peep : gameplay.world->peeps) {
        if (peep && peep->className == "NormalPeep") peep->getOuttaTV();
    }

    if (photoData.audience <= 0) {
        gameplay.world->sortForDepth();
        return false;
    }

    std::vector<NormalPeep*> candidates;
    for (Peep* peep : gameplay.world->peeps) {
        auto* normal = dynamic_cast<NormalPeep*>(peep);
        if (!normal || normal->dead) continue;
        if (filter && !filter(*normal)) continue;
        candidates.push_back(normal);
    }
    std::shuffle(candidates.begin(), candidates.end(), gameplay.rng);

    for (NormalPeep* peep : candidates) {
        bool watch = false;
        float flip = 1.0f;
        float offset = 60.0f;
        if (peep->type == "circle" && circlesLeft > 0) {
            flip = 1.0f;
            offset = 60.0f + static_cast<float>(photoData.audienceCircles - circlesLeft) * 40.0f;
            --circlesLeft;
            watch = true;
        }
        if (peep->type == "square" && squaresLeft > 0) {
            flip = -1.0f;
            offset = 60.0f + static_cast<float>(photoData.audienceSquares - squaresLeft) * 40.0f;
            --squaresLeft;
            watch = true;
        }
        if (watch) {
            peep->x = gameplay.tv->x - flip * offset;
            peep->y = gameplay.tv->y + gameplay.random();
            if (action) action(*peep);
            else peep->watchTV();
        }
    }
    gameplay.world->sortForDepth();
    return true;
}

void Director::cutViewportTo(Vector2 pivot, float scale) {
    gameplay.worldPivot = pivot;
    gameplay.worldScale = scale;
}

void Director::tweenViewportTo(Vector2 pivot, float scale, float duration) {
    viewportTweening = true;
    viewportTime = 0.0f;
    viewportDuration = duration;
    viewportStartPivot = gameplay.worldPivot;
    viewportEndPivot = pivot;
    viewportStartScale = gameplay.worldScale;
    viewportEndScale = scale;
}

void Director::chyLovers() {
    Prop* lover = firstInCamera([](Prop& prop) { return prop.className == "LoverPeep"; });
    if (auto* lp = dynamic_cast<LoverPeep*>(lover)) {
        if (lp->isEmbarrassed) {
            chyron = gameplay.tr(txt::outtaHere);
        } else {
            photoData.caughtLovers = true;
            photoData.forceChyron = true;
            chyron = gameplay.tr(txt::getARoom);
        }
    }
}

bool Director::chyHats() {
    Prop* hat = firstInCamera([](Prop& prop) {
        auto* peep = dynamic_cast<NormalPeep*>(&prop);
        return peep && peep->wearingHat;
    });
    if (hat) {
        photoData.audience = 1;
        photoData.caughtHat = true;
        chyron = gameplay.tr(txt::notCoolAnymore);
        return true;
    }
    return false;
}

void Director::chyPeeps() {
    if (gameplay.camera->isOverTV(true)) {
        chyron = gameplay.tr(txt::tvOnTv);
        return;
    }
    auto crickets = propsInCamera([](Prop& prop) { return prop.className == "Cricket"; });
    auto peeps = propsInCamera([](Prop& prop) { return prop.className == "NormalPeep"; });
    if (!crickets.empty()) {
        photoData.caughtCricket = true;
        chyron = gameplay.tr(crickets.size() == 1 ? txt::cricky : txt::tooManyCrickets);
    } else if (!peeps.empty()) {
        chyron = gameplay.tr(peeps.size() == 1 ? txt::normalPeep : txt::normalPeeps);
    } else {
        photoData.itsNothing = true;
        chyron = gameplay.tr(txt::wowNothing);
    }
}

std::string Director::spoutManifesto() {
    manifestoIndex = std::min(manifestoIndex + 1, static_cast<int>(std::size(txt::manifesto)) - 1);
    return gameplay.tr(txt::manifesto[static_cast<std::size_t>(manifestoIndex)]);
}

void Director::analyzePhoto() {
    switch (stage) {
        case Stage::Hat: {
            Prop* hat = firstInCamera([](Prop& prop) { return prop.className == "HatPeep"; });
            if (hat) {
                photoData.audience = 3;
                photoData.caughtHat = true;
                chyron = gameplay.tr(txt::niceHat);
            } else {
                chyPeeps();
            }
            break;
        }
        case Stage::Lovers:
            chyLovers();
            if (!photoData.caughtLovers && !chyHats()) chyPeeps();
            break;
        case Stage::Screamer: {
            auto* crazy = dynamic_cast<CrazyPeep*>(firstInCamera([](Prop& prop) { return prop.className == "CrazyPeep"; }));
            if (crazy) {
                if (crazy->isScreaming()) {
                    photoData.hijack = true;
                    photoData.audienceCircles = 1;
                    photoData.caughtCrazy = true;
                    chyron = gameplay.tr(txt::crazySquareAttacks);
                } else {
                    chyron = firstInCamera([](Prop& prop) {
                        auto* peep = dynamic_cast<NormalPeep*>(&prop);
                        return peep && peep->shocked;
                    }) ? gameplay.tr(txt::justMissed) : gameplay.tr(txt::somethingInteresting);
                }
            } else {
                chyLovers();
                if (!photoData.caughtLovers && !chyHats()) {
                    if (firstInCamera([](Prop& prop) {
                            auto* peep = dynamic_cast<NormalPeep*>(&prop);
                            return peep && peep->shocked;
                        })) chyron = gameplay.tr(txt::whoIsScreaming);
                    else chyPeeps();
                }
            }
            break;
        }
        case Stage::Nervous: {
            auto* nervous = dynamic_cast<NervousPeep*>(firstInCamera([](Prop& prop) { return prop.className == "NervousPeep"; }));
            if (nervous) {
                if (nervous->isScared() && firstInCamera([](Prop& prop) {
                        auto* peep = dynamic_cast<NormalPeep*>(&prop);
                        return peep && peep->confused;
                    })) {
                    photoData.hijack = true;
                    photoData.audienceSquares = 1;
                    photoData.caughtNervous = true;
                    chyron = gameplay.tr(txt::circleFearsSquares);
                } else {
                    chyron = gameplay.tr(nervous->isScared() ? txt::whoScaresThem : txt::somethingInteresting);
                }
            } else if (!chyHats()) {
                chyPeeps();
            }
            break;
        }
        case Stage::Snobby: {
            auto* snobby = dynamic_cast<SnobbyPeep*>(firstInCamera([](Prop& prop) { return prop.className == "SnobbyPeep"; }));
            if (snobby) {
                if (snobby->isSmug) {
                    photoData.hijack = true;
                    photoData.audienceCircles = 1;
                    photoData.caughtSnobby = true;
                    chyron = gameplay.tr(txt::squaresSnubCircles);
                } else {
                    chyron = gameplay.tr(txt::somethingInteresting);
                }
            } else if (!chyHats()) {
                chyPeeps();
            }
            break;
        }
        case Stage::Angry: {
            if (firstInCamera([](Prop& prop) { return prop.className == "HelpingAnim"; })) {
                auto* helping = dynamic_cast<HelpingAnim*>(firstInCamera([](Prop& prop) { return prop.className == "HelpingAnim"; }));
                chyron = helping && helping->hasHelped ? spoutManifesto() : gameplay.tr(txt::nerdsNow);
            } else if (firstInCamera([](Prop& prop) { return prop.className == "ProtestAnim" || prop.className == "HappyWeirdoPeep" || prop.className == "LoverPeep"; })) {
                chyron = spoutManifesto();
            } else {
                auto angry = propsInCamera([](Prop& prop) { return prop.className == "AngryPeep"; });
                if (!angry.empty()) {
                    auto shouting = propsInCamera([](Prop& prop) {
                        auto* angryPeep = dynamic_cast<AngryPeep*>(&prop);
                        return angryPeep && angryPeep->isShouting;
                    });
                    if (!shouting.empty()) {
                        photoData.audience = 2;
                        photoData.caughtAngry = true;
                        int angryCount = 0;
                        for (Peep* peep : gameplay.world->peeps) {
                            if (peep && peep->className == "AngryPeep") ++angryCount;
                        }
                        const float ratio = static_cast<float>(angryCount + 4) / std::max(1.0f, static_cast<float>(gameplay.world->peeps.size() - 1));
                        if (ratio >= 1.0f) chyron = gameplay.tr(txt::everyoneHates);
                        else if (ratio >= 0.75f) chyron = gameplay.tr(txt::almostEveryoneHates);
                        else {
                            bool circle = false;
                            bool square = false;
                            for (Prop* prop : shouting) {
                                auto* angryPeep = dynamic_cast<AngryPeep*>(prop);
                                if (angryPeep->type == "circle") circle = true;
                                if (angryPeep->type == "square") square = true;
                            }
                            photoData.caughtAngryCircle = circle;
                            photoData.caughtAngrySquare = square;
                            chyron = gameplay.tr(circle ? txt::circlesHateSquares : txt::squaresHateCircles);
                        }
                    } else {
                        chyron = gameplay.tr(txt::areTheyYelling);
                    }
                } else if (firstInCamera([](Prop& prop) {
                               auto* peep = dynamic_cast<NormalPeep*>(&prop);
                               return peep && peep->shocked;
                           })) {
                    chyron = gameplay.tr(txt::shockedPeep);
                } else {
                    chyPeeps();
                }
            }
            break;
        }
        case Stage::Evil: {
            auto* evil = dynamic_cast<EvilHatPeep*>(firstInCamera([](Prop& prop) { return prop.className == "EvilHatPeep"; }));
            if (evil && evil->hasGunOut) chyron = gameplay.tr(txt::ellipsis);
            else if (evil) chyron = gameplay.tr(txt::coolNoMore);
            else chyron = spoutManifesto();
            break;
        }
        case Stage::Panic:
            chyron = gameplay.tr(txt::beScared);
            photoData.forceChyron = true;
            break;
    }
}

void Director::cutStage() {
    switch (stage) {
        case Stage::Hat:
            if (photoData.caughtHat) {
                gameplay.world->addBalancedPeeps(1);
                audienceCutToTV([](NormalPeep& peep) { peep.wearHat(); });
                for (auto& prop : gameplay.world->props) {
                    if (prop->className == "HatPeep") prop->kill();
                }
                gameplay.stageLovers();
            } else {
                audienceCutToTV();
            }
            break;
        case Stage::Lovers:
            if (photoData.caughtLovers) {
                audienceCutToTV();
                for (Peep* peep : gameplay.world->peeps) {
                    if (auto* lover = dynamic_cast<LoverPeep*>(peep)) lover->makeEmbarrassed();
                }
            } else if (photoData.caughtHat) {
                audienceCutToTV([](NormalPeep& peep) { peep.takeOffHat(false); }, [](NormalPeep& peep) { return peep.wearingHat; });
            } else {
                audienceCutToTV();
            }
            gameplay.stageScreamer();
            break;
        case Stage::Screamer:
            if (photoData.caughtCrazy) {
                gameplay.world->addBalancedPeeps(1);
                audienceCutToTV();
                for (auto& prop : gameplay.world->props) if (prop->className == "CrazyPeep") prop->kill();
                gameplay.world->replaceWatcher("circle", std::make_unique<NervousPeep>(gameplay));
                gameplay.stageNervous();
            } else {
                audienceCutToTV();
            }
            break;
        case Stage::Nervous:
            if (photoData.caughtNervous) {
                gameplay.world->addBalancedPeeps(1);
                audienceCutToTV();
                for (auto& prop : gameplay.world->props) if (prop->className == "NervousPeep") prop->kill();
                gameplay.world->replaceWatcher("square", std::make_unique<SnobbyPeep>(gameplay));
                gameplay.stageSnobby();
            } else {
                audienceCutToTV();
            }
            break;
        case Stage::Snobby:
            if (photoData.caughtSnobby) {
                for (Peep* peep : gameplay.world->peeps) {
                    if (auto* normal = dynamic_cast<NormalPeep*>(peep)) {
                        if (normal->wearingHat) normal->takeOffHat(true);
                    }
                }
                audienceCutToTV();
                for (auto& prop : gameplay.world->props) if (prop->className == "SnobbyPeep") prop->kill();
                gameplay.world->replaceWatcher("circle", std::make_unique<AngryPeep>(gameplay, "circle"));
                gameplay.stageAngry();
            } else {
                audienceCutToTV();
            }
            break;
        case Stage::Angry: {
            if (photoData.caughtAngry) {
                audienceCutToTV();
                std::vector<NormalPeep*> watchers;
                for (Peep* peep : gameplay.world->peeps) {
                    auto* normal = dynamic_cast<NormalPeep*>(peep);
                    if (normal && normal->isWatching) watchers.push_back(normal);
                }
                for (NormalPeep* normal : watchers) {
                    auto replacement = std::make_unique<AngryPeep>(gameplay, normal->type);
                    AngryPeep& angry = static_cast<AngryPeep&>(gameplay.world->replacePeep(*normal, std::move(replacement)));
                    angry.watchTV();
                }
            } else {
                audienceCutToTV();
            }

            int angryCount = 0;
            for (Peep* peep : gameplay.world->peeps) if (peep && peep->className == "AngryPeep") ++angryCount;
            const float ratio = static_cast<float>(angryCount) / std::max(1.0f, static_cast<float>(gameplay.world->peeps.size() - 1));
            if (ratio > 0.5f && !firstInCamera([](Prop& prop) { return prop.className == "ProtestAnim"; })) {
                bool exists = false;
                for (auto& prop : gameplay.world->props) if (prop->className == "ProtestAnim") exists = true;
                if (!exists) gameplay.world->addProp<ProtestAnim>(gameplay);
            }
            if (ratio >= 1.0f) {
                gameplay.stageEvil();
            }
            break;
        }
        case Stage::Evil:
            audienceCutToTV();
            break;
        case Stage::Panic: {
            audienceCutToTV();
            bool removedOriginalMurderer = false;
            bool removedPreviousMurderers = false;
            for (auto& prop : gameplay.world->props) {
                if (prop->className == "EvilHatPeep") {
                    prop->kill();
                    removedOriginalMurderer = true;
                } else if (prop->className == "MurderPeep") {
                    prop->kill();
                    removedPreviousMurderers = true;
                }
            }
            if (removedOriginalMurderer) {
                gameplay.avoidSpots.clear();
            }
            if (removedPreviousMurderers && !gameplay.zoomer->started) {
                gameplay.zoomer->init();
            }

            const std::vector<std::string> weapons = {"gun", "bat", "shotgun", "axe"};
            auto& murderer1 = gameplay.world->addPeep<MurderPeep>(gameplay);
            murderer1.init("circle", weapons[static_cast<std::size_t>(gameplay.panicWeaponIndex)]);
            auto& murderer2 = gameplay.world->addPeep<MurderPeep>(gameplay);
            murderer2.init("square", weapons[static_cast<std::size_t>((gameplay.panicWeaponIndex + 1) % weapons.size())]);
            gameplay.panicWeaponIndex = (gameplay.panicWeaponIndex + 1) % static_cast<int>(weapons.size());
            break;
        }
    }
}

void ScreenShake::shake(float value) {
    intensity = value;
    snowAlpha = baseAlpha + 0.35f;
}

void ScreenShake::update(float) {
    if (snowAlpha > 0.0f) {
        snowAlpha = snowAlpha * 0.95f + baseAlpha * 0.05f;
    }

    if (intensity <= 0.5f) {
        gameplay.offX = 0.0f;
        gameplay.offY = 0.0f;
        intensity = 0.0f;
    } else {
        gameplay.offX = gameplay.sceneScale * gameplay.random(-1.0f, 1.0f) * intensity;
        gameplay.offY = gameplay.sceneScale * gameplay.random(-1.0f, 1.0f) * intensity;
        intensity *= 0.95f;
    }
}

void ScreenShake::draw(const View&) const {
    if (snowAlpha <= 0.01f) return;
    float scaleX = 1.0f + gameplay.random(0.0f, 0.2f);
    float scaleY = 1.0f + gameplay.random(0.0f, 0.2f);
    if (gameplay.randomChance(0.5f)) scaleX *= -1.0f;
    if (gameplay.randomChance(0.5f)) scaleY *= -1.0f;
    const Vector2 scenePosition{kGameWidth / 2.0f - gameplay.offX, kGameHeight / 2.0f - gameplay.offY};
    const Vector2 screenPosition{
        gameplay.sceneX + gameplay.offX + scenePosition.x * gameplay.sceneScale,
        gameplay.sceneY + gameplay.offY + scenePosition.y * gameplay.sceneScale
    };
    drawRendererTextureAnchored(
        gameplay.game.renderer(),
        gameplay.assets().textureHandle("sprites/snow.png"),
        screenPosition,
        {0.5f, 0.5f},
        {scaleX * gameplay.sceneScale, scaleY * gameplay.sceneScale},
        0.0f,
        WHITE,
        snowAlpha
    );
}

void ScreenZoomOut::init() {
    if (started) return;
    started = true;
}

void ScreenZoomOut::update(float dt) {
    if (completed || !started) return;
    scale *= std::pow(0.9996f, dt * kFrameRate);
    gameplay.sceneScale = scale;
    gameplay.sceneX = (kGameWidth - kGameWidth * scale) / 2.0f;
    gameplay.sceneY = (kGameHeight - kGameHeight * scale) / 2.0f;
    timer -= dt;
    if (timer <= 0.0f) {
        completed = true;
        if (onComplete) onComplete();
    }
}

Gameplay::Gameplay(Game& gameRef)
    : game(gameRef) {
    world = std::make_unique<World>(*this);
    camera = std::make_unique<CameraRig>(*this);
    director = std::make_unique<Director>(*this);
    shaker = std::make_unique<ScreenShake>(*this);
    zoomer = std::make_unique<ScreenZoomOut>(*this);
    tv = &world->addProp<TV>(*this);
}

Gameplay::~Gameplay() = default;

void Gameplay::enter() {
    blackoutAlpha = 1.0f;
    stageStart();
    stageHat();
}

void Gameplay::exit() {
    audio().stopAllMusic();
}

void Gameplay::update(float dt) {
    if (game.logicalMousePressed() && !camera->frozen) {
        camera->takePhoto();
    }

    world->update(dt);
    camera->update(dt);
    director->update(dt);
    zoomer->update(dt);
    shaker->update(dt);

    const float ratio = zoomer->timer / zoomer->fullTimer;
    shaker->baseAlpha = 0.15f + (1.0f - ratio) * 0.45f;
    blackoutAlpha = std::max(0.0f, blackoutAlpha - dt);
}

void Gameplay::draw() {
    View view;
    view.pivot = worldPivot;
    view.center = {kGameWidth / 2.0f * sceneScale, kGameHeight / 2.0f * sceneScale};
    view.scale = worldScale * sceneScale;
    view.sceneOffset = {sceneX + offX, sceneY + offY};

    world->draw(view);
    camera->draw(view);
    shaker->draw(view);
    if (zoomer->started) {
        constexpr float laptopOffsetX = 816.0f;
        constexpr float laptopOffsetY = 459.0f;
        drawRendererTextureAnchored(
            game.renderer(),
            assets().textureHandle("sprites/laptop.png"),
            {sceneX - laptopOffsetX * sceneScale, sceneY - laptopOffsetY * sceneScale},
            {0.0f, 0.0f},
            {sceneScale, sceneScale},
            0.0f,
            WHITE,
            1.0f
        );
    }
    if (blackoutAlpha > 0.0f) {
        game.renderer().DrawRectangle({0.0f, 0.0f, static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)}, toRender(BLACK), blackoutAlpha);
    }
}

void Gameplay::renderWorldToPhoto(render::RenderTargetHandle target,
                                  const CameraRig& cam) {
    render::Renderer& renderer = game.renderer();
    renderer.BeginRenderTarget(target);
    const render::TextureHandle targetTexture = renderer.RenderTargetTexture(target);
    const render::Vec2 textureSize = renderer.TextureSize(targetTexture);
    const float targetWidth = std::max(1.0f, textureSize.x);
    const float targetHeight = std::max(1.0f, textureSize.y);
    renderer.DrawRectangle({0.0f, 0.0f, targetWidth, targetHeight}, toRender(BLACK));
    const float captureScale = std::min(targetWidth / kCameraWidth, targetHeight / kCameraHeight);
    View photoView;
    photoView.pivot = {cam.x, cam.y};
    photoView.center = {targetWidth / 2.0f, targetHeight / 2.0f};
    photoView.scale = std::max(0.001f, captureScale);
    photoView.sceneOffset = {0.0f, 0.0f};
    world->drawToPhoto(photoView);
    renderer.EndRenderTarget();
}

void Gameplay::stageStart() {
    world->clearPeeps();
    world->addBalancedPeeps(20);
}

void Gameplay::stageHat() {
    world->addPeep<HatPeep>(*this);
    director->stage = Stage::Hat;
}

void Gameplay::stageLovers() {
    auto& lover1 = world->addPeep<LoverPeep>(*this);
    lover1.setType("circle");
    auto& lover2 = world->addPeep<LoverPeep>(*this);
    lover2.setType("square");
    lover2.follow(lover1);
    director->stage = Stage::Lovers;
}

void Gameplay::stageScreamer() {
    bool exists = false;
    for (auto& prop : world->props) if (prop->className == "CrazyPeep") exists = true;
    if (!exists) world->addPeep<CrazyPeep>(*this);
    director->stage = Stage::Screamer;
}

void Gameplay::stageNervous(bool hack) {
    for (Peep* peep : std::vector<Peep*>(world->peeps)) {
        if (peep && peep->className == "LoverPeep") peep->kill();
    }
    if (hack) {
        auto& nervous = world->addPeep<NervousPeep>(*this);
        nervous.jumpStart();
    }
    director->stage = Stage::Nervous;
}

void Gameplay::stageSnobby(bool hack) {
    if (hack) {
        auto& snobby = world->addPeep<SnobbyPeep>(*this);
        snobby.jumpStart();
    }
    director->stage = Stage::Snobby;
}

void Gameplay::stageAngry(bool hack) {
    if (hack) {
        auto& angry = world->addPeep<AngryPeep>(*this, "circle");
        angry.jumpStart();
    }
    bool helpingExists = false;
    for (auto& prop : world->props) if (prop->className == "HelpingAnim") helpingExists = true;
    if (!helpingExists) world->addProp<HelpingAnim>(*this);
    director->stage = Stage::Angry;
}

void Gameplay::stageEvil(bool hack) {
    if (hack) {
        world->addPeep<HappyWeirdoPeep>(*this);
        world->addProp<ProtestAnim>(*this);
    }

    HappyWeirdoPeep* happy = nullptr;
    for (Peep* peep : world->peeps) {
        if (auto* h = dynamic_cast<HappyWeirdoPeep*>(peep)) {
            happy = h;
            break;
        }
    }
    if (!happy) {
        happy = &world->addPeep<HappyWeirdoPeep>(*this);
    }
    happy->prepareForMurder();

    auto freezeEveryone = [this]() {
        std::vector<Peep*> snapshot = world->peeps;
        for (Peep* old : snapshot) {
            if (!old || old->dead) continue;
            if (old->className == "NormalPeep" || old->className == "AngryPeep") {
                auto stunned = std::make_unique<NormalPeep>(*this);
                stunned->setType(old->type.empty() ? "circle" : old->type);
                stunned->vel = old->vel;
                stunned->flip = old->x < kGameWidth / 2.0f ? 1.0f : -1.0f;
                stunned->beStunned();
                world->replacePeep(*old, std::move(stunned));
            }
        }
        for (auto& prop : world->props) {
            if (auto* protest = dynamic_cast<ProtestAnim*>(prop.get())) protest->beStunned();
        }
        audio().stopMusic("sounds/bg_park.mp3");
    };

    auto& murderer = world->addPeep<EvilHatPeep>(*this);
    murderer.victim = happy;
    murderer.freezeEveryone = freezeEveryone;
    murderer.bang = [this]() { stagePanic(); };
    director->stage = Stage::Evil;
}

void Gameplay::stagePanic() {
    panicWeaponIndex = 0;
    audio().playMusic("sounds/bg_panic.mp3", 0.75f, true);
    audio().playMusic("sounds/bg_creepy.mp3", 0.0f, true);
    audio().fadeMusic("sounds/bg_creepy.mp3", 0.0f, 1.0f, 5.0f);
    camera->noSounds = true;
    director->noSounds = true;
    shaker->shake(100.0f);
    avoidSpots.clear();
    avoidSpots.push_back({530.0f, 430.0f, 150.0f});

    std::vector<Peep*> snapshot = world->peeps;
    for (Peep* old : snapshot) {
        if (!old || old->dead) continue;
        if (old->className == "NormalPeep") {
            auto panic = std::make_unique<PanicPeep>(*this);
            panic->setType(old->type);
            world->replacePeep(*old, std::move(panic));
        }
    }

    for (auto& prop : world->props) {
        if (prop->className == "ProtestAnim") prop->kill();
    }
    auto& panicCircle = world->addPeep<PanicPeep>(*this);
    panicCircle.setLover("circle");
    auto& panicSquare = world->addPeep<PanicPeep>(*this);
    panicSquare.setLover("square");

    HappyWeirdoPeep* happy = nullptr;
    for (Peep* peep : world->peeps) {
        if (auto* h = dynamic_cast<HappyWeirdoPeep*>(peep)) happy = h;
    }
    if (happy) {
        auto& body = world->addProp<DeadBody>(*this);
        body.init(-1.4f, 3.5f, happy->x, happy->y, -1.0f, 0);
        for (int i = 0; i < 30; ++i) {
            auto& gore = world->addProp<Gore>(*this);
            gore.init(-kTau / 4.0f + random(0.0f, 1.0f), 10.0f + random(0.0f, 5.0f), happy->x, happy->y, random(-30.0f, 0.0f));
        }
        happy->kill();
    }

    zoomer->onComplete = [this]() {
        audio().stopMusic("sounds/bg_creepy.mp3");
        audio().stopMusic("sounds/bg_panic.mp3");
        game.requestScene(SceneId::Credits);
    };
    director->stage = Stage::Panic;
}

}  // namespace

class GameplayScene::Impl {
public:
    explicit Impl(Game& game) : gameplay(std::make_unique<Gameplay>(game)) {}
    std::unique_ptr<Gameplay> gameplay;
};

GameplayScene::GameplayScene(Game& game) : Scene(game), impl_(new Impl(game)) {}

GameplayScene::~GameplayScene() {
    delete impl_;
}

void GameplayScene::enter() {
    impl_->gameplay->enter();
}

void GameplayScene::exit() {
    impl_->gameplay->exit();
}

void GameplayScene::update(float dt) {
    impl_->gameplay->update(dt);
}

void GameplayScene::draw() {
    impl_->gameplay->draw();
}

}  // namespace wb
