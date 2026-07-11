#pragma once

#include "Settings.h"
#include "render/Renderer.h"

namespace wb {

class AssetManager;
class Localization;

class FramePipeline {
public:
    virtual ~FramePipeline() = default;

    virtual void UpdateViewport(const GameSettings& settings) = 0;
    virtual void BeginLogicalDraw(const GameSettings& settings) = 0;
    virtual void EndLogicalDraw(const GameSettings& settings,
                                AssetManager* assets,
                                const Localization* localization,
                                float postTime) = 0;

    virtual render::Rect Viewport() const = 0;
    virtual render::Rect LogicalViewport() const = 0;
    virtual float LogicalScale() const = 0;
};

}  // namespace wb
