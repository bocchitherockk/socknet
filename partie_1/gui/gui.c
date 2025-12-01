#include <stddef.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>

#include "./Process.h"

#define SCREEN_WIDTH  1400
#define SCREEN_HEIGHT 900

#define HEADER_HEIGHT 70
#define SIDEBAR_WIDTH 200
#define PANEL_PADDING 20
#define PROCESS_HEIGHT 235
#define PROCESS_SPACING 15

// Color palette
#define COLOR_BG           (Color){245, 247, 250, 255}
#define COLOR_HEADER       (Color){30 , 41 , 59 , 255}
#define COLOR_SIDEBAR      (Color){51 , 65 , 85 , 255}
#define COLOR_SERVER       (Color){99 , 102, 241, 255}
#define COLOR_CLIENT       (Color){16 , 185, 129, 255}
#define COLOR_PANEL        (Color){255, 255, 255, 255}
#define COLOR_BUTTON       (Color){59 , 130, 246, 255}
#define COLOR_BUTTON_HOVER (Color){37 , 99 , 235, 255}
#define COLOR_TEXT_LIGHT   (Color){148, 163, 184, 255}
#define COLOR_TEXT_DARK    (Color){30 , 41 , 59 , 255}
#define COLOR_EXECUTING    (Color){251, 191, 36 , 255}
#define COLOR_STOPPED      (Color){34 , 197, 94 , 255}
#define COLOR_CODE_BG      (Color){248, 250, 252, 255}

Font font_times_new_roman = {0};
Font font_iosevka = {0};

// Helper function to draw rounded rectangles
void DrawRectangleRoundedCustom(Rectangle rec, float roundness, Color color) {
    DrawRectangleRounded(rec, roundness, 10, color);
}

// Helper function to draw text with shadow
void DrawTextShadow(Font font, const char *text, Vector2 pos, float fontSize, float spacing, Color color) {
    DrawTextEx(font, text, (Vector2){pos.x + 1, pos.y + 1}, fontSize, spacing, (Color){0, 0, 0, 30});
    DrawTextEx(font, text, pos, fontSize, spacing, color);
}

// Draw a modern button with hover effect
bool DrawButton(Rectangle bounds, const char *text, Font font) {
    Vector2 mouse = GetMousePosition();
    bool is_hovering = CheckCollisionPointRec(mouse, bounds);
    bool is_clicked = is_hovering && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color button_color = is_hovering ? COLOR_BUTTON_HOVER : COLOR_BUTTON;

    DrawRectangleRoundedCustom(bounds, 0.3f, button_color);

    if (is_hovering) {
        DrawRectangleRoundedLines(bounds, 0.3f, 10, (Color){255, 255, 255, 100});
    }

    Vector2 text_size = MeasureTextEx(font, text, 18, 1);
    Vector2 text_pos = {
        bounds.x + (bounds.width - text_size.x) / 2,
        bounds.y + (bounds.height - text_size.y) / 2
    };

    DrawTextEx(font, text, text_pos, 18, 1, WHITE);

    return is_clicked;
}

// Draw header with title and stats
void DrawHeader() {
    DrawRectangle(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER);

    // Title
    DrawTextShadow(font_times_new_roman, "Process Debugger", (Vector2){30, 22}, 28, 1, WHITE);

    // Stats
    char server_status[50];
    sprintf(server_status, "Server: %s", server.is_set ? "ACTIVE" : "INACTIVE");
    DrawTextEx(font_times_new_roman, server_status, (Vector2){SCREEN_WIDTH - 350, 20}, 16, 1,
               server.is_set ? COLOR_STOPPED : COLOR_TEXT_LIGHT);

    char clients_count[50];
    sprintf(clients_count, "Clients: %zu", clients.count);
    DrawTextEx(font_times_new_roman, clients_count, (Vector2){SCREEN_WIDTH - 350, 42}, 16, 1, COLOR_TEXT_LIGHT);
}

// Draw sidebar with controls
void DrawSidebar() {
    DrawRectangle(0, HEADER_HEIGHT, SIDEBAR_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, COLOR_SIDEBAR);

    // Section title
    DrawTextEx(font_times_new_roman, "CONTROLS", (Vector2){20, HEADER_HEIGHT + 30}, 14, 1, COLOR_TEXT_LIGHT);

    // Create Server button
    Rectangle server_btn = {20, HEADER_HEIGHT + 70, SIDEBAR_WIDTH - 40, 50};
    if (DrawButton(server_btn, "Create Server", font_times_new_roman)) {
        if (!server.is_set) {
            Process_create(PROCESS_TYPE_SERVER);
        }
    }

    // Create Client button
    Rectangle client_btn = {20, HEADER_HEIGHT + 135, SIDEBAR_WIDTH - 40, 50};
    if (DrawButton(client_btn, "Create Client", font_times_new_roman)) {
        Process_create(PROCESS_TYPE_CLIENT);
    }

    // Instructions
    DrawTextEx(font_times_new_roman, "KEYBOARD", (Vector2){20, HEADER_HEIGHT + 250}, 14, 1, COLOR_TEXT_LIGHT);
    DrawTextEx(font_times_new_roman, "Q - Quit", (Vector2){20, HEADER_HEIGHT + 280}, 13, 1, (Color){180, 190, 200, 255});
}

// Draw a single process card
void DrawProcessCard(Rectangle bounds, Process *process, const char *label) {
    // Main card background
    DrawRectangleRoundedCustom(bounds, 0.15f, COLOR_PANEL);
    DrawRectangleRoundedLines(bounds, 0.15f, 10, (Color){226, 232, 240, 255});

    // Header with label and status
    Color header_color = process->process_type == PROCESS_TYPE_SERVER ? COLOR_SERVER : COLOR_CLIENT;
    DrawRectangleRounded(
        (Rectangle){bounds.x, bounds.y, bounds.width, 40},
        0.15f, 10,
        (Color){header_color.r, header_color.g, header_color.b, 30}
    );

    DrawTextEx(font_times_new_roman, label, (Vector2){bounds.x + 15, bounds.y + 12}, 18, 1, header_color);

    // Status indicator
    const char *status_text = process->process_state == PROCESS_STATE_EXECUTING ? "RUNNING" : "STOPPED";
    Color status_color = process->process_state == PROCESS_STATE_EXECUTING ? COLOR_EXECUTING : COLOR_STOPPED;

    Vector2 status_size = MeasureTextEx(font_times_new_roman, status_text, 12, 1);
    float status_x = bounds.x + bounds.width - status_size.x - 20;
    DrawCircle(status_x - 12, bounds.y + 18, 5, status_color);
    DrawTextEx(font_times_new_roman, status_text, (Vector2){status_x, bounds.y + 12}, 12, 1, status_color);

    float y_offset = bounds.y + 55;

    // Code line section
    DrawTextEx(font_times_new_roman, "Current Line:", (Vector2){bounds.x + 15, y_offset}, 13, 1, COLOR_TEXT_LIGHT);
    y_offset += 20;

    Rectangle code_bg = {bounds.x + 15, y_offset, bounds.width - 30, 30};
    DrawRectangleRoundedCustom(code_bg, 0.2f, COLOR_CODE_BG);

    // Truncate line content if too long
    char display_line[100];
    strncpy(display_line, process->line_content, 95);
    display_line[95] = '\0';
    if (strlen(process->line_content) > 95) {
        strcat(display_line, "...");
    }

    DrawTextEx(font_iosevka, display_line, (Vector2){bounds.x + 25, y_offset + 8}, 12, 0.5f, COLOR_TEXT_DARK);
    y_offset += 40;

    // Question and Answer in two columns
    float col_width = (bounds.width - 45) / 2;

    // Question column
    DrawTextEx(font_times_new_roman, "Question:", (Vector2){bounds.x + 15, y_offset}, 13, 1, COLOR_TEXT_LIGHT);
    DrawTextEx(font_iosevka, process->question, (Vector2){bounds.x + 15, y_offset + 18}, 14, 0.5f, (Color){16, 185, 129, 255});

    // Answer column
    // TODO: make text non overflowing
    DrawTextEx(font_times_new_roman, "Answer:", (Vector2){bounds.x + 30 + col_width, y_offset}, 13, 1, COLOR_TEXT_LIGHT);
    DrawTextEx(font_iosevka, process->answer, (Vector2){bounds.x + 30 + col_width, y_offset + 18}, 14, 0.5f, (Color){239, 68, 68, 255});
    DrawTextEx(font_iosevka, process->answer_data, (Vector2){bounds.x + 30 + col_width, y_offset + 42}, 14, 0.5f, (Color){239, 68, 68, 255});

    y_offset += 70;

    // Next button
    Rectangle next_btn = {bounds.x + 15, y_offset, bounds.width - 30, 35};
    bool disabled = process->process_state == PROCESS_STATE_EXECUTING;

    if (!disabled && DrawButton(next_btn, "Execute Next", font_times_new_roman)) {
        Process_next(process);
    } else if (disabled) {
        DrawRectangleRoundedCustom(next_btn, 0.3f, (Color){148, 163, 184, 100});
        Vector2 text_size = MeasureTextEx(font_times_new_roman, "Execute Next", 18, 1);
        Vector2 text_pos = {
            next_btn.x + (next_btn.width - text_size.x) / 2,
            next_btn.y + (next_btn.height - text_size.y) / 2
        };
        DrawTextEx(font_times_new_roman, "Execute Next", text_pos, 18, 1, (Color){148, 163, 184, 255});
    }
}

// Draw server panel
void DrawServerPanel() {
    if (!server.is_set) {
        // Empty state
        Rectangle panel = {
            SIDEBAR_WIDTH + PANEL_PADDING,
            HEADER_HEIGHT + PANEL_PADDING,
            (SCREEN_WIDTH - SIDEBAR_WIDTH) / 2 - PANEL_PADDING * 1.5f,
            PROCESS_HEIGHT
        };

        DrawRectangleRoundedCustom(panel, 0.15f, (Color){255, 255, 255, 100});
        DrawRectangleLinesEx(
            panel,
            2,
            (Color){226, 232, 240, 255}
        );

        const char *text = "No server running";
        Vector2 text_size = MeasureTextEx(font_times_new_roman, text, 16, 1);
        DrawTextEx(font_times_new_roman, text,
                   (Vector2){panel.x + (panel.width - text_size.x) / 2,
                            panel.y + (panel.height - text_size.y) / 2},
                   16, 1, COLOR_TEXT_LIGHT);
        return;
    }

    Rectangle panel = {
        SIDEBAR_WIDTH + PANEL_PADDING,
        HEADER_HEIGHT + PANEL_PADDING,
        (SCREEN_WIDTH - SIDEBAR_WIDTH) / 2 - PANEL_PADDING * 1.5f,
        PROCESS_HEIGHT
    };

    DrawProcessCard(panel, &server, "SERVER PROCESS");
}

int clients_scroll = 0;
#define CLIENTS_SCROLL_SPEED 45

// Draw clients panel with scrolling
void DrawClientsPanel() {
    float panel_width = (SCREEN_WIDTH - SIDEBAR_WIDTH) / 2 - PANEL_PADDING * 1.5f;
    float panel_x = SIDEBAR_WIDTH + (SCREEN_WIDTH - SIDEBAR_WIDTH) / 2 + PANEL_PADDING / 2;

    if (clients.count == 0) {
        // Empty state
        Rectangle panel = {
            panel_x,
            HEADER_HEIGHT + PANEL_PADDING,
            panel_width,
            PROCESS_HEIGHT
        };

        DrawRectangleRoundedCustom(panel, 0.15f, (Color){255, 255, 255, 100});
        DrawRectangleLinesEx(panel, 2, (Color){226, 232, 240, 255});

        const char *text = "No clients connected";
        Vector2 text_size = MeasureTextEx(font_times_new_roman, text, 16, 1);
        DrawTextEx(font_times_new_roman, text,
                   (Vector2){panel.x + (panel.width - text_size.x) / 2,
                            panel.y + (panel.height - text_size.y) / 2},
                   16, 1, COLOR_TEXT_LIGHT);
        return;
    }

    Rectangle scrollable_panel = {
        .x = panel_x,
        .y = HEADER_HEIGHT + PANEL_PADDING,
        .width = panel_width,
        .height = SCREEN_HEIGHT - HEADER_HEIGHT - PANEL_PADDING,
    };

    Vector2 mouse_position = GetMousePosition();
    if (CheckCollisionPointRec(mouse_position, scrollable_panel)) {
        clients_scroll = Clamp(
            clients_scroll + GetMouseWheelMove() * CLIENTS_SCROLL_SPEED,
            -((float)clients.count * (PROCESS_HEIGHT + PANEL_PADDING)) + (SCREEN_HEIGHT - HEADER_HEIGHT),
            0
        );
    }
    BeginScissorMode(scrollable_panel.x, scrollable_panel.y, scrollable_panel.width, scrollable_panel.height);

    // Draw each client
    for (size_t i = 0; i < clients.count; i++) {
        float y_pos = HEADER_HEIGHT + PANEL_PADDING + i * (PROCESS_HEIGHT + PROCESS_SPACING);

        Rectangle panel = {
            panel_x,
            y_pos + clients_scroll,
            panel_width,
            PROCESS_HEIGHT
        };

        char label[50];
        sprintf(label, "CLIENT #%zu", i + 1);
        DrawProcessCard(panel, &clients.items[i], label);
    }

    EndScissorMode();
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Process Debugger - Client/Server Monitor");
    SetTargetFPS(60);

    font_times_new_roman = LoadFontEx("./gui/fonts/times.ttf", 96, 0, 0);
    font_iosevka = LoadFontEx("./gui/fonts/Iosevka-Slab-Semibold-01.ttf", 96, 0, 0);

    SetTextureFilter(font_times_new_roman.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(font_iosevka.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(COLOR_BG);

            DrawHeader();
            DrawSidebar();
            DrawServerPanel();
            DrawClientsPanel();

        EndDrawing();

        if (IsKeyPressed(KEY_A)) break;
    }

    UnloadFont(font_times_new_roman);
    UnloadFont(font_iosevka);
    CloseWindow();
    Process_kill_all();

    return 0;
}