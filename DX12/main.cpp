#include "Core/Engine.h"
#include "Core/Game.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    Engine::Init(hInstance, lpCmdLine);
    std::shared_ptr<Game> game = std::make_shared<Game>();
    
    Engine::Get().RegisterStartupEventHandler(game);
    Engine::Get().RegisterUpdateEventHandler(game);
    Engine::Get().RegisterRenderEventHandler(game);
    
    Engine::Get().Run();
}