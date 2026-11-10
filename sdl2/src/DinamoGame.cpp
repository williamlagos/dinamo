#include "DinamoGame.hpp"
#include "BitmapFont.hpp"

#include <httplib.h>
#include <json.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

using json11::Json;

namespace dinamo {

namespace {

constexpr float kPi = 3.14159265358979323846f;

EnemyKind parseEnemyKind(const std::string& type) {
    if (type == "boss") return EnemyKind::BOSS;
    if (type == "butterfly") return EnemyKind::BUTTERFLY;
    return EnemyKind::BEE;
}

uint32_t nextRand(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

float randUnit(uint32_t& state) {
    return static_cast<float>(nextRand(state) & 0xFFFFu) / 65535.0f;
}

std::pair<std::string, int> parseServerUrl(const std::string& url) {
    std::string clean = url;
    if (clean.rfind("http://", 0) == 0) {
        clean = clean.substr(7);
    } else if (clean.rfind("https://", 0) == 0) {
        clean = clean.substr(8);
    }
    auto slash = clean.find('/');
    if (slash != std::string::npos) {
        clean = clean.substr(0, slash);
    }
    auto colon = clean.find(':');
    if (colon != std::string::npos) {
        return {clean.substr(0, colon), std::atoi(clean.substr(colon + 1).c_str())};
    }
    return {clean, 8000};
}

} // namespace

DinamoGame::DinamoGame()
    : playerEntity_("DinamoFighter"),
      entityRegistry_(true),
      formationMatrix_(10, 4, 0),
      tileMatrix_(20, 18, 0) {
}

DinamoGame::~DinamoGame() {
    shutdown();
}

bool DinamoGame::init(SDL_Renderer* renderer, const std::string& explicitSharedDir) {
    renderer_ = renderer;
    config_.load(explicitSharedDir);

    if (!config_.sharedDir.empty()) {
        sprites_.loadFromSharedDir(config_.sharedDir);
    }

    if (renderer_) {
        sound_.init();
        rebuildThemeTextures(renderer_);
        syncLeaderboardAsync(false);
    }

    setupStars();
    startNewGame();
    return true;
}

void DinamoGame::shutdown() {
    for (auto& t : netThreads_) {
        if (t.joinable()) t.join();
    }
    netThreads_.clear();
    destroyTextures();
    sound_.shutdown();
    renderer_ = nullptr;
}

void DinamoGame::destroyTextures() {
    auto kill = [](SDL_Texture*& tex) {
        if (tex) {
            SDL_DestroyTexture(tex);
            tex = nullptr;
        }
    };
    kill(bgTex_);
    kill(playerTex_);
    kill(bossTex_);
    kill(butterflyTex_);
    kill(beeTex_);
    kill(bulletTex_);
    kill(explodeTex_);
}

void DinamoGame::rebuildThemeTextures(SDL_Renderer* renderer) {
    destroyTextures();
    if (!renderer) return;

    const ThemePalette& theme = config_.getTheme(currentTheme_);
    RGBA baseQuad[4] = {theme.lightest, theme.light, theme.dark, theme.darkest};

    const RGBA* playerQuad = theme.hasPerRoleColors ? theme.playerColors : baseQuad;
    const RGBA* bossQuad = theme.hasPerRoleColors ? theme.bossColors : baseQuad;
    const RGBA* butterflyQuad = theme.hasPerRoleColors ? theme.butterflyColors : baseQuad;
    const RGBA* beeQuad = theme.hasPerRoleColors ? theme.beeColors : baseQuad;

    bgTex_ = PngSprites::buildThemedTexture(renderer, sprites_.starfieldBg, baseQuad, true);
    playerTex_ = PngSprites::buildThemedTexture(renderer, sprites_.playerSheet, playerQuad, true);
    bossTex_ = PngSprites::buildThemedTexture(renderer, sprites_.mineSheet, bossQuad, true);
    butterflyTex_ = PngSprites::buildThemedTexture(renderer, sprites_.enemySheet, butterflyQuad, true);
    beeTex_ = PngSprites::buildThemedTexture(renderer, sprites_.enemySheet, beeQuad, true);
    bulletTex_ = PngSprites::buildThemedTexture(renderer, sprites_.bulletSheet, playerQuad, true);
    explodeTex_ = PngSprites::buildThemedTexture(renderer, sprites_.explodeSheet, baseQuad, true);
}

void DinamoGame::setTheme(ThemeKind kind) {
    currentTheme_ = kind;
    if (renderer_) {
        rebuildThemeTextures(renderer_);
    }
}

void DinamoGame::cycleTheme() {
    int next = (static_cast<int>(currentTheme_) + 1) % 3;
    setTheme(static_cast<ThemeKind>(next));
}

void DinamoGame::setupStars() {
    stars_.clear();
    uint32_t seed = 42u;
    for (int i = 0; i < 28; ++i) {
        StarParticle s;
        s.x = randUnit(seed) * static_cast<float>(config_.nativeWidth);
        s.y = randUnit(seed) * static_cast<float>(config_.nativeHeight);
        s.speed = 10.0f + randUnit(seed) * 24.0f;
        s.colorIndex = i;
        stars_.push_back(s);
    }
}

void DinamoGame::startNewGame() {
    score_ = 0;
    highScore_ = std::max(highScore_, remoteHighScore_.load());
    lives_ = 3;
    stage_ = config_.stage > 0 ? config_.stage : 1;
    gameOver_ = false;
    playerDead_ = false;
    playerX_ = static_cast<float>(config_.nativeWidth) * 0.5f;
    playerEntity_.whichPosition(static_cast<int>(playerX_), 126);
    playerEntity_.whichSize(12, 12);

    startStage(stage_);
}

void DinamoGame::startStage(int stageNum) {
    stage_ = stageNum;
    stageIntro_ = true;
    stateTimer_ = 1.6f;
    stageElapsed_ = 0.0f;
    formationTime_ = 0.0f;
    diveTimer_ = 0.0f;
    enemies_.clear();
    bullets_.clear();
    explosions_.clear();

    // Map 10x4 formation from shared/waves.json inside 160x144 viewport
    const float colSpacing = 12.0f;
    const float rowSpacing = 12.0f;
    const float startX = 26.0f;
    const float startY = 24.0f;

    for (const auto& slot : config_.slots) {
        float hx = startX + static_cast<float>(slot.col) * colSpacing;
        float hy = startY + static_cast<float>(slot.row) * rowSpacing;

        EnemyUnit unit(slot.type, slot.health);
        unit.kind = parseEnemyKind(slot.type);
        unit.row = slot.row;
        unit.col = slot.col;
        unit.homeX = hx;
        unit.homeY = hy;
        unit.points = slot.points;
        unit.divePoints = slot.divePoints;

        // Check if subwaves specifies a choreographed entrance for this slot
        double delay = -1.0;
        std::string path = "topLeft";
        for (const auto& sw : config_.subwaves) {
            if (sw.row == slot.row && sw.col == slot.col) {
                delay = sw.delay;
                path = sw.path;
                break;
            }
        }

        if (delay >= 0.0) {
            unit.behavior = EnemyBehavior::ENTERING;
            unit.entranceDelay = delay * 0.35; // Paced smoothly for arcade action
            if (path == "leftSide") {
                unit.x = -12.0f;
                unit.y = 70.0f;
            } else if (path == "rightSide") {
                unit.x = static_cast<float>(config_.nativeWidth) + 12.0f;
                unit.y = 70.0f;
            } else if (path == "topRight") {
                unit.x = static_cast<float>(config_.nativeWidth) * 0.72f;
                unit.y = -14.0f;
            } else {
                unit.x = static_cast<float>(config_.nativeWidth) * 0.28f;
                unit.y = -14.0f;
            }
        } else {
            unit.behavior = EnemyBehavior::FORMATION;
            unit.x = hx;
            unit.y = hy;
        }
        unit.entity.whichPosition(static_cast<int>(unit.x), static_cast<int>(unit.y));
        enemies_.push_back(std::move(unit));
    }

    syncElementsMatrices();
}

void DinamoGame::syncElementsMatrices() {
    formationMatrix_ = IntMatrix(config_.formationCols, config_.formationRows, 0);
    tileMatrix_ = IntMatrix(config_.tilesX, config_.tilesY, 0);

    int quadCounts[4] = {0, 0, 0, 0};
    for (const auto& e : enemies_) {
        int val = (e.kind == EnemyKind::BOSS) ? 3 : (e.kind == EnemyKind::BUTTERFLY ? 2 : 1);
        formationMatrix_.set(e.col, e.row, val);

        int tx = std::clamp(static_cast<int>(e.x) / config_.tileSize, 0, config_.tilesX - 1);
        int ty = std::clamp(static_cast<int>(e.y) / config_.tileSize, 0, config_.tilesY - 1);
        tileMatrix_.set(tx, ty, val);

        int qIdx = ((e.row >= 2) ? 2 : 0) + ((e.col >= 5) ? 1 : 0);
        quadCounts[qIdx]++;
    }

    // Record player cell in tile matrix
    int ptx = std::clamp(static_cast<int>(playerX_) / config_.tileSize, 0, config_.tilesX - 1);
    tileMatrix_.set(ptx, config_.tilesY - 2, 9);

    // Update Quadtree spatial signature via Codes
    quadtree_.clearTree();
    std::string codeStr;
    for (int q = 0; q < 4; ++q) {
        codeStr.push_back(quadCounts[q] > 0 ? 'p' : 'b');
    }
    quadtree_.parseCode(codeStr);
}

void DinamoGame::setInput(bool left, bool right, bool firing) {
    inputLeft_ = left;
    inputRight_ = right;
    inputFiring_ = firing;
}

void DinamoGame::setSensorMotion(float gyroX, float gyroY, float gyroZ, float accelX, float accelY, float accelZ) {
    SensorFrame frame;
    frame.setGyro(gyroX, gyroY, gyroZ);
    frame.setAccel(accelX, accelY, accelZ);
    entityRegistry_.updateSensors(frame);
}

void DinamoGame::fireOnce() {
    if (gameOver_ || stageIntro_ || playerDead_) return;
    if (playerBulletCount() < 2 && fireCooldown_ <= 0.0f) {
        sound_.playLaser();
        bullets_.push_back({playerX_, 122.0f, 0.0f, -185.0f, true});
        fireCooldown_ = 0.18f;
    }
}

int DinamoGame::playerBulletCount() const {
    int count = 0;
    for (const auto& b : bullets_) {
        if (b.fromPlayer) count++;
    }
    return count;
}

void DinamoGame::update(float dt) {
    dt = std::min(dt, 0.05f);
    updateStars(dt);

    if (gameOver_) return;

    if (stageIntro_) {
        stateTimer_ -= dt;
        if (stateTimer_ <= 0.0f) {
            stageIntro_ = false;
        }
        return;
    }

    stageElapsed_ += dt;
    updatePlayer(dt);
    updateEnemies(dt);
    updateProjectiles(dt);
    updateExplosions(dt);
    checkCollisions();
    syncElementsMatrices();
    checkStageProgress();
}

void DinamoGame::updateStars(float dt) {
    bgScrollY_ += 12.0f * dt;
    if (bgScrollY_ >= static_cast<float>(config_.nativeHeight)) {
        bgScrollY_ -= static_cast<float>(config_.nativeHeight);
    }
    const float tiltX = static_cast<float>(entityRegistry_.getSensors().getTiltX());
    const float widthF = static_cast<float>(config_.nativeWidth);
    for (auto& s : stars_) {
        s.y += s.speed * dt;
        if (std::abs(tiltX) > 0.02f) {
            s.x += tiltX * (s.speed * 0.45f) * dt;
            if (s.x < 0.0f) s.x += widthF;
            if (s.x >= widthF) s.x -= widthF;
        }
        if (s.y >= static_cast<float>(config_.nativeHeight)) {
            s.y -= static_cast<float>(config_.nativeHeight);
            s.x = std::fmod(s.x + 37.0f, widthF);
        }
    }
}

void DinamoGame::updatePlayer(float dt) {
    if (playerDead_) {
        stateTimer_ -= dt;
        if (stateTimer_ <= 0.0f) {
            if (lives_ > 0) {
                playerDead_ = false;
                playerX_ = static_cast<float>(config_.nativeWidth) * 0.5f;
                invulnerableTimer_ = 2.0f;
            } else {
                gameOver_ = true;
                syncLeaderboardAsync(true);
            }
        }
        return;
    }

    if (invulnerableTimer_ > 0.0f) {
        invulnerableTimer_ = std::max(0.0f, invulnerableTimer_ - dt);
    }

    const float speed = 92.0f;
    int vx = 0;
    if (inputLeft_) {
        playerX_ -= speed * dt;
        vx -= static_cast<int>(speed);
    }
    if (inputRight_) {
        playerX_ += speed * dt;
        vx += static_cast<int>(speed);
    }
    if (tiltSteeringEnabled_) {
        const float tiltX = static_cast<float>(entityRegistry_.getSensors().getTiltX());
        if (std::abs(tiltX) > 0.08f) {
            playerX_ += tiltX * speed * dt;
            vx += static_cast<int>(tiltX * speed);
        }
    }
    playerX_ = std::clamp(playerX_, 10.0f, static_cast<float>(config_.nativeWidth) - 10.0f);
    playerEntity_.whichSpeed(vx, 0);
    playerEntity_.whichPosition(static_cast<int>(playerX_), 128);

    if (fireCooldown_ > 0.0f) {
        fireCooldown_ = std::max(0.0f, fireCooldown_ - dt);
    }
    if (inputFiring_) {
        fireOnce();
    }
}

void DinamoGame::updateEnemies(float dt) {
    formationTime_ += dt * 2.2f;
    float swayX = std::sin(formationTime_) * 8.0f;

    wingTimer_ += dt;
    if (wingTimer_ >= 0.24f) {
        wingTimer_ = 0.0f;
        wingFrame_ = (wingFrame_ + 1) % 2;
    }

    diveTimer_ += dt;
    float diveInterval = std::max(0.9f, 2.4f - static_cast<float>(stage_ - 1) * 0.25f);
    if (diveTimer_ >= diveInterval) {
        diveTimer_ = 0.0f;
        triggerDiveBomb();
    }

    for (auto& e : enemies_) {
        if (e.behavior == EnemyBehavior::ENTERING) {
            if (stageElapsed_ < e.entranceDelay) {
                continue;
            }
            float targetX = e.homeX + swayX;
            float targetY = e.homeY;
            float dx = targetX - e.x;
            float dy = targetY - e.y;
            float dist = std::hypot(dx, dy);
            float moveSpeed = 120.0f;
            if (dist <= moveSpeed * dt) {
                e.x = targetX;
                e.y = targetY;
                e.behavior = EnemyBehavior::FORMATION;
            } else {
                e.x += (dx / dist) * moveSpeed * dt;
                e.y += (dy / dist) * moveSpeed * dt;
            }
        } else if (e.behavior == EnemyBehavior::FORMATION) {
            e.x = e.homeX + swayX;
            e.y = e.homeY;
        } else if (e.behavior == EnemyBehavior::DIVING) {
            e.diveProgress += dt * (0.45f + static_cast<float>(stage_ - 1) * 0.05f);
            float p = e.diveProgress;
            e.x = (1.0f - p) * e.diveStartX + p * e.diveTargetX + std::sin(p * kPi * 2.0f) * 18.0f;
            e.y = e.diveStartY + p * (static_cast<float>(config_.nativeHeight) + 18.0f);

            if (!e.hasFired && e.y > 55.0f && e.y < 102.0f) {
                e.hasFired = true;
                bullets_.push_back({e.x, e.y + 4.0f, 0.0f, 108.0f, false});
            }

            if (e.y > static_cast<float>(config_.nativeHeight) + 10.0f) {
                e.behavior = EnemyBehavior::RETURNING;
                e.y = -10.0f;
                e.x = e.homeX + swayX;
                e.hasFired = false;
            }
        } else if (e.behavior == EnemyBehavior::RETURNING) {
            e.x = e.homeX + swayX;
            e.y += 75.0f * dt;
            if (e.y >= e.homeY) {
                e.y = e.homeY;
                e.behavior = EnemyBehavior::FORMATION;
            }
        }
        e.entity.whichPosition(static_cast<int>(e.x), static_cast<int>(e.y));
    }
}

void DinamoGame::triggerDiveBomb() {
    std::vector<size_t> readyIndices;
    for (size_t i = 0; i < enemies_.size(); ++i) {
        if (enemies_[i].behavior == EnemyBehavior::FORMATION) {
            readyIndices.push_back(i);
        }
    }
    if (readyIndices.empty()) return;

    size_t pick = readyIndices[nextRand(rngState_) % readyIndices.size()];
    EnemyUnit& diver = enemies_[pick];
    diver.behavior = EnemyBehavior::DIVING;
    diver.diveProgress = 0.0f;
    diver.diveStartX = diver.x;
    diver.diveStartY = diver.y;
    diver.diveTargetX = playerX_;
    diver.hasFired = false;
    sound_.playDive();
}

void DinamoGame::updateProjectiles(float dt) {
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        it->x += it->vx * dt;
        it->y += it->vy * dt;
        if (it->y < -10.0f || it->y > static_cast<float>(config_.nativeHeight) + 10.0f) {
            it = bullets_.erase(it);
        } else {
            ++it;
        }
    }
}

void DinamoGame::updateExplosions(float dt) {
    for (auto it = explosions_.begin(); it != explosions_.end();) {
        it->timer += dt;
        if (it->timer >= 0.06f) {
            it->timer = 0.0f;
            it->frame++;
            if (it->frame >= 4) {
                it = explosions_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void DinamoGame::triggerPlayerHit() {
    if (playerDead_ || invulnerableTimer_ > 0.0f) return;
    sound_.playPlayerDeath();
    explosions_.push_back({playerX_, 128.0f, 0, 0.0f});
    lives_--;
    playerDead_ = true;
    stateTimer_ = 1.4f;
}

void DinamoGame::checkCollisions() {
    for (auto bIt = bullets_.begin(); bIt != bullets_.end();) {
        bool consumed = false;
        if (bIt->fromPlayer) {
            for (auto eIt = enemies_.begin(); eIt != enemies_.end(); ++eIt) {
                if (std::hypot(bIt->x - eIt->x, bIt->y - eIt->y) < 7.5f) {
                    consumed = true;
                    eIt->entity.hurtElement(1.0);
                    if (eIt->entity.getHitPoints() <= 0.0) {
                        sound_.playExplosion();
                        explosions_.push_back({eIt->x, eIt->y, 0, 0.0f});
                        bool diving = (eIt->behavior == EnemyBehavior::DIVING);
                        score_ += diving ? eIt->divePoints : eIt->points;
                        highScore_ = std::max(highScore_, score_);
                        enemies_.erase(eIt);
                    } else {
                        sound_.playHit();
                        score_ += 50;
                        highScore_ = std::max(highScore_, score_);
                    }
                    break;
                }
            }
        } else if (!playerDead_ && invulnerableTimer_ <= 0.0f) {
            if (std::hypot(bIt->x - playerX_, bIt->y - 128.0f) < 6.5f) {
                consumed = true;
                triggerPlayerHit();
            }
        }

        if (consumed) {
            bIt = bullets_.erase(bIt);
        } else {
            ++bIt;
        }
    }

    // Enemy ships colliding directly with player fighter
    if (!playerDead_ && invulnerableTimer_ <= 0.0f) {
        for (auto eIt = enemies_.begin(); eIt != enemies_.end(); ++eIt) {
            if (std::hypot(eIt->x - playerX_, eIt->y - 128.0f) < 9.0f) {
                explosions_.push_back({eIt->x, eIt->y, 0, 0.0f});
                enemies_.erase(eIt);
                triggerPlayerHit();
                break;
            }
        }
    }
}

void DinamoGame::checkStageProgress() {
    if (enemies_.empty() && !gameOver_ && !stageIntro_) {
        startStage(stage_ + 1);
    }
}

void DinamoGame::render(SDL_Renderer* renderer) {
    if (!renderer) return;
    const ThemePalette& theme = config_.getTheme(currentTheme_);

    // 1. Background fill + scrolling GB starfield
    SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, 255);
    SDL_RenderClear(renderer);

    if (bgTex_) {
        int yOff = static_cast<int>(bgScrollY_);
        SDL_Rect r1{0, yOff, config_.nativeWidth, config_.nativeHeight};
        SDL_Rect r2{0, yOff - config_.nativeHeight, config_.nativeWidth, config_.nativeHeight};
        SDL_RenderCopy(renderer, bgTex_, nullptr, &r1);
        SDL_RenderCopy(renderer, bgTex_, nullptr, &r2);
    }

    if (!theme.starColors.empty()) {
        for (const auto& s : stars_) {
            const RGBA& c = theme.starColors[static_cast<size_t>(s.colorIndex) % theme.starColors.size()];
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_RenderDrawPoint(renderer, static_cast<int>(s.x), static_cast<int>(s.y));
        }
    }

    // 2. Player Fighter (16x16 sprite scaled smoothly to 14x14, blinking when invulnerable)
    if (!playerDead_) {
        bool show = (invulnerableTimer_ <= 0.0f) || (static_cast<int>(invulnerableTimer_ * 16.0f) % 2 == 0);
        if (show) {
            if (playerTex_) {
                SDL_Rect src{wingFrame_ * 16, 0, 16, 16};
                SDL_Rect dst{static_cast<int>(playerX_) - 7, 121, 14, 14};
                SDL_RenderCopy(renderer, playerTex_, &src, &dst);
            } else {
                SDL_SetRenderDrawColor(renderer, theme.lightest.r, theme.lightest.g, theme.lightest.b, 255);
                SDL_Rect dst{static_cast<int>(playerX_) - 5, 123, 10, 10};
                SDL_RenderFillRect(renderer, &dst);
            }
        }
    }

    // 3. Armada Enemies
    for (const auto& e : enemies_) {
        SDL_Texture* tex = nullptr;
        if (e.kind == EnemyKind::BOSS) tex = bossTex_;
        else if (e.kind == EnemyKind::BUTTERFLY) tex = butterflyTex_;
        else tex = beeTex_;

        if (tex) {
            SDL_Rect src{wingFrame_ * 16, 0, 16, 16};
            SDL_Rect dst{static_cast<int>(e.x) - 6, static_cast<int>(e.y) - 6, 12, 12};
            SDL_RenderCopy(renderer, tex, &src, &dst);
        } else {
            SDL_SetRenderDrawColor(renderer, theme.light.r, theme.light.g, theme.light.b, 255);
            SDL_Rect dst{static_cast<int>(e.x) - 4, static_cast<int>(e.y) - 4, 8, 8};
            SDL_RenderFillRect(renderer, &dst);
        }
    }

    // 4. Projectiles
    for (const auto& b : bullets_) {
        if (b.fromPlayer) {
            SDL_SetRenderDrawColor(renderer, theme.lightest.r, theme.lightest.g, theme.lightest.b, 255);
            SDL_Rect r{static_cast<int>(b.x) - 1, static_cast<int>(b.y) - 3, 2, 6};
            SDL_RenderFillRect(renderer, &r);
        } else if (bulletTex_) {
            SDL_Rect dst{static_cast<int>(b.x) - 4, static_cast<int>(b.y) - 4, 8, 8};
            SDL_RenderCopy(renderer, bulletTex_, nullptr, &dst);
        } else {
            SDL_SetRenderDrawColor(renderer, theme.dark.r, theme.dark.g, theme.dark.b, 255);
            SDL_Rect r{static_cast<int>(b.x) - 1, static_cast<int>(b.y) - 2, 2, 4};
            SDL_RenderFillRect(renderer, &r);
        }
    }

    // 5. Explosions
    for (const auto& exp : explosions_) {
        if (explodeTex_) {
            SDL_Rect src{exp.frame * 16, 0, 16, 16};
            SDL_Rect dst{static_cast<int>(exp.x) - 8, static_cast<int>(exp.y) - 8, 16, 16};
            SDL_RenderCopy(renderer, explodeTex_, &src, &dst);
        } else {
            int sz = (exp.frame + 1) * 3;
            SDL_SetRenderDrawColor(renderer, theme.lightest.r, theme.lightest.g, theme.lightest.b, 255);
            SDL_Rect r{static_cast<int>(exp.x) - sz / 2, static_cast<int>(exp.y) - sz / 2, sz, sz};
            SDL_RenderDrawRect(renderer, &r);
        }
    }

    // 6. Retro Arcade HUD
    BitmapFont::drawText(renderer, "1UP", 4, 3, theme.lightest);
    BitmapFont::drawText(renderer, std::to_string(score_), 4, 10, theme.light);

    BitmapFont::drawText(renderer, "HIGH", 62, 3, theme.lightest);
    BitmapFont::drawText(renderer, std::to_string(highScore_), 62, 10, theme.light);

    BitmapFont::drawText(renderer, "ST " + std::to_string(stage_), 132, 10, theme.light);

    // Bottom status bar: life icons & theme indicator
    for (int i = 0; i < std::max(0, lives_ - 1); ++i) {
        if (playerTex_) {
            SDL_Rect src{0, 0, 16, 16};
            SDL_Rect dst{4 + i * 9, 134, 8, 8};
            SDL_RenderCopy(renderer, playerTex_, &src, &dst);
        } else {
            SDL_SetRenderDrawColor(renderer, theme.lightest.r, theme.lightest.g, theme.lightest.b, 255);
            SDL_Rect dst{4 + i * 8, 136, 5, 5};
            SDL_RenderFillRect(renderer, &dst);
        }
    }
    std::string themeLabel = "[" + theme.displayName + "]";
    BitmapFont::drawText(
        renderer,
        themeLabel,
        config_.nativeWidth - BitmapFont::measureWidth(themeLabel) - 4,
        136,
        theme.light
    );

    if (stageIntro_) {
        BitmapFont::drawText(renderer, "STAGE " + std::to_string(stage_), 0, 64, theme.lightest, 1, true);
        BitmapFont::drawText(renderer, "READY", 0, 74, theme.light, 1, true);
    } else if (gameOver_) {
        BitmapFont::drawText(renderer, "GAME OVER", 0, 60, theme.lightest, 1, true);
        BitmapFont::drawText(renderer, "SCORE " + std::to_string(score_), 0, 70, theme.light, 1, true);
        BitmapFont::drawText(renderer, "PRESS R / START", 0, 82, theme.light, 1, true);
    }
}

void DinamoGame::syncLeaderboardAsync(bool submitCurrentScore) {
    const char* envUrl = std::getenv("ELEMENTS_SERVER_URL");
    if (!envUrl && !submitCurrentScore) {
        // Only auto-poll on boot if server URL is explicitly set, or try localhost cleanly
        envUrl = "http://127.0.0.1:8000";
    }
    std::string serverUrl = envUrl ? envUrl : "http://127.0.0.1:8000";
    int scoreSnapshot = score_;
    int stageSnapshot = stage_;

    std::lock_guard<std::mutex> lock(netMutex_);
    netThreads_.emplace_back([this, serverUrl, submitCurrentScore, scoreSnapshot, stageSnapshot]() {
        auto [host, port] = parseServerUrl(serverUrl);
        httplib::Client cli(host, port);
        cli.set_timeout_sec(1);

        if (submitCurrentScore && scoreSnapshot > 0) {
#if defined(__linux__) && (defined(__aarch64__) || defined(__arm__))
            const char* platform = "eos-runtime";
#elif defined(__linux__)
            const char* platform = "steamdeck";
#else
            const char* platform = "sdl2";
#endif
            std::ostringstream payload;
            payload << "{\"player\":\"SDL\",\"score\":" << scoreSnapshot
                    << ",\"stage\":" << stageSnapshot
                    << ",\"platform\":\"" << platform << "\"}";
            cli.Post("/api/leaderboard", payload.str(), "application/json");
        }

        auto res = cli.Get("/api/leaderboard?limit=1");
        if (res && res->status == 200) {
            std::string err;
            Json doc = Json::parse(res->body, err);
            if (err.empty() && doc["scores"].is_array() && !doc["scores"].array_items().empty()) {
                int topScore = doc["scores"].array_items()[0]["score"].int_value();
                if (topScore > remoteHighScore_.load()) {
                    remoteHighScore_.store(topScore);
                }
            }
        }
    });
}

} // namespace dinamo
