#include "sameboy_host.h"

#include <Core/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_BATCH_FRAMES 512

struct SameBoyHost {
    GB_gameboy_t *gb;
    DinamoHardwareModel model;
    uint32_t sample_rate;
    uint32_t pixels[160 * 144];

    GB_palette_t active_palette;
    bool has_custom_palette;

    char bootrom_dir[1024];

    DinamoAudioCallback audio_cb;
    void *audio_user_data;
    int16_t audio_batch[AUDIO_BATCH_FRAMES * 2];
    size_t audio_batch_frames;
};

static GB_model_t to_sameboy_model(DinamoHardwareModel model) {
    switch (model) {
        case DINAMO_HW_POCKET:
            return GB_MODEL_MGB;
        case DINAMO_HW_CGB:
            return GB_MODEL_CGB_E;
        case DINAMO_HW_DMG:
        default:
            return GB_MODEL_DMG_B;
    }
}

static GB_key_t to_sameboy_key(DinamoKey key) {
    switch (key) {
        case DINAMO_KEY_RIGHT:  return GB_KEY_RIGHT;
        case DINAMO_KEY_LEFT:   return GB_KEY_LEFT;
        case DINAMO_KEY_UP:     return GB_KEY_UP;
        case DINAMO_KEY_DOWN:   return GB_KEY_DOWN;
        case DINAMO_KEY_A:      return GB_KEY_A;
        case DINAMO_KEY_B:      return GB_KEY_B;
        case DINAMO_KEY_SELECT: return GB_KEY_SELECT;
        case DINAMO_KEY_START:  return GB_KEY_START;
        default:                return GB_KEY_A;
    }
}

static uint32_t encode_argb8888(GB_gameboy_t *gb, uint8_t r, uint8_t g, uint8_t b) {
    (void)gb;
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void on_apu_sample(GB_gameboy_t *gb, GB_sample_t *sample) {
    SameBoyHost *host = (SameBoyHost *)GB_get_user_data(gb);
    if (!host || !host->audio_cb) {
        return;
    }

    size_t idx = host->audio_batch_frames * 2;
    host->audio_batch[idx]     = sample->left;
    host->audio_batch[idx + 1] = sample->right;
    host->audio_batch_frames++;

    if (host->audio_batch_frames >= AUDIO_BATCH_FRAMES) {
        host->audio_cb(host->audio_batch, host->audio_batch_frames, host->audio_user_data);
        host->audio_batch_frames = 0;
    }
}

static void on_log_message(GB_gameboy_t *gb, const char *string, GB_log_attributes_t attributes) {
    (void)gb;
    if (attributes & GB_LOG_ERROR) {
        fprintf(stderr, "[SameBoy ERROR] %s", string);
    }
}

static void build_fast_boot_stub(uint8_t stub[0x100], GB_boot_rom_t type) {
    memset(stub, 0x00, 0x100);

    uint8_t boot_a = 0x01; // DMG
    if (type == GB_BOOT_ROM_MGB) {
        boot_a = 0xFF; // Game Boy Pocket
    } else if (type >= GB_BOOT_ROM_CGB_0) {
        boot_a = 0x11; // Game Boy Color
    }

    size_t p = 0;
    // ld sp, $fffe
    stub[p++] = 0x31; stub[p++] = 0xFE; stub[p++] = 0xFF;
    // ld a, $80 ; ldh ($26), a  (NR52 APU enable)
    stub[p++] = 0x3E; stub[p++] = 0x80; stub[p++] = 0xE0; stub[p++] = 0x26;
    // ld a, $fc ; ldh ($47), a  (BGP)
    stub[p++] = 0x3E; stub[p++] = 0xFC; stub[p++] = 0xE0; stub[p++] = 0x47;
    // ld a, $ff ; ldh ($48), a ; ldh ($49), a (OBP0/1)
    stub[p++] = 0x3E; stub[p++] = 0xFF;
    stub[p++] = 0xE0; stub[p++] = 0x48;
    stub[p++] = 0xE0; stub[p++] = 0x49;
    // xor a ; ldh ($42), a ; ldh ($43), a (SCY=0, SCX=0)
    stub[p++] = 0xAF;
    stub[p++] = 0xE0; stub[p++] = 0x42;
    stub[p++] = 0xE0; stub[p++] = 0x43;
    // ld a, $91 ; ldh ($40), a (LCDC enabled + BG enabled)
    stub[p++] = 0x3E; stub[p++] = 0x91; stub[p++] = 0xE0; stub[p++] = 0x40;

    if (type >= GB_BOOT_ROM_CGB_0) {
        // Initialize CGB BG palette 0 & OBJ palette 0/1 cleanly via BGPI/OBPI auto-inc
        static const uint8_t cgb_shades[8] = {
            0xFF, 0x7F, // lightest white
            0xB5, 0x56, // light gray
            0x4A, 0x29, // dark gray
            0x00, 0x00  // black
        };
        // BGPI = $80
        stub[p++] = 0x3E; stub[p++] = 0x80; stub[p++] = 0xE0; stub[p++] = 0x68;
        for (int i = 0; i < 8; i++) {
            stub[p++] = 0x3E; stub[p++] = cgb_shades[i]; stub[p++] = 0xE0; stub[p++] = 0x69;
        }
        // OBPI = $80 (set palette 0 and 1)
        stub[p++] = 0x3E; stub[p++] = 0x80; stub[p++] = 0xE0; stub[p++] = 0x6A;
        for (int pal = 0; pal < 2; pal++) {
            for (int i = 0; i < 8; i++) {
                stub[p++] = 0x3E; stub[p++] = cgb_shades[i]; stub[p++] = 0xE0; stub[p++] = 0x6B;
            }
        }
    }

    // ld hl, (boot_a << 8) | $B0 ; push hl ; pop af
    stub[p++] = 0x21; stub[p++] = 0xB0; stub[p++] = boot_a;
    stub[p++] = 0xE5;
    stub[p++] = 0xF1;
    // ld bc, $0013
    stub[p++] = 0x01; stub[p++] = 0x13; stub[p++] = 0x00;
    // ld de, $00d8
    stub[p++] = 0x11; stub[p++] = 0xD8; stub[p++] = 0x00;
    // ld hl, $014d
    stub[p++] = 0x21; stub[p++] = 0x4D; stub[p++] = 0x01;
    // jp $00fe
    stub[p++] = 0xC3; stub[p++] = 0xFE; stub[p++] = 0x00;

    // Right at 0x00FE: ldh ($50), a -> unmaps boot ROM and jumps straight to 0x0100
    stub[0xFE] = 0xE0;
    stub[0xFF] = 0x50;
}

static void on_boot_rom_load(GB_gameboy_t *gb, GB_boot_rom_t type) {
    SameBoyHost *host = (SameBoyHost *)GB_get_user_data(gb);
    if (host && host->bootrom_dir[0] != '\0') {
        const char *name = "dmg_boot.bin";
        if (type == GB_BOOT_ROM_MGB) {
            name = "mgb_boot.bin";
        } else if (type >= GB_BOOT_ROM_CGB_0) {
            name = "cgb_boot.bin";
        }

        char full_path[1200];
        snprintf(full_path, sizeof(full_path), "%s/%s", host->bootrom_dir, name);
        if (GB_load_boot_rom(gb, full_path) == 0) {
            return;
        }
    }

    uint8_t stub[0x100];
    build_fast_boot_stub(stub, type);
    GB_load_boot_rom_from_buffer(gb, stub, sizeof(stub));
}

static void configure_gb_callbacks(SameBoyHost *host) {
    GB_set_user_data(host->gb, host);
    GB_set_log_callback(host->gb, on_log_message);
    GB_set_boot_rom_load_callback(host->gb, on_boot_rom_load);
    GB_set_rgb_encode_callback(host->gb, encode_argb8888);
    GB_set_pixels_output(host->gb, host->pixels);
    GB_set_sample_rate(host->gb, host->sample_rate);
    GB_set_highpass_filter_mode(host->gb, GB_HIGHPASS_ACCURATE);
    GB_apu_set_sample_callback(host->gb, on_apu_sample);

    if (host->has_custom_palette) {
        GB_set_palette(host->gb, &host->active_palette);
    } else {
        GB_set_palette(host->gb, &GB_PALETTE_DMG);
    }
}

SameBoyHost *sb_host_create(DinamoHardwareModel model, uint32_t sample_rate) {
    SameBoyHost *host = (SameBoyHost *)calloc(1, sizeof(SameBoyHost));
    if (!host) {
        return NULL;
    }

    host->model = model;
    host->sample_rate = sample_rate > 0 ? sample_rate : 44100;
    host->gb = GB_alloc();
    if (!host->gb) {
        free(host);
        return NULL;
    }

    GB_init(host->gb, to_sameboy_model(model));
    configure_gb_callbacks(host);
    GB_reset(host->gb);
    return host;
}

void sb_host_destroy(SameBoyHost *host) {
    if (!host) {
        return;
    }
    if (host->gb) {
        GB_free(host->gb);
        GB_dealloc(host->gb);
        host->gb = NULL;
    }
    free(host);
}

void sb_host_set_bootrom_dir(SameBoyHost *host, const char *dir_path) {
    if (!host) return;
    if (!dir_path) {
        host->bootrom_dir[0] = '\0';
        return;
    }
    snprintf(host->bootrom_dir, sizeof(host->bootrom_dir), "%s", dir_path);
}

bool sb_host_load_rom(SameBoyHost *host, const char *rom_path) {
    if (!host || !host->gb || !rom_path) {
        return false;
    }
    int rc = GB_load_rom(host->gb, rom_path);
    if (rc == 0) {
        GB_reset(host->gb);
        return true;
    }
    return false;
}

void sb_host_switch_model(SameBoyHost *host, DinamoHardwareModel model) {
    if (!host || !host->gb) {
        return;
    }
    host->model = model;
    GB_switch_model_and_reset(host->gb, to_sameboy_model(model));
    configure_gb_callbacks(host);
}

void sb_host_set_palette(SameBoyHost *host, const DinamoRgbPalette *palette) {
    if (!host || !host->gb || !palette) {
        return;
    }
    for (int i = 0; i < 5; i++) {
        host->active_palette.colors[i].r = palette->colors[i][0];
        host->active_palette.colors[i].g = palette->colors[i][1];
        host->active_palette.colors[i].b = palette->colors[i][2];
    }
    host->has_custom_palette = true;
    GB_set_palette(host->gb, &host->active_palette);
}

void sb_host_set_audio_callback(SameBoyHost *host, DinamoAudioCallback cb, void *user_data) {
    if (!host) return;
    host->audio_cb = cb;
    host->audio_user_data = user_data;
}

void sb_host_set_key(SameBoyHost *host, DinamoKey key, bool pressed) {
    if (!host || !host->gb) return;
    GB_set_key_state(host->gb, to_sameboy_key(key), pressed);
}

bool sb_host_load_battery(SameBoyHost *host, const char *sav_path) {
    if (!host || !host->gb || !sav_path) return false;
    return GB_load_battery(host->gb, sav_path) == 0;
}

bool sb_host_save_battery(SameBoyHost *host, const char *sav_path) {
    if (!host || !host->gb || !sav_path) return false;
    if (GB_save_battery_size(host->gb) <= 0) {
        return true;
    }
    return GB_save_battery(host->gb, sav_path) == 0;
}

bool sb_host_is_battery_dirty(SameBoyHost *host) {
    if (!host || !host->gb) return false;
    return GB_get_battery_dirty(host->gb);
}

void sb_host_clear_battery_dirty(SameBoyHost *host) {
    if (!host || !host->gb) return;
    GB_clear_battery_dirty(host->gb);
}

const uint32_t *sb_host_run_frame(SameBoyHost *host) {
    if (!host || !host->gb) return NULL;
    GB_run_frame(host->gb);
    if (host->audio_batch_frames > 0 && host->audio_cb) {
        host->audio_cb(host->audio_batch, host->audio_batch_frames, host->audio_user_data);
        host->audio_batch_frames = 0;
    }
    return host->pixels;
}

const uint32_t *sb_host_pixels(const SameBoyHost *host) {
    if (!host) return NULL;
    return host->pixels;
}

void sb_host_rom_title(SameBoyHost *host, char *out_title_17) {
    if (!host || !host->gb || !out_title_17) return;
    GB_get_rom_title(host->gb, out_title_17);
}
