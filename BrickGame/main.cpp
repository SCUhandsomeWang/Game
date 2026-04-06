#include "raylib.h"
#include "GameManager.h"
#include <cstdlib>
#include <ctime>

int main() {
    srand((unsigned int)time(nullptr));
    
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Brick Breaker Game");
    SetExitKey(KEY_NULL);
    
    GameManager game(screenWidth, screenHeight);
    game.Run();
    
    CloseWindow();
    return 0;
}
