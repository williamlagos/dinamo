#pragma once

#include "PngSprites.hpp"
#include "SharedConfig.hpp"
#include "SoundEngine.hpp"

#include <elements.h>
#include <matrix.h>
#include <ranking.h>

#include <SDL.h>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace dinamo {

enum class EnemyKind {
    BEE,
    BUTTERFLY,
    BOSS
};

enum class EnemyBehavior {
    ENTERING,
    FORMATION,
    DIVING,
    RETURNING
};

struct EnemyUnit {
    EnemyKind kind = EnemyKind::BEE;
    EnemyBehavior behavior = EnemyBehavior::FORMATION;
    int row = 0;
    int col = 0;
    float x = 0.0f;
    float y = 0.0f;
    float homeX = 0.0f;
    float homeY = 0.0f;
    int points = 50;
    int divePoints = 100;
    bool hasFired = false;
    float diveProgress = 0.0f;
    float diveStartX = 0.0f;
    float diveStartY = 0.0f;
    float diveTargetX = 0.0f;
    double entranceDelay = 0.0;
    Element entity;

    EnemyUnit(const std::string& name, int hp) : entity(name) {
        if (hp < 100) {
            entity.hurtElement(100.0 - static_cast<double>(hp));
        }
        entity.whichSize(12, 12);
    }
};

struct Projectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    bool fromPlayer = true;
};

struct ExplosionEffect {
    float x = 0.0f;
    float y = 0.0f;
    int frame = 0;
    float timer = 0.0f;
};

struct StarParticle {
    float x = 0.0f;
    float y = 0.0f;
    float speed = 16.0f;
    int colorIndex = 0;
};

class DinamoGame {
public:
    DinamoGame();
    ~DinamoGame();

    bool init(SDL_Renderer* renderer = nullptr, const std::string& explicitSharedDir = "");
    void shutdown();

    void startNewGame();
    void update(float dt);
    void render(SDL_Renderer* renderer);

    void cycleTheme();
    void setTheme(ThemeKind kind);
    ThemeKind currentTheme() const { return currentTheme_; }

    void setInput(bool left, bool right, bool firing);
    void setSensorMotion(float gyroX, float gyroY, float gyroZ, float accelX, float accelY, float accelZ);
    void setTiltSteeringEnabled(bool enabled) { tiltSteeringEnabled_ = enabled; }
    bool isTiltSteeringEnabled() const { return tiltSteeringEnabled_; }
    void fireOnce();

    // Accessors for testing & HUD inspection
    int score() const { return score_; }
    int highScore() const { return highScore_; }
    int lives() const { return lives_; }
    int stage() const { return stage_; }
    bool isGameOver() const { return gameOver_; }
    bool isStageIntro() const { return stageIntro_; }
    int enemyCount() const { return static_cast<int>(enemies_.size()); }
    int playerBulletCount() const;
    float playerX() const { return playerX_; }
    const IntMatrix& formationMatrix() const { return formationMatrix_; }
    const IntMatrix& tileMatrix() const { return tileMatrix_; }
    const SensorFrame& sensorFrame() const { return entityRegistry_.getSensors(); }
    std::string quadtreeSignature() { return quadtree_.printTree(); }
    const SharedConfig& config() const { return config_; }

    void syncLeaderboardAsync(bool submitCurrentScore);

private:
    void startStage(int stageNum);
    void rebuildThemeTextures(SDL_Renderer* renderer);
    void destroyTextures();
    void setupStars();
    void syncElementsMatrices();
    void updateStars(float dt);
    void updatePlayer(float dt);
    void updateEnemies(float dt);
    void updateProjectiles(float dt);
    void updateExplosions(float dt);
    void checkCollisions();
    void checkStageProgress();
    void triggerDiveBomb();
    void triggerPlayerHit();

    SharedConfig config_;
    PngSprites sprites_;
    SoundEngine sound_;

    ThemeKind currentTheme_ = ThemeKind::DMG;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* bgTex_ = nullptr;
    SDL_Texture* playerTex_ = nullptr;
    SDL_Texture* bossTex_ = nullptr;
    SDL_Texture* butterflyTex_ = nullptr;
    SDL_Texture* beeTex_ = nullptr;
    SDL_Texture* bulletTex_ = nullptr;
    SDL_Texture* explodeTex_ = nullptr;

    // Elements C++ Core Primitives
    Element playerEntity_;
    Elements entityRegistry_;
    IntMatrix formationMatrix_;
    IntMatrix tileMatrix_;
    Codes quadtree_;

    // Gameplay State
    int score_ = 0;
    int highScore_ = 20000;
    int lives_ = 3;
    int stage_ = 1;
    bool gameOver_ = false;
    bool stageIntro_ = true;
    bool playerDead_ = false;
    float stateTimer_ = 0.0f;
    float invulnerableTimer_ = 0.0f;

    float playerX_ = 80.0f;
    bool inputLeft_ = false;
    bool inputRight_ = false;
    bool inputFiring_ = false;
    bool tiltSteeringEnabled_ = false;
    float fireCooldown_ = 0.0f;

    float stageElapsed_ = 0.0f;
    float formationTime_ = 0.0f;
    float wingTimer_ = 0.0f;
    int wingFrame_ = 0;
    float diveTimer_ = 0.0f;
    float bgScrollY_ = 0.0f;
    unsigned int rngState_ = 0x13579BDFu;

    std::vector<EnemyUnit> enemies_;
    std::vector<Projectile> bullets_;
    std::vector<ExplosionEffect> explosions_;
    std::vector<StarParticle> stars_;

    std::atomic<int> remoteHighScore_{20000};
    std::mutex netMutex_;
    std::vector<std::thread> netThreads_;
};

} // namespace dinamo
