#include "jammer_app.h"

static const char* jamming_modes[] = {
    "OOK 650kHz",
    "2FSK 2.38kHz",
    "2FSK 47.6kHz",
    "MSK 99.97Kb/s",
    "GFSK 9.99Kb/s",
    "Bruteforce 0xFF"
};

typedef struct {
    uint32_t min;
    uint32_t max;
} FrequencyBand;

static const FrequencyBand valid_frequency_bands[] = {
    {300000000, 348000000},  // Band 1
    {387000000, 464000000},  // Band 2
    {779000000, 928000000}   // Band 3
};

#define NUM_FREQUENCY_BANDS (sizeof(valid_frequency_bands) / sizeof(valid_frequency_bands[0]))

static bool is_frequency_valid(uint32_t frequency) {
    for(size_t i = 0; i < NUM_FREQUENCY_BANDS; i++) {
        if(frequency >= valid_frequency_bands[i].min && frequency <= valid_frequency_bands[i].max) {
            return true;
        }
    }
    return false;
}

static uint32_t adjust_frequency_to_valid(uint32_t frequency, bool up) {
    if(is_frequency_valid(frequency)) {
        return frequency;
    } else {
        if(up) {
            for(size_t i = 0; i < NUM_FREQUENCY_BANDS; i++) {
                if(frequency < valid_frequency_bands[i].min) {
                    return valid_frequency_bands[i].min;
                }
            }
            return valid_frequency_bands[0].min;
        } else {
            for(int i = NUM_FREQUENCY_BANDS - 1; i >= 0; i--) {
                if(frequency > valid_frequency_bands[i].max) {
                    return valid_frequency_bands[i].max;
                }
            }
            return valid_frequency_bands[NUM_FREQUENCY_BANDS - 1].max;
        }
    }
}

static void jammer_draw_callback(Canvas* canvas, void* context) {
    JammerApp* app = (JammerApp*)context;
    canvas_clear(canvas);

    char freq_str[20];
    snprintf(freq_str, sizeof(freq_str), "%3lu.%02lu",
             app->frequency / 1000000,
             (app->frequency % 1000000) / 10000);

    int total_width = strlen(freq_str) * 12;
    int start_x = (128 - total_width) / 2;
    int digit_position = 0;

    for(size_t i = 0; i < strlen(freq_str); i++) {
        bool highlight = (digit_position == app->cursor_position);

        if(freq_str[i] != '.') {
            canvas_set_font(canvas, highlight ? FontBigNumbers : FontPrimary);
            char temp[2] = {freq_str[i], '\0'};
            canvas_draw_str_aligned(canvas, start_x + (i * 12), 10, AlignCenter, AlignTop, temp);
            digit_position++;
        } else {
            canvas_set_font(canvas, FontPrimary);
            char temp[2] = {freq_str[i], '\0'};
            canvas_draw_str_aligned(canvas, start_x + (i * 12), 10, AlignCenter, AlignTop, temp);
        }
    }

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 55, AlignCenter, AlignTop, jamming_modes[app->jamming_mode]);
}

static void jammer_input_callback(InputEvent* input_event, void* context) {
    JammerApp* app = (JammerApp*)context;
    furi_message_queue_put(app->event_queue, input_event, FuriWaitForever);
}

static void jammer_adjust_frequency(JammerApp* app, bool up) {
    uint32_t frequency = app->frequency;

    if(app->cursor_position == 3) {
        return;
    }

    if(app->cursor_position == 0) {
        uint8_t valid_hundreds[] = {3, 4, 7, 8, 9};
        int num_valid = sizeof(valid_hundreds) / sizeof(valid_hundreds[0]);
        uint8_t current_hundred = frequency / 100000000;
        int index = -1;
        for(int i = 0; i < num_valid; i++) {
            if(valid_hundreds[i] == current_hundred) {
                index = i;
                break;
            }
        }
        if(index == -1) {
            index = up ? 0 : num_valid - 1;
        } else {
            index = up ? (index + 1) % num_valid : (index - 1 + num_valid) % num_valid;
        }
        uint8_t new_hundred = valid_hundreds[index];
        frequency = (frequency % 100000000) + new_hundred * 100000000;
    } else {
        uint32_t step_sizes[] = {100000000, 10000000, 1000000, 10000, 1000};
        int step_index = app->cursor_position;
        if(app->cursor_position > 3) {
            step_index = app->cursor_position - 1;
        }
        uint32_t step = step_sizes[step_index];
        frequency = up ? frequency + step : frequency - step;

        if(frequency > SUBGHZ_FREQUENCY_MAX) {
            frequency = SUBGHZ_FREQUENCY_MIN + (frequency - SUBGHZ_FREQUENCY_MAX - 1);
        } else if(frequency < SUBGHZ_FREQUENCY_MIN) {
            frequency = SUBGHZ_FREQUENCY_MAX - (SUBGHZ_FREQUENCY_MIN - frequency - 1);
        }
    }

    frequency = adjust_frequency_to_valid(frequency, up);

    app->frequency = frequency;

    if(app->tx_running) {
        subghz_tx_rx_worker_stop(app->subghz_txrx);
        subghz_tx_rx_worker_start(app->subghz_txrx, app->device, app->frequency);
    }
}

static int32_t jammer_tx_thread(void* context) {
    JammerApp* app = context;
    uint8_t jam_data[MESSAGE_MAX_LEN];
    memset(jam_data, 0xFF, sizeof(jam_data));

    while(app->tx_running) {
        while(!subghz_tx_rx_worker_write(app->subghz_txrx, jam_data, sizeof(jam_data))) {
            furi_delay_ms(20);
        }
        furi_delay_ms(10);
    }

    return 0;
}

static void jammer_switch_mode(JammerApp* app) {
    app->tx_running = false;

    if(app->tx_thread) {
        furi_thread_join(app->tx_thread);
        furi_thread_free(app->tx_thread);
        app->tx_thread = NULL;
    }

    if(subghz_tx_rx_worker_is_running(app->subghz_txrx)) {
        subghz_tx_rx_worker_stop(app->subghz_txrx);
    }

    app->jamming_mode = (app->jamming_mode + 1) % JammerModeCount;

    switch(app->jamming_mode) {
        case JammerModeOok650Async:
            subghz_devices_load_preset(app->device, FuriHalSubGhzPresetOok650Async, NULL);
            break;
        case JammerMode2FSKDev238Async:
            subghz_devices_load_preset(app->device, FuriHalSubGhzPreset2FSKDev238Async, NULL);
            break;
        case JammerMode2FSKDev476Async:
            subghz_devices_load_preset(app->device, FuriHalSubGhzPreset2FSKDev476Async, NULL);
            break;
        case JammerModeMSK99_97KbAsync:
            subghz_devices_load_preset(app->device, FuriHalSubGhzPresetMSK99_97KbAsync, NULL);
            break;
        case JammerModeGFSK9_99KbAsync:
            subghz_devices_load_preset(app->device, FuriHalSubGhzPresetGFSK9_99KbAsync, NULL);
            break;
        case JammerModeBruteforce:
            subghz_devices_load_preset(app->device, FuriHalSubGhzPresetOok650Async, NULL);
            break;
        default:
            return;
    }

    subghz_tx_rx_worker_start(app->subghz_txrx, app->device, app->frequency);

    app->tx_running = true;
    app->tx_thread = furi_thread_alloc();
    furi_thread_set_name(app->tx_thread, "Jammer TX");
    furi_thread_set_stack_size(app->tx_thread, 2048);
    furi_thread_set_context(app->tx_thread, app);
    furi_thread_set_callback(app->tx_thread, jammer_tx_thread);
    furi_thread_start(app->tx_thread);
}

static void jammer_update_view(JammerApp* app) {
    view_port_update(app->view_port);
}

static bool jammer_init_subghz(JammerApp* app) {
    app->device = subghz_devices_get_by_name(SUBGHZ_DEVICE_NAME);
    if(!app->device) {
        return false;
    }

    subghz_devices_load_preset(app->device, FuriHalSubGhzPresetOok650Async, NULL);

    if(!subghz_tx_rx_worker_start(app->subghz_txrx, app->device, app->frequency)) {
        return false;
    }

    return true;
}

static void jammer_splash_screen_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignTop, "RF Jammer");
    canvas_draw_str_aligned(canvas, 64, 35, AlignCenter, AlignTop, "by RocketGod");
    canvas_draw_frame(canvas, 0, 0, 128, 64);
}

static void jammer_show_splash_screen(JammerApp* app) {
    view_port_draw_callback_set(app->view_port, jammer_splash_screen_draw_callback, app);
    view_port_update(app->view_port);
    furi_delay_ms(2000);
    view_port_draw_callback_set(app->view_port, jammer_draw_callback, app);
}

JammerApp* jammer_app_alloc(void) {
    JammerApp* app = malloc(sizeof(JammerApp));
    if(!app) {
        return NULL;
    }

    app->view_port = view_port_alloc();
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    app->frequency = 315000000;
    app->cursor_position = 0;
    app->running = true;
    app->tx_running = false;
    app->jamming_mode = JammerModeOok650Async;
    app->gui = furi_record_open(RECORD_GUI);

    view_port_draw_callback_set(app->view_port, jammer_draw_callback, app);
    view_port_input_callback_set(app->view_port, jammer_input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->tx_thread = NULL;
    app->device = NULL;

    subghz_devices_init();
    app->subghz_txrx = subghz_tx_rx_worker_alloc();

    furi_hal_power_suppress_charge_enter();

    return app;
}

void jammer_app_free(JammerApp* app) {
    app->tx_running = false;
    if(app->tx_thread) {
        furi_thread_join(app->tx_thread);
        furi_thread_free(app->tx_thread);
        app->tx_thread = NULL;
    }

    if(subghz_tx_rx_worker_is_running(app->subghz_txrx)) {
        subghz_tx_rx_worker_stop(app->subghz_txrx);
    }
    subghz_tx_rx_worker_free(app->subghz_txrx);
    subghz_devices_deinit();

    furi_hal_power_suppress_charge_exit();

    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);

    free(app);
}

int32_t jammer_app(void* p) {
    UNUSED(p);

    JammerApp* app = jammer_app_alloc();
    if(!app) {
        return -1;
    }

    jammer_show_splash_screen(app);

    if(!jammer_init_subghz(app)) {
        jammer_app_free(app);
        return -1;
    }

    InputEvent event;
    while(app->running) {
        if(furi_message_queue_get(app->event_queue, &event, 10) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                switch(event.key) {
                    case InputKeyOk:
                        jammer_switch_mode(app);
                        jammer_update_view(app);
                        break;
                    case InputKeyBack:
                        app->running = false;
                        break;
                    case InputKeyRight:
                        if(app->cursor_position < 5) {
                            app->cursor_position++;
                            if(app->cursor_position == 3) app->cursor_position++;
                            jammer_update_view(app);
                        }
                        break;
                    case InputKeyLeft:
                        if(app->cursor_position > 0) {
                            app->cursor_position--;
                            if(app->cursor_position == 3) app->cursor_position--;
                            jammer_update_view(app);
                        }
                        break;
                    case InputKeyUp:
                        jammer_adjust_frequency(app, true);
                        jammer_update_view(app);
                        break;
                    case InputKeyDown:
                        jammer_adjust_frequency(app, false);
                        jammer_update_view(app);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    jammer_app_free(app);

    return 0;
}
