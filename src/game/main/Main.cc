#include "main/Game.h"

int main(int argc, char** argv)
{
    auto game = game::Game::CreateInstance(argc, argv);
    game.Run();
    return 0;
}
