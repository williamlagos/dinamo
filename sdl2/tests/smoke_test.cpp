#include "DinamoGame.hpp"
#include "PngSprites.hpp"
#include "SharedConfig.hpp"

#include <cassert>
#include <iostream>

int main() {
    // 1. Verify SharedConfig loads palettes.json and waves.json
    dinamo::SharedConfig cfg;
    bool loaded = cfg.load();
    assert(loaded && "Expected shared/palettes.json and shared/waves.json to load");
    assert(cfg.nativeWidth == 160 && cfg.nativeHeight == 144);
    assert(cfg.tilesX == 20 && cfg.tilesY == 18);
    assert(cfg.slots.size() == 30 && "Expected 30 formation slots from shared/waves.json");
    assert(cfg.subwaves.size() == 30 && "Expected 30 subwave entries from shared/waves.json");

    // 2. Verify PNG sprite sheet decoding via libpng
    dinamo::PngSprites sprites;
    bool pngLoaded = sprites.loadFromSharedDir(cfg.sharedDir);
    assert(pngLoaded && "Expected all 6 GB PNG sprites to decode");
    assert(sprites.playerSheet.width == 96 && sprites.playerSheet.height == 16);
    assert(sprites.enemySheet.width == 64 && sprites.enemySheet.height == 32);
    assert(sprites.starfieldBg.width == 160 && sprites.starfieldBg.height == 144);

    // 3. Verify DinamoGame + Elements C++ Core integration (headless tick simulation)
    dinamo::DinamoGame game;
    assert(game.init(nullptr, cfg.sharedDir));
    assert(game.enemyCount() == 30);
    assert(game.formationMatrix().getCols() == 10 && game.formationMatrix().getRows() == 4);
    assert(game.tileMatrix().getCols() == 20 && game.tileMatrix().getRows() == 18);
    assert(game.formationMatrix().get(3, 0) == 3); // Boss slot
    assert(!game.quadtreeSignature().empty());

    // Advance past stage intro (1.6s)
    for (int i = 0; i < 120; ++i) {
        game.update(1.0f / 60.0f);
    }
    assert(!game.isStageIntro());

    // Simulate moving right & firing
    float startX = game.playerX();
    game.setInput(false, true, true);
    for (int i = 0; i < 15; ++i) {
        game.update(1.0f / 60.0f);
    }
    assert(game.playerX() > startX);
    assert(game.playerBulletCount() >= 1);

    // Cycle palettes cleanly (DMG -> POCKET -> GBC -> DMG)
    assert(game.currentTheme() == dinamo::ThemeKind::DMG);
    game.cycleTheme();
    assert(game.currentTheme() == dinamo::ThemeKind::POCKET);
    game.cycleTheme();
    assert(game.currentTheme() == dinamo::ThemeKind::GBC);
    game.cycleTheme();
    assert(game.currentTheme() == dinamo::ThemeKind::DMG);

    // 4. Verify Elements SensorFrame IMU gyroscope/accelerometer integration
    game.setInput(false, false, false);
    game.setTiltSteeringEnabled(true);
    float preTiltX = game.playerX();
    game.setSensorMotion(0.0f, -0.8f, 0.0f, -5.0f, 0.0f, 9.8f);
    assert(game.sensorFrame().hasMotion());
    assert(game.sensorFrame().getTiltX() < 0.0);
    for (int i = 0; i < 10; ++i) {
        game.update(1.0f / 60.0f);
    }
    assert(game.playerX() < preTiltX && "Expected negative IMU tilt to steer fighter left");

    std::cout << "dinamo_smoke_test passed! Enemies=" << game.enemyCount()
              << " Quadtree=" << game.quadtreeSignature()
              << " PlayerX=" << game.playerX() << "\n";
    return 0;
}
