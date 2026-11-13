#ifndef DINAMO_SAMEBOY_HOST_H
#define DINAMO_SAMEBOY_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DINAMO_HW_DMG = 0,
    DINAMO_HW_POCKET = 1,
    DINAMO_HW_CGB = 2
} DinamoHardwareModel;

typedef enum {
    DINAMO_KEY_RIGHT = 0,
    DINAMO_KEY_LEFT,
    DINAMO_KEY_UP,
    DINAMO_KEY_DOWN,
    DINAMO_KEY_A,
    DINAMO_KEY_B,
    DINAMO_KEY_SELECT,
    DINAMO_KEY_START
} DinamoKey;

typedef struct {
    // 0: darkest, 1: dark, 2: light, 3: lightest, 4: lcd_off
    uint8_t colors[5][3];
} DinamoRgbPalette;

typedef void (*DinamoAudioCallback)(const int16_t *samples, size_t stereo_frames, void *user_data);

typedef struct SameBoyHost SameBoyHost;

SameBoyHost *sb_host_create(DinamoHardwareModel model, uint32_t sample_rate);
void sb_host_destroy(SameBoyHost *host);

void sb_host_set_bootrom_dir(SameBoyHost *host, const char *dir_path);
bool sb_host_load_rom(SameBoyHost *host, const char *rom_path);
void sb_host_switch_model(SameBoyHost *host, DinamoHardwareModel model);

void sb_host_set_palette(SameBoyHost *host, const DinamoRgbPalette *palette);
void sb_host_set_audio_callback(SameBoyHost *host, DinamoAudioCallback cb, void *user_data);
void sb_host_set_key(SameBoyHost *host, DinamoKey key, bool pressed);

bool sb_host_load_battery(SameBoyHost *host, const char *sav_path);
bool sb_host_save_battery(SameBoyHost *host, const char *sav_path);
bool sb_host_is_battery_dirty(SameBoyHost *host);
void sb_host_clear_battery_dirty(SameBoyHost *host);

const uint32_t *sb_host_run_frame(SameBoyHost *host);
const uint32_t *sb_host_pixels(const SameBoyHost *host);
void sb_host_rom_title(SameBoyHost *host, char *out_title_17);

#ifdef __cplusplus
}
#endif

#endif // DINAMO_SAMEBOY_HOST_H
