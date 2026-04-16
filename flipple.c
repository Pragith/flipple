#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

#define TAG "Flipple"

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
    char guesses[6][6];
    uint8_t states[6][5]; // 0: none, 1: gray, 2: yellow, 3: green
    int current_guess;
    int current_letter;
    char target[6];
    int cursor_x;
    int cursor_y;
    bool game_over;
    bool win;
    FuriMutex* mutex;
} FlippleModel;

typedef struct {
    FlippleModel* model;
    ViewPort* view_port;
    Gui* gui;
    FuriMessageQueue* event_queue;
} FlippleApp;

const char* keyboard[3] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM<>"
};
const int key_counts[3] = {10, 9, 9};

static void draw_callback(Canvas* canvas, void* ctx) {
    FlippleApp* app = ctx;
    FlippleModel* m = app->model;
    furi_mutex_acquire(m->mutex, FuriWaitForever);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontKeyboard);

    // Draw Grid
    for(int r=0; r<6; r++) {
        for(int c=0; c<5; c++) {
            int x = c * 10 + 2;
            int y = r * 10 + 2;
            
            canvas_draw_frame(canvas, x, y, 9, 9);
            
            if (m->guesses[r][c] != '\0') {
                char str[2] = {m->guesses[r][c], '\0'};
                
                if (m->states[r][c] == 1) { // Gray
                    canvas_draw_dot(canvas, x+2, y+2);
                    canvas_draw_str(canvas, x + 2, y + 8, str);
                } else if (m->states[r][c] == 2) { // Yellow
                    canvas_draw_line(canvas, x+1, y+8, x+7, y+8);
                    canvas_draw_str(canvas, x + 2, y + 8, str);
                } else if (m->states[r][c] == 3) { // Green
                    canvas_draw_box(canvas, x, y, 9, 9);
                    canvas_set_color(canvas, ColorWhite);
                    canvas_draw_str(canvas, x + 2, y + 8, str);
                    canvas_set_color(canvas, ColorBlack);
                } else {
                    canvas_draw_str(canvas, x + 2, y + 8, str);
                }
            }
        }
    }

    // Draw Keyboard
    int kb_x = 55;
    int kb_y = 10;
    for(int r=0; r<3; r++) {
        for(int c=0; c<key_counts[r]; c++) {
            int kx = kb_x + c * 7 + (r==1?3:0) + (r==2?6:0);
            int ky = kb_y + r * 10;
            char str[2] = {keyboard[r][c], '\0'};
            
            bool is_selected = (m->cursor_y == r && m->cursor_x == c && !m->game_over);
            
            if (is_selected) {
                canvas_draw_box(canvas, kx-1, ky-1, 7, 10);
                canvas_set_color(canvas, ColorWhite);
            }
            
            if (str[0] == '<') canvas_draw_str(canvas, kx, ky+8, "B"); // Backspace
            else if (str[0] == '>') canvas_draw_str(canvas, kx, ky+8, "E"); // Enter
            else canvas_draw_str(canvas, kx, ky+8, str);
            
            if (is_selected) {
                canvas_set_color(canvas, ColorBlack);
            }
        }
    }
    
    // Status
    canvas_set_font(canvas, FontSecondary);
    if (m->game_over) {
        if (m->win) {
            canvas_draw_str(canvas, 60, 50, "You Win!");
        } else {
            canvas_draw_str(canvas, 60, 50, "Game Over!");
            canvas_draw_str(canvas, 60, 60, m->target);
        }
        canvas_draw_str(canvas, 60, 40, "Press OK");
    }

    furi_mutex_release(m->mutex);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    FlippleApp* app = ctx;
    furi_message_queue_put(app->event_queue, input_event, 0);
}

static void process_guess(FlippleModel* m) {
    if (m->current_letter < 5) return; // Need 5 letters
    
    bool win = true;
    bool target_used[5] = {false};
    
    // First pass: Green
    for(int c=0; c<5; c++) {
        if (m->guesses[m->current_guess][c] == m->target[c]) {
            m->states[m->current_guess][c] = 3;
            target_used[c] = true;
        } else {
            win = false;
        }
    }
    
    // Second pass: Yellow and Gray
    for(int c=0; c<5; c++) {
        if (m->states[m->current_guess][c] == 3) continue;
        
        bool found = false;
        for(int j=0; j<5; j++) {
            if (!target_used[j] && m->guesses[m->current_guess][c] == m->target[j]) {
                m->states[m->current_guess][c] = 2;
                target_used[j] = true;
                found = true;
                break;
            }
        }
        if (!found) {
            m->states[m->current_guess][c] = 1;
        }
    }
    
    if (win) {
        m->game_over = true;
        m->win = true;
    } else {
        m->current_guess++;
        m->current_letter = 0;
        if (m->current_guess >= 6) {
            m->game_over = true;
            m->win = false;
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
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    
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
                    } else if (app->model->current_letter < 5) {
                        app->model->guesses[app->model->current_guess][app->model->current_letter] = c;
                        app->model->current_letter++;
                    }
                }
            } else {
                if (input.key == InputKeyOk) {
                    memset(app->model->guesses, 0, sizeof(app->model->guesses));
                    memset(app->model->states, 0, sizeof(app->model->states));
                    app->model->current_guess = 0;
                    app->model->current_letter = 0;
                    app->model->game_over = false;
                    app->model->win = false;
                    app->model->cursor_x = 0;
                    app->model->cursor_y = 0;
                    tick = furi_hal_rtc_get_timestamp();
                    srand(tick + rand());
                    word_idx = rand() % WORD_COUNT;
                    strcpy(app->model->target, word_list[word_idx]);
                }
            }
            
            furi_mutex_release(app->model->mutex);
        }
        view_port_update(app->view_port);
    }
    
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->model->mutex);
    free(app->model);
    free(app);
    furi_record_close(RECORD_GUI);
    return 0;
}
