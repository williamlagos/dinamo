package com.dinamo.game

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import java.util.concurrent.Executors
import kotlin.math.PI
import kotlin.math.max
import kotlin.math.sin

class SoundEngine {
    private val executor = Executors.newSingleThreadExecutor()
    private val sampleRate = 44100
    var isMuted = false

    fun playTone(freqStart: Double, freqEnd: Double, durationSec: Double, isNoise: Boolean = false) {
        if (isMuted) return

        executor.execute {
            try {
                val numSamples = (sampleRate * durationSec).toInt()
                val buffer = ShortArray(numSamples)

                if (isNoise) {
                    // Game Boy Channel 4 7-bit LFSR pseudo-random noise
                    var lfsr = 0x7fff
                    for (i in 0 until numSamples) {
                        val progress = i.toDouble() / numSamples
                        val envelope = max(0.0, 1.0 - progress)
                        val bit = (lfsr and 1) xor ((lfsr shr 1) and 1)
                        lfsr = (lfsr shr 1) or (bit shl 14)
                        lfsr = (lfsr and 0x40.inv()) or (bit shl 6)
                        val sample = if ((lfsr and 1) != 0) 1.0 else -1.0
                        buffer[i] = (sample * envelope * 12000.0).toInt().toShort()
                    }
                } else {
                    // Pulse square wave (Channel 1 & 2)
                    var phase = 0.0
                    for (i in 0 until numSamples) {
                        val progress = i.toDouble() / numSamples
                        val freq = freqStart + (freqEnd - freqStart) * progress
                        val phaseInc = 2.0 * PI * freq / sampleRate
                        phase += phaseInc
                        if (phase > 2.0 * PI) phase -= 2.0 * PI

                        val wave = if (sin(phase) > 0.0) 1.0 else -1.0
                        val envelope = max(0.0, 1.0 - progress * 1.1)
                        buffer[i] = (wave * envelope * 10000.0).toInt().toShort()
                    }
                }

                val track = AudioTrack.Builder()
                    .setAudioAttributes(
                        AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_GAME)
                            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                            .build()
                    )
                    .setAudioFormat(
                        AudioFormat.Builder()
                            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                            .setSampleRate(sampleRate)
                            .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                            .build()
                    )
                    .setBufferSizeInBytes(buffer.size * 2)
                    .setTransferMode(AudioTrack.MODE_STATIC)
                    .build()

                track.write(buffer, 0, buffer.size)
                track.play()

                // Release track after playback completes
                Thread.sleep((durationSec * 1000).toLong() + 50)
                track.stop()
                track.release()
            } catch (e: Exception) {
                // Ignore audio playback exceptions gracefully
            }
        }
    }

    fun playLaser() {
        playTone(880.0, 220.0, 0.10, isNoise = false)
    }

    fun playDive() {
        playTone(520.0, 260.0, 0.35, isNoise = false)
    }

    fun playExplosion() {
        playTone(0.0, 0.0, 0.22, isNoise = true)
    }

    fun playPlayerDeath() {
        playTone(0.0, 0.0, 0.45, isNoise = true)
    }
}
