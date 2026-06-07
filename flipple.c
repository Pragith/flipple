#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <string.h>

#include "flipple_engine.h"

#define TAG           "Flipple"
#define STATS_FILE    "stats.txt"
#define WORDLIST_FILE "wordlist.txt"
#define REEL_LENGTH   9

const char* word_list[] = {"APPLE", "BERRY", "CHAIR", "DANCE", "EAGLE", "FLAME", "GRAPE", "HOUSE",
                           "IMAGE", "JUICE", "KNIFE", "LEMON", "MOUSE", "NIGHT", "OCEAN", "PIZZA",
                           "QUEEN", "RIVER", "SNAKE", "TRAIN", "UNCLE", "VOICE", "WATER", "XENON",
                           "YACHT", "ZEBRA", "GHOST", "BLAME", "CRISP", "DRIVE", "EMPTY", "FORCE",
                           "GUEST", "HEART", "INDEX", "JOLLY", "KNEEL", "LARGE", "MAGIC", "NOVEL",
                           "ONION", "PULSE", "QUOTE", "ROUND", "SUGAR"};
#define WORD_COUNT (sizeof(word_list) / sizeof(word_list[0]))

typedef struct {
    char reels[FLIPPLE_WORD_LENGTH][REEL_LENGTH];
    uint8_t selected[FLIPPLE_WORD_LENGTH];
    uint8_t feedback[FLIPPLE_WORD_LENGTH];
    uint8_t left_span[FLIPPLE_WORD_LENGTH];
    uint8_t right_span[FLIPPLE_WORD_LENGTH];
    uint8_t active_row;
    uint8_t keys_left;
    char target[FLIPPLE_WORD_LENGTH + 1];
    bool game_over;
    bool win;
    FuriMutex* mutex;
    Storage* storage;
    FlippleStats stats;
    bool stats_dirty;
} FlippleModel;

typedef struct {
    FlippleModel* model;
    ViewPort* view_port;
    Gui* gui;
    FuriMessageQueue* event_queue;
    char (*runtime_word_list)[FLIPPLE_WORD_LENGTH + 1];
    size_t runtime_word_count;
} FlippleApp;

static const char* stats_path(void) {
    return APP_DATA_PATH(STATS_FILE);
}

static const char* wordlist_path(void) {
    return APP_ASSETS_PATH(WORDLIST_FILE);
}

static const int8_t reel_offsets[REEL_LENGTH] = {-12, -7, -4, -1, 0, 2, 5, 9, 13};

static void draw_reel_tile(
    Canvas* canvas,
    int x,
    int y,
    char ch,
    bool centered,
    bool active,
    uint8_t feedback) {
    const int tile_w = 13;
    const int tile_h = 9;
    bool filled = centered && feedback != FLIPPLE_TILE_EMPTY;

    if(filled) {
        canvas_draw_box(canvas, x, y, tile_w, tile_h);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_draw_frame(canvas, x, y, tile_w, tile_h);
    }

    char str[2] = {ch, '\0'};
    canvas_draw_str(canvas, x + 4, y + 8, str);

    if(filled) {
        canvas_set_color(canvas, ColorBlack);
        if(feedback == FLIPPLE_TILE_GREEN) {
            canvas_draw_line(canvas, x + 3, y + tile_h - 2, x + tile_w - 4, y + tile_h - 2);
        }
    }

    if(active && centered) {
        canvas_draw_frame(canvas, x - 2, y - 2, tile_w + 4, tile_h + 4);
    }
}

static void draw_key_counter(Canvas* canvas, const FlippleModel* m) {
    canvas_draw_frame(canvas, 40, 56, 18, 8);
    canvas_draw_circle(canvas, 47, 60, 3);
    canvas_draw_dot(canvas, 47, 60);
    canvas_draw_line(canvas, 50, 60, 55, 60);
    canvas_draw_line(canvas, 54, 60, 54, 62);

    char count[8];
    snprintf(count, sizeof(count), "x%u", (unsigned int)m->keys_left);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 64, 63, count);
}

static void draw_callback(Canvas* canvas, void* ctx) {
    FlippleApp* app = ctx;
    FlippleModel* m = app->model;
    furi_mutex_acquire(m->mutex, FuriWaitForever);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_frame(canvas, 54, 3, 20, 52);
    canvas_draw_frame(canvas, 56, 5, 16, 48);

    const int center_x = 58;
    const int center_y = 7;
    const int step_x = 16;
    const int step_y = 10;

    for(int row = 0; row < FLIPPLE_WORD_LENGTH; row++) {
        int y = center_y + row * step_y;
        for(int offset = -(int)m->left_span[row]; offset <= (int)m->right_span[row]; offset++) {
            int reel_index = (int)m->selected[row] + offset;
            while(reel_index < 0)
                reel_index += REEL_LENGTH;
            reel_index %= REEL_LENGTH;

            bool centered = offset == 0;
            int x = center_x + offset * step_x;
            draw_reel_tile(
                canvas,
                x,
                y,
                m->reels[row][reel_index],
                centered,
                row == m->active_row && !m->game_over,
                centered ? m->feedback[row] : FLIPPLE_TILE_EMPTY);
        }
    }

    draw_key_counter(canvas, m);

    if(m->game_over) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_box(canvas, 82, 55, 45, 9);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str(canvas, 85, 63, m->win ? "SOLVED" : m->target);
        canvas_set_color(canvas, ColorBlack);
    }

    furi_mutex_release(m->mutex);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    FlippleApp* app = ctx;
    furi_message_queue_put(app->event_queue, input_event, 0);
}

static void build_reels(FlippleModel* m) {
    for(size_t row = 0; row < FLIPPLE_WORD_LENGTH; row++) {
        char target = m->target[row];
        for(size_t col = 0; col < REEL_LENGTH; col++) {
            int letter = (target - 'A' + reel_offsets[col]) % 26;
            if(letter < 0) letter += 26;
            m->reels[row][col] = (char)('A' + letter);
        }
        m->selected[row] = (uint8_t)((row * 2 + 1) % REEL_LENGTH);
        if(m->selected[row] == 4) {
            m->selected[row] = 5;
        }

        uint8_t visible_count = (uint8_t)(3 + (rand() % 6));
        uint8_t extra = visible_count - 1;
        m->left_span[row] = (uint8_t)(rand() % (extra + 1));
        m->right_span[row] = (uint8_t)(extra - m->left_span[row]);
    }
}

static void start_round(FlippleModel* m, const char* target) {
    strcpy(m->target, target);
    memset(m->feedback, 0, sizeof(m->feedback));
    build_reels(m);
    m->active_row = 0;
    m->keys_left = FLIPPLE_MAX_GUESSES;
    m->game_over = false;
    m->win = false;
}

static void current_word(const FlippleModel* m, char word[FLIPPLE_WORD_LENGTH + 1]) {
    for(size_t row = 0; row < FLIPPLE_WORD_LENGTH; row++) {
        word[row] = m->reels[row][m->selected[row]];
    }
    word[FLIPPLE_WORD_LENGTH] = '\0';
}

static void rotate_active_reel(FlippleModel* m, int delta) {
    if(m->feedback[m->active_row] == FLIPPLE_TILE_GREEN) return;

    int next = (int)m->selected[m->active_row] + delta;
    while(next < 0)
        next += REEL_LENGTH;
    m->selected[m->active_row] = (uint8_t)(next % REEL_LENGTH);
    m->feedback[m->active_row] = FLIPPLE_TILE_EMPTY;
}

static void stats_load(FlippleModel* m) {
    File* file = storage_file_alloc(m->storage);
    if(!file) return;

    if(storage_file_open(file, stats_path(), FSAM_READ, FSOM_OPEN_EXISTING)) {
        char buf[128] = {0};
        size_t read = storage_file_read(file, buf, sizeof(buf) - 1);
        buf[read] = '\0';
        unsigned long games = 0;
        unsigned long wins = 0;
        unsigned long current = 0;
        unsigned long best = 0;
        unsigned long score = 0;
        if(sscanf(
               buf,
               "games=%lu wins=%lu current=%lu best=%lu score=%lu",
               &games,
               &wins,
               &current,
               &best,
               &score) == 5) {
            m->stats.games_played = (uint32_t)games;
            m->stats.wins = (uint32_t)wins;
            m->stats.current_streak = (uint32_t)current;
            m->stats.best_streak = (uint32_t)best;
            m->stats.best_score = (uint32_t)score;
        }
        storage_file_close(file);
    }

    storage_file_free(file);
}

static void stats_save(FlippleModel* m) {
    if(!m->stats_dirty) return;

    storage_simply_mkdir(m->storage, APP_DATA_PATH(""));
    File* file = storage_file_alloc(m->storage);
    if(!file) return;

    if(storage_file_open(file, stats_path(), FSAM_WRITE, FSOM_OPEN_ALWAYS)) {
        char buf[128];
        int len = snprintf(
            buf,
            sizeof(buf),
            "games=%lu wins=%lu current=%lu best=%lu score=%lu\n",
            (unsigned long)m->stats.games_played,
            (unsigned long)m->stats.wins,
            (unsigned long)m->stats.current_streak,
            (unsigned long)m->stats.best_streak,
            (unsigned long)m->stats.best_score);
        storage_file_seek(file, 0, true);
        storage_file_write(file, buf, (size_t)len);
        storage_file_truncate(file);
        storage_file_sync(file);
        storage_file_close(file);
        m->stats_dirty = false;
    }

    storage_file_free(file);
}

static size_t
    load_wordlist(Storage* storage, char words[][FLIPPLE_WORD_LENGTH + 1], size_t max_words) {
    File* file = storage_file_alloc(storage);
    if(!file) return 0;

    size_t count = 0;
    if(storage_file_open(file, wordlist_path(), FSAM_READ, FSOM_OPEN_EXISTING)) {
        char buf[64];
        size_t idx = 0;
        size_t read = 0;
        while((read = storage_file_read(file, buf, sizeof(buf))) > 0 && count < max_words) {
            for(size_t i = 0; i < read && count < max_words; i++) {
                char ch = buf[i];
                if(ch == '\r') continue;
                if(ch == '\n') {
                    if(idx == FLIPPLE_WORD_LENGTH) {
                        words[count][FLIPPLE_WORD_LENGTH] = '\0';
                        count++;
                    }
                    idx = 0;
                    continue;
                }
                if(idx < FLIPPLE_WORD_LENGTH && ch >= 'a' && ch <= 'z')
                    ch = (char)(ch - ('a' - 'A'));
                if(idx < FLIPPLE_WORD_LENGTH && ch >= 'A' && ch <= 'Z') {
                    words[count][idx++] = ch;
                }
            }
        }
        if(idx == FLIPPLE_WORD_LENGTH && count < max_words) {
            words[count][FLIPPLE_WORD_LENGTH] = '\0';
            count++;
        }
        storage_file_close(file);
    }

    storage_file_free(file);
    return count;
}

static void submit_guess(FlippleModel* m) {
    if(m->game_over || m->keys_left == 0) return;

    char guess[FLIPPLE_WORD_LENGTH + 1];
    current_word(m, guess);
    FlippleGuessResult result = {0};
    flipple_evaluate_guess(m->target, guess, &result);
    memcpy(m->feedback, result.states, sizeof(result.states));

    if(result.win) {
        m->game_over = true;
        m->win = true;
        flipple_apply_round_result(
            &m->stats, true, (uint32_t)(FLIPPLE_MAX_GUESSES - m->keys_left));
        m->stats_dirty = true;
    } else {
        m->keys_left--;
        if(m->keys_left == 0) {
            m->game_over = true;
            m->win = false;
            flipple_apply_round_result(&m->stats, false, FLIPPLE_MAX_GUESSES - 1);
            m->stats_dirty = true;
        }
    }
}

int32_t flipple_app(void* p) {
    UNUSED(p);
    FlippleApp* app = malloc(sizeof(FlippleApp));
    if(!app) return 255;
    memset(app, 0, sizeof(FlippleApp));

    app->model = malloc(sizeof(FlippleModel));
    if(!app->model) {
        free(app);
        return 255;
    }
    memset(app->model, 0, sizeof(FlippleModel));

    uint32_t tick = furi_hal_rtc_get_timestamp();
    srand(tick);
    int word_idx = rand() % WORD_COUNT;
    strcpy(app->model->target, word_list[word_idx]);

    app->model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!app->model->mutex) goto cleanup;

    app->model->storage = furi_record_open(RECORD_STORAGE);
    if(!app->model->storage) goto cleanup;

    stats_load(app->model);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    if(!app->event_queue) goto cleanup;

    app->view_port = view_port_alloc();
    if(!app->view_port) goto cleanup;

    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);

    app->gui = furi_record_open(RECORD_GUI);
    if(!app->gui) goto cleanup;

    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->runtime_word_list = malloc(sizeof(char[WORD_COUNT][FLIPPLE_WORD_LENGTH + 1]));
    if(!app->runtime_word_list) goto cleanup;

    app->runtime_word_count =
        load_wordlist(app->model->storage, app->runtime_word_list, WORD_COUNT);
    if(app->runtime_word_count == 0) {
        for(size_t i = 0; i < WORD_COUNT; i++) {
            strcpy(app->runtime_word_list[i], word_list[i]);
        }
        app->runtime_word_count = WORD_COUNT;
    }
    word_idx = rand() % app->runtime_word_count;
    start_round(app->model, app->runtime_word_list[word_idx]);

    InputEvent input;
    while(furi_message_queue_get(app->event_queue, &input, FuriWaitForever) == FuriStatusOk) {
        if(input.type == InputTypeLong && input.key == InputKeyBack) {
            break;
        }

        if(input.type == InputTypeShort) {
            furi_mutex_acquire(app->model->mutex, FuriWaitForever);

            if(input.key == InputKeyBack) {
                furi_mutex_release(app->model->mutex);
                break;
            }

            if(!app->model->game_over) {
                if(input.key == InputKeyLeft) {
                    rotate_active_reel(app->model, -1);
                } else if(input.key == InputKeyRight) {
                    rotate_active_reel(app->model, 1);
                } else if(input.key == InputKeyUp) {
                    if(app->model->active_row > 0) app->model->active_row--;
                } else if(input.key == InputKeyDown) {
                    if(app->model->active_row < FLIPPLE_WORD_LENGTH - 1) app->model->active_row++;
                } else if(input.key == InputKeyOk) {
                    submit_guess(app->model);
                }
            } else {
                if(input.key == InputKeyOk) {
                    tick = furi_hal_rtc_get_timestamp();
                    srand(tick + rand());
                    word_idx = rand() % app->runtime_word_count;
                    start_round(app->model, app->runtime_word_list[word_idx]);
                }
            }

            stats_save(app->model);
            furi_mutex_release(app->model->mutex);
        }
        view_port_update(app->view_port);
    }

cleanup:
    if(app->gui && app->view_port) {
        gui_remove_view_port(app->gui, app->view_port);
    }
    if(app->view_port) {
        view_port_free(app->view_port);
    }
    if(app->event_queue) {
        furi_message_queue_free(app->event_queue);
    }
    if(app->model) {
        if(app->model->mutex) {
            furi_mutex_free(app->model->mutex);
        }
        if(app->model->storage) {
            furi_record_close(RECORD_STORAGE);
        }
        free(app->model);
    }
    if(app->runtime_word_list) {
        free(app->runtime_word_list);
    }
    if(app->gui) {
        furi_record_close(RECORD_GUI);
    }
    free(app);
    return 0;
}
