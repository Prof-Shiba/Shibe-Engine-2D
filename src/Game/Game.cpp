#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <fstream>
#include <string>
#include "../ECS/ECS.hpp"
#include "../../libs/glm/glm.hpp"
#include "../Logger/Logger.hpp"
#include "Game.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Components/RigidBodyComponent.hpp"
#include "../Components/SpriteComponent.hpp"
#include "../Systems/MovementSystem.hpp"
#include "../Systems/RenderSystem.hpp"

Game::Game() {
  is_running = false;

  registry = std::make_unique<Registry>();
  asset_manager = std::make_unique<AssetManager>();

  Logger::Log("Game Constructor Called");
}

Game::~Game() {
  Logger::Log("Game Destructor Called");
}

void Game::LoadLevel(int level) {
  // Systems
  registry->add_system<MovementSystem>();
  registry->add_system<RenderSystem>();

  // The linker will find #includes properly, however, when using images etc you must do it from the
  // makefiles perspective. It lives in the main dir, outside this /src/Game dir
  asset_manager->add_texture(renderer, "tank-image", "./assets/images/tank-tiger-right.png");
  asset_manager->add_texture(renderer, "truck-image", "./assets/images/truck-ford-right.png");
  asset_manager->add_texture(renderer, "jungle-tilemap", "./assets/tilemaps/jungle.png");

  // TODO: refactor this
  // why is it taking 3 seconds to launch?
  // this adds another second of launch time. 2s w/o this.
  // possibly due to use of templates?
  const uint8_t TILE_SIZE = 32;
  uint8_t number_of_map_cols = 25;
  uint8_t number_of_map_rows = 20;
  float tile_scale = 3.5;

  std::ifstream in_file {"./assets/tilemaps/jungle.map"};
  if (in_file) {
    for (int y = 0; y < number_of_map_rows; y++) {
      for (int x = 0; x < number_of_map_cols; x++) {
        char ch;

        in_file.get(ch);
        uint16_t src_rect_y = std::atoi(&ch) * TILE_SIZE;

        in_file.get(ch);
        uint16_t src_rect_x = std::atoi(&ch) * TILE_SIZE;

        in_file.ignore();

        Entity map_tile = registry->create_entity();
        map_tile.add_component<TransformComponent>(glm::vec2(x * (tile_scale * TILE_SIZE), y * (tile_scale * TILE_SIZE)), glm::vec2(tile_scale, tile_scale), 0.0);
        map_tile.add_component<SpriteComponent>("jungle-tilemap", TILE_SIZE, TILE_SIZE, src_rect_x, src_rect_y);
      }
    }
  } else {
    Logger::Err("Failed opening jungle.map file. Should be in assets/tilemaps/jungle.map");
  }

  in_file.close();

  // Entities & Components
  Entity tank = registry->create_entity();

  // testing
  auto tank_position = glm::vec2(10, 10);
  auto tank_scale = glm::vec2(3.0, 3.0);
  auto tank_velocity = glm::vec2(50.0, 0.0);
  float tank_rotation = 40.0;

  tank.add_component<TransformComponent>(tank_position, tank_scale, tank_rotation);
  tank.add_component<RigidBodyComponent>(tank_velocity);
  tank.add_component<SpriteComponent>("tank-image", 32, 32); // imgs are 32px, width and height

  Entity truck = registry->create_entity();

  auto truck_position = glm::vec2(90, 120);
  auto truck_scale = glm::vec2(1.0, 1.0);
  auto truck_velocity = glm::vec2(30.0, 30.0);
  float truck_rotation = 0.0;

  truck.add_component<TransformComponent>(truck_position, truck_scale, truck_rotation);
  truck.add_component<RigidBodyComponent>(truck_velocity);
  truck.add_component<SpriteComponent>("truck-image", 32, 32);
}

void Game::Setup() {
  LoadLevel(1);
}

void Game::Update() {
  // Yield resources to OS
  uint32_t time_to_wait = MS_PER_FRAME - (SDL_GetTicks() - ms_previous_frame);
  if (time_to_wait > 0 && time_to_wait <= MS_PER_FRAME)
    SDL_Delay(time_to_wait);
 
  // DT is diff in ticks since last frame, converted to seconds
  double delta_time = (SDL_GetTicks() - ms_previous_frame) / 1000.0;

  // Store current frame time
  ms_previous_frame = SDL_GetTicks();

  registry->get_system<MovementSystem>().Update(delta_time);

  // Process entities that are waiting to be created/destroyed
  registry->update();
};

void Game::Render() {
  SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
  SDL_RenderClear(renderer);
  
  registry->get_system<RenderSystem>().Update(renderer, asset_manager);

  // Double buffer
  SDL_RenderPresent(renderer);
};

void Game::Initialize() {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    Logger::Err("SDL failed to Initialize!");
    return;
  }

  SDL_DisplayMode display_mode;
  SDL_GetCurrentDisplayMode(0, &display_mode);

  // This will be the total area the player can view
  WIDTH = 1280;
  HEIGHT = 720;

  window = SDL_CreateWindow(
    "Shibe Engine",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    WIDTH,
    HEIGHT,
    SDL_WINDOW_ALWAYS_ON_TOP
  );
  if (!window) {
    Logger::Err("Failed creating SDL window!");
    return;
  }

  renderer = SDL_CreateRenderer(window, DEFAULT_MONITOR_NUMBER, 0);
  if (!renderer) {
    Logger::Err("Failed to create SDL renderer!");
    return;
  }

  // Sets the actual video mode to fullscreen, keeping that width from earlier
  // avoids large and smaller monitors/resolutions seeing more or less
  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
  is_running = true;
};

void Game::Run() {
  Setup();
  while (is_running) {
    ProcessInput();
    Update();
    Render();
  }
};

void Game::ProcessInput() {
  SDL_Event sdl_event;

  while (SDL_PollEvent(&sdl_event)) {
    switch (sdl_event.type) {
      case SDL_QUIT:
        is_running = false; 
        break;

      case SDL_KEYDOWN:
        if (sdl_event.key.keysym.sym == SDLK_ESCAPE) {
          is_running = false;
        }
        break;
    }
  }
};

void Game::Destroy() {
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
};
