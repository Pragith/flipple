#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <string.h>

#include "flipple_engine.h"

#define TAG "Flipple"
#define STATS_FILE "stats.txt"
#define WORDLIST_FILE "wordlist.txt"

const char* word_list[] = {
    "APPLE", "BERRY", "CHAIR", "DANCE", "EAGLE",
    "FLAME", "GRAPE", "HOUSE", "IMAGE", "JUICE",
    "KNIFE", "LEMON", "MOUSE", "NIGHT", "OCEAN",
    "PIZZA", "QUEEN", "RIVER", "SNAKE", "TRAIN",
    "UNCLE", "VOICE", "WATER", "XENON", "YACHT",
    "ZEBRA", "GHOST", "BLAME", "CRISP", "DRIVE",
    "EMPTY", "FORCE", "GUEST", "HEART", "INDEX",
    "JOLLY", "KNEEL", "LARGE", "MAGIC", "NOVEL",
    "ONION", "PULSE", "QUOTE", "ROUND", "SUGAR"
};
#define WORD_COUNT (sizeof(word_list)/sizeof(word_list[0]))

typedef struct {
    char guesses[FLIPPLE_MAX_GUESSES][FLIPPLE_WORD_LENGTH + 1];
    uint8_t states[FLIPPLE_MAX_GUESSES][FLIPPLE_WORD_LENGTH];
    int current_guess;
    int current_letter;
    char target[FLIPPLE_WORD_LENGTH + 1];
    int cursor_x;
    int cursor_y;
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
} FlippleApp;

static const char* stats_path(void) {
    return APP_DATA_PATH(STATS_FILE);
}

static const char* wordlist_path(void) {
    return APP_DATA_PATH(WORDLIST_FILE);
}

const char* keyboard[3] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM<>"
};
const int key_counts[3] = {10, 9, 9};

static void draw_tile(Canvas* canvas, int x, int y, char ch, uint8_t state, bool active) {
    if(state == TILE_GREEN) {
        canvas_draw_box(canvas, x, y, 8, 8);
        canvas_set_color(canvas, ColorWhite);
    } else if(state == TILE_YELLOW) {
        canvas_draw_frame(canvas, x, y, 8, 8);
        canvas_draw_line(canvas, x + 1, y + 6, x + 6, y + 6);
    } else if(state == TILE_GRAY) {
        canvas_draw_frame(canvas, x, y, 8, 8);
        canvas_draw_dot(canvas, x + 3, y + 3);
    } else {
        canvas_draw_frame(canvas, x, y, 8, 8);
    }

    if(active && state == TILE_EMPTY) {
        canvas_draw_line(canvas, x + 1, y + 1, x + 6, y + 6);
    }

    if(ch != '\0') {
        char str[2] = {ch, '\0'};
        canvas_draw_str(canvas, x + 2, y + 7, str);
    }

    if(state == TILE_GREEN) {
        canvas_set_color(canvas, ColorBlack);
    }
}

static void draw_callback(Canvas* canvas, void* ctx) {
    FlippleApp* app = ctx;
    FlippleModel* m = app->model;
    furi_mutex_acquire(m->mutex, FuriWaitForever);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 8, "Flipple");
    canvas_draw_str(canvas, 56, 8, m->game_over ? (m->win ? "Solved" : "Failed") : "Guess");
    canvas_draw_line(canvas, 0, 10, 127, 10);

    for(int r = 0; r < FLIPPLE_MAX_GUESSES; r++) {
        for(int c = 0; c < FLIPPLE_WORD_LENGTH; c++) {
            int x = 2 + c * 9;
            int y = 13 + r * 8;
            bool active = (!m->game_over && r == m->current_guess && c == m->current_letter);
            draw_tile(canvas, x, y, m->guesses[r][c], m->states[r][c], active);
        }
    }

    canvas_set_font(canvas, FontSecondary);
    if(m->game_over) {
        char line[32];
        snprintf(line, sizeof(line), "G%lu W%lu S%lu",
            (unsigned long)m->stats.games_played,
            (unsigned long)m->stats.wins,
            (unsigned long)m->stats.current_streak);
        canvas_draw_str(canvas, 49, 20, line);
        snprintf(line, sizeof(line), "Best %lu / %lu",
            (unsigned long)m->stats.best_score,
            (unsigned long)m->stats.best_streak);
        canvas_draw_str(canvas, 49, 29, line);
        canvas_draw_str(canvas, 49, 38, "OK new game");
        canvas_draw_str(canvas, 49, 47, "Back exit");
    } else {
        canvas_draw_str(canvas, 49, 20, "Controls");
        canvas_draw_str(canvas, 49, 29, "Arrows move");
        canvas_draw_str(canvas, 49, 38, "OK type");
        canvas_draw_str(canvas, 49, 47, "Back del");
        canvas_draw_str(canvas, 49, 56, "Hold BACK exit");
    }

    if(m->game_over) {
        canvas_draw_frame(canvas, 48, 13, 78, 34);
        if(m->win) {
            canvas_draw_str(canvas, 56, 25, "You win");
            canvas_draw_str(canvas, 56, 35, "Press OK");
        } else {
            canvas_draw_str(canvas, 56, 25, "Target");
            canvas_draw_str(canvas, 56, 35, m->target);
        }
    } else {
        int kb_x = 50;
        int kb_y = 13;
        canvas_set_font(canvas, FontKeyboard);
        for(int r = 0; r < 3; r++) {
            for(int c = 0; c < key_counts[r]; c++) {
                int kx = kb_x + c * 7 + (r == 1 ? 3 : 0) + (r == 2 ? 6 : 0);
                int ky = kb_y + r * 13;
                char ch = keyboard[r][c];
                bool selected = (m->cursor_y == r && m->cursor_x == c);

                if(selected) {
                    canvas_draw_box(canvas, kx - 1, ky - 1, 7, 10);
                    canvas_set_color(canvas, ColorWhite);
                }

                if(ch == '<') {
                    canvas_draw_str(canvas, kx, ky + 8, "DEL");
                } else if(ch == '>') {
                    canvas_draw_str(canvas, kx, ky + 8, "OK");
                } else {
                    char str[2] = {ch, '\0'};
                    canvas_draw_str(canvas, kx, ky + 8, str);
                }

                if(selected) {
                    canvas_set_color(canvas, ColorBlack);
                }
            }
        }
    }

    furi_mutex_release(m->mutex);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    FlippleApp* app = ctx;
    furi_message_queue_put(app->event_queue, input_event, 0);
}

static void stats_reset_session(FlippleModel* m) {
    memset(m->guesses, 0, sizeof(m->guesses));
    memset(m->states, 0, sizeof(m->states));
    m->current_guess = 0;
    m->current_letter = 0;
    m->game_over = false;
    m->win = false;
    m->cursor_x = 0;
    m->cursor_y = 0;
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
        if(sscanf(buf, "games=%lu wins=%lu current=%lu best=%lu score=%lu", &games, &wins, &current, &best, &score) == 5) {
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

static size_t load_wordlist(Storage* storage, char words[][FLIPPLE_WORD_LENGTH + 1], size_t max_words) {
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
                if(idx < FLIPPLE_WORD_LENGTH && ch >= 'a' && ch <= 'z') ch = (char)(ch - ('a' - 'A'));
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

static void process_guess(FlippleModel* m) {
    if(m->current_letter < FLIPPLE_WORD_LENGTH) return;

    FlippleGuessResult result = {0};
    flipple_evaluate_guess(m->target, m->guesses[m->current_guess], &result);
    memcpy(m->states[m->current_guess], result.states, sizeof(result.states));

    if(result.win) {
        m->game_over = true;
        m->win = true;
        flipple_apply_round_result(&m->stats, true, (uint32_t)m->current_guess);
    } else {
        m->current_guess++;
        m->current_letter = 0;
        if(m->current_guess >= FLIPPLE_MAX_GUESSES) {
            m->game_over = true;
            m->win = false;
            flipple_apply_round_result(&m->stats, false, (uint32_t)m->current_guess);
        }
    }
}

int32_t flipple_app(void* p) {
    UNUSED(p);
    FlippleApp* app = malloc(sizeof(FlippleApp));
    app->model = malloc(sizeof(FlippleModel));
    memset(app->model, 0, sizeof(FlippleModel));
    
    uint32_t tick = furi_hal_rtc_get_timestamp();
    srand(tick);
    int word_idx = rand() % WORD_COUNT;
    strcpy(app->model->target, word_list[word_idx]);
    
    app->model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->model->storage = furi_record_open(RECORD_STORAGE);
    stats_load(app->model);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    char runtime_word_list[WORD_COUNT][FLIPPLE_WORD_LENGTH + 1];
    size_t runtime_word_count = load_wordlist(app->model->storage, runtime_word_list, WORD_COUNT);
    if(runtime_word_count == 0) {
        for(size_t i = 0; i < WORD_COUNT; i++) {
            strcpy(runtime_word_list[i], word_list[i]);
        }
        runtime_word_count = WORD_COUNT;
    }

    InputEvent input;
    while(furi_message_queue_get(app->event_queue, &input, FuriWaitForever) == FuriStatusOk) {
        if(input.type == InputTypeShort) {
            furi_mutex_acquire(app->model->mutex, FuriWaitForever);
            
            if (input.key == InputKeyBack) {
                furi_mutex_release(app->model->mutex);
                break;
            }
            
            if (!app->model->game_over) {
                if (input.key == InputKeyLeft) {
                    if (app->model->cursor_x > 0) app->model->cursor_x--;
                } else if (input.key == InputKeyRight) {
                    if (app->model->cursor_x < key_counts[app->model->cursor_y]-1) app->model->cursor_x++;
                } else if (input.key == InputKeyUp) {
                    if (app->model->cursor_y > 0) {
                        app->model->cursor_y--;
                        if (app->model->cursor_x >= key_counts[app->model->cursor_y]) 
                            app->model->cursor_x = key_counts[app->model->cursor_y]-1;
                    }
                } else if (input.key == InputKeyDown) {
                    if (app->model->cursor_y < 2) {
                        app->model->cursor_y++;
                        if (app->model->cursor_x >= key_counts[app->model->cursor_y]) 
                            app->model->cursor_x = key_counts[app->model->cursor_y]-1;
                    }
                } else if (input.key == InputKeyOk) {
                    char c = keyboard[app->model->cursor_y][app->model->cursor_x];
                    if (c == '<') {
                        if (app->model->current_letter > 0) {
                            app->model->current_letter--;
                            app->model->guesses[app->model->current_guess][app->model->current_letter] = '\0';
                        }
                    } else if (c == '>') {
                        process_guess(app->model);
                    } else if (app->model->current_letter < FLIPPLE_WORD_LENGTH) {
                        app->model->guesses[app->model->current_guess][app->model->current_letter] = c;
                        app->model->current_letter++;
                    }
                }
            } else {
                if (input.key == InputKeyOk) {
                    stats_reset_session(app->model);
                    tick = furi_hal_rtc_get_timestamp();
                    srand(tick + rand());
                    word_idx = rand() % runtime_word_count;
                    strcpy(app->model->target, runtime_word_list[word_idx]);
                }
            }
            
            stats_save(app->model);
            furi_mutex_release(app->model->mutex);
        }
        view_port_update(app->view_port);
    }
    
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->model->mutex);
    furi_record_close(RECORD_STORAGE);
    free(app->model);
    free(app);
    furi_record_close(RECORD_GUI);
    return 0;
}
