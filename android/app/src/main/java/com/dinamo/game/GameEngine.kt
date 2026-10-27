package com.dinamo.game

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Rect
import android.graphics.Typeface
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin
import kotlin.random.Random

enum class EnemyType { BEE, BUTTERFLY, BOSS }
enum class EnemyState { ENTERING, FORMATION, DIVING, RETURNING }

class Enemy(
    val type: EnemyType,
    val row: Int,
    val col: Int,
    var x: Float,
    var y: Float,
    var targetX: Float,
    var targetY: Float,
    var health: Int
) {
    var state = EnemyState.FORMATION
    var hasFired = false
    var diveProgress = 0f
    var diveStartX = 0f
    var diveStartY = 0f
    var diveTargetX = 0f
}

class Bullet(
    var x: Float,
    var y: Float,
    val vx: Float,
    val vy: Float,
    val isPlayer: Boolean
)

class Explosion(
    val x: Float,
    val y: Float,
    var frame: Int = 0,
    var timer: Float = 0f
)

class Star(
    var x: Float,
    var y: Float,
    val speed: Float,
    val colorIndex: Int
)

class GameEngine(private val context: Context) {
    val nativeWidth = 160
    val nativeHeight = 144

    val virtualBitmap: Bitmap = Bitmap.createBitmap(nativeWidth, nativeHeight, Bitmap.Config.ARGB_8888)
    private val canvas = Canvas(virtualBitmap)
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)

    val soundEngine = SoundEngine()
    var currentTheme = GameTheme.DMG
        private set

    // Game state
    var score = 0
    var highScore = 20000
    var lives = 3
    var stage = 1
    var isGameOver = false
    var isStageIntro = false
    var isPlayerDead = false
    private var respawnTimer = 0f
    private var invulnerableTimer = 0f

    // Controls
    var playerX = 80f
    var moveLeft = false
    var moveRight = false
    var isFiring = false
    private var fireCooldown = 0f

    // Armada & Animation
    private val enemies = mutableListOf<Enemy>()
    private val bullets = mutableListOf<Bullet>()
    private val explosions = mutableListOf<Explosion>()
    private val stars = mutableListOf<Star>()
    private var formationTime = 0f
    private var wingFrame = 1
    private var wingTimer = 0f
    private var diveTimer = 0f

    init {
        val prefs = context.getSharedPreferences("dinamo_prefs", Context.MODE_PRIVATE)
        highScore = prefs.getInt("high_score", 20000)

        paint.isFilterBitmap = false
        paint.typeface = Typeface.MONOSPACE

        setupStars()
        startNewGame()
    }

    fun setTheme(theme: GameTheme) {
        currentTheme = theme
        setupStars()
    }

    fun cycleTheme() {
        setTheme(currentTheme.next())
    }

    private fun setupStars() {
        stars.clear()
        val rnd = Random(42)
        for (i in 0 until 28) {
            stars.add(
                Star(
                    x = rnd.nextFloat() * nativeWidth,
                    y = rnd.nextFloat() * nativeHeight,
                    speed = 10f + rnd.nextFloat() * 25f,
                    colorIndex = i % 2
                )
            )
        }
    }

    fun startNewGame() {
        score = 0
        lives = 3
        stage = 1
        isGameOver = false
        isPlayerDead = false
        playerX = 80f

        enemies.clear()
        bullets.clear()
        explosions.clear()

        startStage(1)
    }

    private fun startStage(stageNum: Int) {
        stage = stageNum
        isStageIntro = true
        respawnTimer = 2.0f

        enemies.clear()
        bullets.clear()

        // 10-ship armada formation matching GB Studio project
        // Row 0: 2 Bosses (cols 2, 3)
        // Row 1: 4 Escorts (cols 1, 2, 3, 4)
        // Row 2: 4 Bees (cols 1, 2, 3, 4)
        val colSpacing = 16f
        val rowSpacing = 14f
        val startX = 40f
        val startY = 24f

        enemies.add(Enemy(EnemyType.BOSS, 0, 2, startX + 2 * colSpacing, startY, startX + 2 * colSpacing, startY, 2))
        enemies.add(Enemy(EnemyType.BOSS, 0, 3, startX + 3 * colSpacing, startY, startX + 3 * colSpacing, startY, 2))

        for (col in 1..4) {
            enemies.add(Enemy(EnemyType.BUTTERFLY, 1, col, startX + col * colSpacing, startY + rowSpacing, startX + col * colSpacing, startY + rowSpacing, 1))
            enemies.add(Enemy(EnemyType.BEE, 2, col, startX + col * colSpacing, startY + 2 * rowSpacing, startX + col * colSpacing, startY + 2 * rowSpacing, 1))
        }
    }

    fun update(dt: Float) {
        updateStars(dt)

        if (isGameOver) return

        if (isStageIntro) {
            respawnTimer -= dt
            if (respawnTimer <= 0) {
                isStageIntro = false
            }
            return
        }

        updatePlayer(dt)
        updateFormation(dt)
        updateProjectiles(dt)
        updateExplosions(dt)
        checkCollisions()
        checkStageProgress()
    }

    private fun updateStars(dt: Float) {
        for (star in stars) {
            star.y += star.speed * dt
            if (star.y > nativeHeight) {
                star.y = 0f
                star.x = (star.x + 37f) % nativeWidth
            }
        }
    }

    private fun updatePlayer(dt: Float) {
        if (isPlayerDead) {
            respawnTimer -= dt
            if (respawnTimer <= 0) {
                if (lives > 0) {
                    isPlayerDead = false
                    playerX = 80f
                    invulnerableTimer = 2.0f
                } else {
                    gameOver()
                }
            }
            return
        }

        if (invulnerableTimer > 0) {
            invulnerableTimer -= dt
        }

        val speed = 90f * dt
        if (moveLeft) playerX -= speed
        if (moveRight) playerX += speed
        playerX = max(10f, min(nativeWidth - 10f, playerX))

        if (fireCooldown > 0) fireCooldown -= dt
        if (isFiring && fireCooldown <= 0) {
            val playerBullets = bullets.count { it.isPlayer }
            if (playerBullets < 2) {
                firePlayerLaser()
                fireCooldown = 0.22f
            }
        }
    }

    private fun firePlayerLaser() {
        soundEngine.playLaser()
        bullets.add(Bullet(playerX, 126f, 0f, -180f, true))
    }

    private fun updateFormation(dt: Float) {
        formationTime += dt * 2.2f
        val swayX = sin(formationTime) * 6f

        wingTimer += dt
        if (wingTimer >= 0.25f) {
            wingTimer = 0f
            wingFrame = if (wingFrame == 1) 2 else 1
        }

        diveTimer += dt
        if (diveTimer >= 2.5f) {
            diveTimer = 0f
            triggerDiveBomb()
        }

        for (enemy in enemies) {
            if (enemy.state == EnemyState.FORMATION) {
                enemy.x = enemy.targetX + swayX
                enemy.y = enemy.targetY
            } else if (enemy.state == EnemyState.DIVING) {
                enemy.diveProgress += dt * 0.45f
                val p = enemy.diveProgress
                // Swooping Bézier curve down to playerX
                enemy.x = (1 - p) * enemy.diveStartX + p * enemy.diveTargetX + sin(p * 3.14f) * 20f
                enemy.y = enemy.diveStartY + p * (nativeHeight + 20f)

                if (!enemy.hasFired && enemy.y > 60f && enemy.y < 100f) {
                    enemy.hasFired = true
                    bullets.add(Bullet(enemy.x, enemy.y, 0f, 110f, false))
                }

                if (enemy.y > nativeHeight + 10f) {
                    enemy.state = EnemyState.RETURNING
                    enemy.y = -10f
                    enemy.x = enemy.targetX
                    enemy.hasFired = false
                }
            } else if (enemy.state == EnemyState.RETURNING) {
                enemy.y += dt * 70f
                if (enemy.y >= enemy.targetY) {
                    enemy.state = EnemyState.FORMATION
                }
            }
        }
    }

    private fun triggerDiveBomb() {
        val formationEnemies = enemies.filter { it.state == EnemyState.FORMATION }
        if (formationEnemies.isEmpty()) return

        val diver = formationEnemies.random()
        diver.state = EnemyState.DIVING
        diver.diveProgress = 0f
        diver.diveStartX = diver.x
        diver.diveStartY = diver.y
        diver.diveTargetX = playerX
        diver.hasFired = false
        soundEngine.playDive()
    }

    private fun updateProjectiles(dt: Float) {
        val it = bullets.iterator()
        while (it.hasNext()) {
            val b = it.next()
            b.x += b.vx * dt
            b.y += b.vy * dt
            if (b.y < -10f || b.y > nativeHeight + 10f) {
                it.remove()
            }
        }
    }

    private fun updateExplosions(dt: Float) {
        val it = explosions.iterator()
        while (it.hasNext()) {
            val exp = it.next()
            exp.timer += dt
            if (exp.timer >= 0.06f) {
                exp.timer = 0f
                exp.frame++
                if (exp.frame >= 4) {
                    it.remove()
                }
            }
        }
    }

    private fun checkCollisions() {
        // Player lasers vs enemies
        val bIt = bullets.iterator()
        while (bIt.hasNext()) {
            val b = bIt.next()
            if (!b.isPlayer) continue

            val eIt = enemies.iterator()
            while (eIt.hasNext()) {
                val enemy = eIt.next()
                if (hypot(b.x - enemy.x, b.y - enemy.y) < 10f) {
                    bIt.remove()
                    enemy.health--
                    if (enemy.health <= 0) {
                        soundEngine.playExplosion()
                        explosions.add(Explosion(enemy.x, enemy.y))
                        val pts = when (enemy.type) {
                            EnemyType.BEE -> if (enemy.state == EnemyState.DIVING) 100 else 50
                            EnemyType.BUTTERFLY -> if (enemy.state == EnemyState.DIVING) 160 else 80
                            EnemyType.BOSS -> if (enemy.state == EnemyState.DIVING) 400 else 150
                        }
                        addScore(pts)
                        eIt.remove()
                    } else {
                        soundEngine.playLaser()
                        addScore(50)
                    }
                    break
                }
            }
        }

        // Alien bullets & enemy collisions vs player
        if (isPlayerDead || invulnerableTimer > 0f) return

        val alienBullets = bullets.filter { !it.isPlayer }
        for (b in alienBullets) {
            if (hypot(b.x - playerX, b.y - 128f) < 8f) {
                bullets.remove(b)
                killPlayer()
                return
            }
        }

        for (enemy in enemies) {
            if (hypot(enemy.x - playerX, enemy.y - 128f) < 12f) {
                soundEngine.playExplosion()
                explosions.add(Explosion(enemy.x, enemy.y))
                enemies.remove(enemy)
                killPlayer()
                return
            }
        }
    }

    private fun killPlayer() {
        soundEngine.playPlayerDeath()
        explosions.add(Explosion(playerX, 128f))
        isPlayerDead = true
        lives--
        respawnTimer = 1.5f
    }

    private fun addScore(pts: Int) {
        score += pts
        if (score > highScore) {
            highScore = score
            val prefs = context.getSharedPreferences("dinamo_prefs", Context.MODE_PRIVATE)
            prefs.edit().putInt("high_score", highScore).apply()
        }
    }

    private fun checkStageProgress() {
        if (enemies.isEmpty() && !isStageIntro && !isGameOver) {
            startStage(stage + 1)
        }
    }

    private fun gameOver() {
        isGameOver = true
    }

    // --- Rendering in 160x144 Virtual Resolution ---
    fun render() {
        val colors = Palettes.forTheme(currentTheme)
        canvas.drawColor(colors.background)

        // Render Stars
        for (star in stars) {
            paint.color = colors.starColors[star.colorIndex % colors.starColors.size]
            canvas.drawRect(star.x, star.y, star.x + 1f, star.y + 1f, paint)
        }

        // Render Enemies (16x16 pixel sprites)
        for (enemy in enemies) {
            renderEnemy(enemy, colors)
        }

        // Render Bullets
        for (b in bullets) {
            paint.color = if (b.isPlayer) colors.lightest else colors.light
            canvas.drawRect(b.x - 1f, b.y - 3f, b.x + 1f, b.y + 3f, paint)
        }

        // Render Explosions
        for (exp in explosions) {
            renderExplosion(exp, colors)
        }

        // Render Player Ship (16x16)
        if (!isPlayerDead) {
            val alpha = if (invulnerableTimer > 0f && (invulnerableTimer * 8).toInt() % 2 == 0) 100 else 255
            renderPlayer(playerX, 128f, colors, alpha)
        }

        // Render HUD
        renderHUD(colors)
    }

    private fun renderPlayer(x: Float, y: Float, colors: ThemeColors, alpha: Int = 255) {
        paint.alpha = alpha

        // Body (Lightest)
        paint.color = colors.lightest
        canvas.drawRect(x - 1f, y - 6f, x + 1f, y + 6f, paint)
        canvas.drawRect(x - 3f, y - 2f, x + 3f, y + 4f, paint)
        canvas.drawRect(x - 5f, y + 2f, x + 5f, y + 5f, paint)

        // Wings / Cannons (Dark)
        paint.color = colors.dark
        canvas.drawRect(x - 6f, y + 4f, x - 4f, y + 7f, paint)
        canvas.drawRect(x + 4f, y + 4f, x + 6f, y + 7f, paint)
        canvas.drawRect(x - 2f, y + 5f, x + 2f, y + 7f, paint)

        // Wing highlights (Light)
        paint.color = colors.light
        canvas.drawRect(x - 6f, y + 1f, x - 4f, y + 3f, paint)
        canvas.drawRect(x + 4f, y + 1f, x + 6f, y + 3f, paint)

        paint.alpha = 255
    }

    private fun renderEnemy(enemy: Enemy, colors: ThemeColors) {
        val x = enemy.x
        val y = enemy.y
        val isFlap = wingFrame == 2

        when (enemy.type) {
            EnemyType.BOSS -> {
                // Boss Drone (Green/Yellow)
                paint.color = if (enemy.health > 1) colors.light else colors.dark
                canvas.drawRect(x - 5f, y - 4f, x + 5f, y + 4f, paint)
                paint.color = colors.lightest
                canvas.drawRect(x - 2f, y - 2f, x + 2f, y + 2f, paint)
                paint.color = colors.dark
                val wingY = if (isFlap) y - 2f else y + 2f
                canvas.drawRect(x - 7f, wingY, x - 5f, wingY + 3f, paint)
                canvas.drawRect(x + 5f, wingY, x + 7f, wingY + 3f, paint)
            }
            EnemyType.BUTTERFLY -> {
                // Red Escort
                paint.color = colors.dark
                canvas.drawRect(x - 4f, y - 3f, x + 4f, y + 3f, paint)
                paint.color = colors.lightest
                canvas.drawRect(x - 1f, y - 1f, x + 1f, y + 1f, paint)
                paint.color = colors.light
                val wingSpan = if (isFlap) 6f else 5f
                canvas.drawRect(x - wingSpan, y - 2f, x - 4f, y + 2f, paint)
                canvas.drawRect(x + 4f, y - 2f, x + wingSpan, y + 2f, paint)
            }
            EnemyType.BEE -> {
                // Blue Vanguard Bee
                paint.color = colors.light
                canvas.drawRect(x - 3f, y - 3f, x + 3f, y + 3f, paint)
                paint.color = colors.lightest
                canvas.drawRect(x - 1f, y - 2f, x + 1f, y, paint)
                paint.color = colors.dark
                val wingOff = if (isFlap) 1f else -1f
                canvas.drawRect(x - 5f, y + wingOff, x - 3f, y + wingOff + 2f, paint)
                canvas.drawRect(x + 3f, y + wingOff, x + 5f, y + wingOff + 2f, paint)
            }
        }
    }

    private fun renderExplosion(exp: Explosion, colors: ThemeColors) {
        val r = (exp.frame + 1) * 2f
        paint.color = if (exp.frame % 2 == 0) colors.lightest else colors.light
        canvas.drawCircle(exp.x, exp.y, r, paint)
    }

    private fun renderHUD(colors: ThemeColors) {
        paint.textSize = 6f
        paint.color = colors.hudPrimary

        // 1UP & Score
        canvas.drawText("1UP", 6f, 8f, paint)
        canvas.drawText(String.format("%02d", score), 6f, 16f, paint)

        // HIGH Score
        canvas.drawText("HIGH", 64f, 8f, paint)
        canvas.drawText("$highScore", 64f, 16f, paint)

        // Stage
        paint.color = colors.hudSecondary
        canvas.drawText("ST $stage", 126f, 16f, paint)

        // Theme indicator
        paint.textSize = 5f
        canvas.drawText("[${currentTheme.displayName}]", 126f, 138f, paint)

        // Life icons
        for (i in 0 until max(0, lives - 1)) {
            renderPlayer(12f + i * 10f, 138f, colors)
        }

        // Overlay text
        if (isStageIntro) {
            paint.textSize = 8f
            paint.color = colors.hudPrimary
            val msg = "STAGE $stage"
            val w = paint.measureText(msg)
            canvas.drawText(msg, (nativeWidth - w) / 2f, 65f, paint)
            val ready = "READY"
            val rw = paint.measureText(ready)
            canvas.drawText(ready, (nativeWidth - rw) / 2f, 78f, paint)
        } else if (isGameOver) {
            paint.textSize = 8f
            paint.color = colors.hudPrimary
            val msg = "GAME OVER"
            val w = paint.measureText(msg)
            canvas.drawText(msg, (nativeWidth - w) / 2f, 65f, paint)
            paint.textSize = 6f
            val sc = "SCORE: $score"
            val sw = paint.measureText(sc)
            canvas.drawText(sc, (nativeWidth - sw) / 2f, 78f, paint)
            val restart = "TAP TO PLAY"
            val rw = paint.measureText(restart)
            canvas.drawText(restart, (nativeWidth - rw) / 2f, 90f, paint)
        }
    }
}
