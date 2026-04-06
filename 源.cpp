#include "raylib.h"
#include "GameManager.h"
#include <cstdlib>
#include <ctime>

int main() {
    srand((unsigned int)time(nullptr));
    InitWindow(800, 600, "Brick Breaker Game");
    SetExitKey(KEY_NULL);
    GameManager game(800, 600);
    game.Run();
    CloseWindow();
    return 0;
}
