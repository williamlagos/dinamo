package com.dinamo.game

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import kotlin.math.min

class DinamoView(context: Context) : SurfaceView(context), SurfaceHolder.Callback, Runnable {

    val engine = GameEngine(context)

    @Volatile
    private var isRunning = false
    private var gameThread: Thread? = null

    private val dstRect = Rect()
    private val renderPaint = Paint().apply {
        isFilterBitmap = false
        isDither = false
    }

    init {
        holder.addCallback(this)
        isFocusable = true
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        isRunning = true
        gameThread = Thread(this, "DinamoGameLoop")
        gameThread?.start()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Compute clean centered aspect-fit destination rectangle (10:9 aspect ratio)
        val scaleX = width.toFloat() / engine.nativeWidth
        val scaleY = height.toFloat() / engine.nativeHeight
        val scale = min(scaleX, scaleY)

        val targetW = (engine.nativeWidth * scale).toInt()
        val targetH = (engine.nativeHeight * scale).toInt()

        val left = (width - targetW) / 2
        val top = (height - targetH) / 2

        dstRect.set(left, top, left + targetW, top + targetH)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        isRunning = false
        try {
            gameThread?.join(500)
        } catch (e: InterruptedException) {
            // Thread join cleanup
        }
    }

    override fun run() {
        var lastTime = System.nanoTime()

        while (isRunning) {
            val now = System.nanoTime()
            val dt = ((now - lastTime) / 1_000_000_000.0f).coerceIn(0.001f, 0.05f)
            lastTime = now

            engine.update(dt)
            engine.render()

            drawFrame()

            // 60 FPS target delay
            val elapsedMs = (System.nanoTime() - now) / 1_000_000
            val sleepMs = 16 - elapsedMs
            if (sleepMs > 0) {
                try {
                    Thread.sleep(sleepMs)
                } catch (e: InterruptedException) {
                    break
                }
            }
        }
    }

    private fun drawFrame() {
        var canvas: Canvas? = null
        try {
            canvas = holder.lockCanvas()
            if (canvas != null) {
                // Background pillarbox/letterbox in deep black
                canvas.drawColor(Color.BLACK)

                // Render 160x144 virtual Game Boy canvas scaled crisply
                canvas.drawBitmap(engine.virtualBitmap, null, dstRect, renderPaint)
            }
        } finally {
            if (canvas != null) {
                holder.unlockCanvasAndPost(canvas)
            }
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val virtualX = (event.x - dstRect.left) / dstRect.width() * engine.nativeWidth
        val virtualY = (event.y - dstRect.top) / dstRect.height() * engine.nativeHeight

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (engine.isGameOver) {
                    engine.startNewGame()
                    return true
                }

                // Tap upper HUD area to switch palette
                if (virtualY < 24f) {
                    engine.cycleTheme()
                    return true
                }

                engine.playerX = virtualX.coerceIn(10f, engine.nativeWidth - 10f)
                engine.isFiring = true
            }

            MotionEvent.ACTION_MOVE -> {
                engine.playerX = virtualX.coerceIn(10f, engine.nativeWidth - 10f)
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                engine.isFiring = false
            }
        }
        return true
    }
}
