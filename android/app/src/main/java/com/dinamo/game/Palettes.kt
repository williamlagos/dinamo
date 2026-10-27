package com.dinamo.game

import android.graphics.Color

enum class GameTheme(val displayName: String) {
    DMG("DMG"),
    POCKET("Pocket"),
    GBC("GBC"),
    ARCADE("Arcade");

    fun next(): GameTheme {
        val values = values()
        return values[(ordinal + 1) % values.size]
    }
}

data class ThemeColors(
    val background: Int,
    val lightest: Int,
    val light: Int,
    val dark: Int,
    val darkest: Int,
    val starColors: List<Int>,
    val hudPrimary: Int,
    val hudSecondary: Int
)

object Palettes {
    val dmg = ThemeColors(
        background = Color.rgb(8, 24, 32),
        lightest = Color.rgb(224, 248, 208),
        light = Color.rgb(136, 192, 112),
        dark = Color.rgb(52, 104, 86),
        darkest = Color.rgb(8, 24, 32),
        starColors = listOf(Color.rgb(136, 192, 112), Color.rgb(224, 248, 208)),
        hudPrimary = Color.rgb(224, 248, 208),
        hudSecondary = Color.rgb(136, 192, 112)
    )

    val pocket = ThemeColors(
        background = Color.rgb(0, 0, 0),
        lightest = Color.rgb(255, 255, 255),
        light = Color.rgb(170, 170, 170),
        dark = Color.rgb(85, 85, 85),
        darkest = Color.rgb(0, 0, 0),
        starColors = listOf(Color.rgb(170, 170, 170), Color.rgb(255, 255, 255)),
        hudPrimary = Color.rgb(255, 255, 255),
        hudSecondary = Color.rgb(170, 170, 170)
    )

    val gbc = ThemeColors(
        background = Color.rgb(8, 16, 32),
        lightest = Color.rgb(255, 255, 255),
        light = Color.rgb(0, 232, 216),
        dark = Color.rgb(216, 40, 0),
        darkest = Color.rgb(16, 16, 32),
        starColors = listOf(Color.rgb(248, 216, 0), Color.rgb(0, 232, 216), Color.rgb(255, 255, 255)),
        hudPrimary = Color.rgb(248, 216, 0),
        hudSecondary = Color.rgb(0, 232, 216)
    )

    val arcade = ThemeColors(
        background = Color.rgb(5, 5, 13),
        lightest = Color.rgb(255, 255, 255),
        light = Color.rgb(34, 119, 255),
        dark = Color.rgb(255, 34, 34),
        darkest = Color.rgb(20, 20, 20),
        starColors = listOf(Color.WHITE, Color.CYAN, Color.YELLOW, Color.RED),
        hudPrimary = Color.WHITE,
        hudSecondary = Color.RED
    )

    fun forTheme(theme: GameTheme): ThemeColors {
        return when (theme) {
            GameTheme.DMG -> dmg
            GameTheme.POCKET -> pocket
            GameTheme.GBC -> gbc
            GameTheme.ARCADE -> arcade
        }
    }
}
