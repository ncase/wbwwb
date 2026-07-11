#include "AssetManager.h"
#include "Game.h"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const char* executablePath = argc > 0 ? argv[0] : ".";
        wb::Game game(
            wb::AssetManager::findAssetRoot(executablePath),
            std::filesystem::absolute(executablePath).parent_path()
        );
        game.run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
