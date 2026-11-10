#include "DinamoGame.hpp"

#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    bool startFullscreen = false;
    bool enableTilt = false;
    std::string sharedDirArg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--fullscreen" || arg == "-f") {
            startFullscreen = true;
        } else if (arg == "--tilt" || arg == "-t") {
            enableTilt = true;
        } else if (arg == "--shared-dir" && i + 1 < argc) {
            sharedDirArg = argv[++i];
        }
    }

    SDL_SetMainReady();
    Uint32 initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR;
    if (SDL_Init(initFlags) != 0) {
        initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER;
        if (SDL_Init(initFlags) != 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
            return 1;
        }
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // Crisp nearest-neighbor pixel art

    // Default 1280x800 matches Steam Deck native display (5x integer scale of 160x144 = 800x720)
    Uint32 winFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (startFullscreen) {
        winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Dinamo — Game Boy / Steam Deck Edition",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        800,
        winFlags
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    dinamo::DinamoGame game;
    game.init(renderer, sharedDirArg);
    game.setTiltSteeringEnabled(enableTilt);

    SDL_RenderSetLogicalSize(renderer, game.config().nativeWidth, game.config().nativeHeight);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    auto enableControllerSensors = [](SDL_GameController* ctrl) {
        if (!ctrl) return;
        if (SDL_GameControllerHasSensor(ctrl, SDL_SENSOR_GYRO)) {
            SDL_GameControllerSetSensorEnabled(ctrl, SDL_SENSOR_GYRO, SDL_TRUE);
        }
        if (SDL_GameControllerHasSensor(ctrl, SDL_SENSOR_ACCEL)) {
            SDL_GameControllerSetSensorEnabled(ctrl, SDL_SENSOR_ACCEL, SDL_TRUE);
        }
    };

    SDL_GameController* controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                enableControllerSensors(controller);
                break;
            }
        }
    }

    bool running = true;
    bool fullscreen = startFullscreen;
    Uint64 lastTick = SDL_GetPerformanceCounter();
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_CONTROLLERDEVICEADDED && !controller) {
                controller = SDL_GameControllerOpen(ev.cdevice.which);
                enableControllerSensors(controller);
            } else if (ev.type == SDL_CONTROLLERDEVICEREMOVED && controller) {
                SDL_Joystick* joy = SDL_GameControllerGetJoystick(controller);
                if (joy && SDL_JoystickInstanceID(joy) == ev.cdevice.which) {
                    SDL_GameControllerClose(controller);
                    controller = nullptr;
                }
            } else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_TAB:
                    case SDLK_p:
                        game.cycleTheme();
                        break;
                    case SDLK_g:
                        game.setTiltSteeringEnabled(!game.isTiltSteeringEnabled());
                        break;
                    case SDLK_r:
                        game.startNewGame();
                        break;
                    case SDLK_F11:
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        break;
                    case SDLK_SPACE:
                    case SDLK_z:
                        game.fireOnce();
                        break;
                    default:
                        break;
                }
            } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (ev.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_Y:
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                        game.cycleTheme();
                        break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                        game.setTiltSteeringEnabled(!game.isTiltSteeringEnabled());
                        break;
                    case SDL_CONTROLLER_BUTTON_START:
                        if (game.isGameOver()) game.startNewGame();
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK:
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        break;
                    case SDL_CONTROLLER_BUTTON_A:
                    case SDL_CONTROLLER_BUTTON_B:
                        game.fireOnce();
                        break;
                    default:
                        break;
                }
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        bool left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        bool right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        bool firing = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_Z] || keys[SDL_SCANCODE_UP];

        if (controller) {
            left |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) != 0;
            right |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0;
            firing |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) != 0;
            firing |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B) != 0;

            Sint16 axisX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            if (axisX < -8000) left = true;
            if (axisX > 8000) right = true;

            float gyro[3] = {0.0f, 0.0f, 0.0f};
            float accel[3] = {0.0f, 0.0f, 0.0f};
            bool hasSensor = false;
            if (SDL_GameControllerIsSensorEnabled(controller, SDL_SENSOR_GYRO)) {
                SDL_GameControllerGetSensorData(controller, SDL_SENSOR_GYRO, gyro, 3);
                hasSensor = true;
            }
            if (SDL_GameControllerIsSensorEnabled(controller, SDL_SENSOR_ACCEL)) {
                SDL_GameControllerGetSensorData(controller, SDL_SENSOR_ACCEL, accel, 3);
                hasSensor = true;
            }
            if (hasSensor) {
                game.setSensorMotion(gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2]);
            }
        }

        game.setInput(left, right, firing);

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(static_cast<double>(now - lastTick) / freq);
        lastTick = now;

        game.update(dt);
        game.render(renderer);
        SDL_RenderPresent(renderer);
    }

    if (controller) {
        SDL_GameControllerClose(controller);
    }
    game.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
