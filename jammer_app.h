#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <subghz/devices/devices.h>
#include <subghz/devices/preset.h>
#include <furi/core/log.h>
#include <furi_hal.h>
#include <lib/subghz/subghz_tx_rx_worker.h>

#define TAG "JammerApp"
#define SUBGHZ_DEVICE_NAME "cc1101_int"
#define SUBGHZ_FREQUENCY_MIN 300000000
#define SUBGHZ_FREQUENCY_MAX 928000000
#define MESSAGE_MAX_LEN 256

// Jamming Modes
typedef enum {
    JammerModeOok650Async,
    JammerMode2FSKDev238Async,
    JammerMode2FSKDev476Async,
    JammerModeMSK99_97KbAsync,
    JammerModeGFSK9_99KbAsync,
    JammerModeBruteforce,
    JammerModeCount
} JammerMode;

// App State (Splash or Main App)
typedef enum {
    JammerStateSplash,  // Splash Screen State
    JammerStateMain     // Main App State
} JammerState;

// Main App Struct
typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    uint32_t frequency;
    uint8_t cursor_position;
    bool running;
    JammerMode jamming_mode;
    const SubGhzDevice* device;
    SubGhzTxRxWorker* subghz_txrx;
    FuriThread* tx_thread;
    bool tx_running;
    JammerState state;  // Add state to handle Splash and Main App logic
} JammerApp;

JammerApp* jammer_app_alloc(void);
void jammer_app_free(JammerApp* app);
int32_t jammer_app(void* p);
