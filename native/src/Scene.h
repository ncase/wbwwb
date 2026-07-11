#pragma once

namespace wb {

class Game;

class Scene {
public:
    explicit Scene(Game& game) : game_(game) {}
    virtual ~Scene() = default;

    virtual void enter() {}
    virtual void exit() {}
    virtual void update(float dt) = 0;
    virtual void draw() = 0;

protected:
    Game& game_;
};

}  // namespace wb
