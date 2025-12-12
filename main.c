#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"
#include "network.h"

// Simple dynamic string for text input
typedef struct {
    char *text;
    int size;
    int capacity;
} TextBuffer;

static TextBuffer *TextBuffer_Create(int initial_capacity) {
    TextBuffer *buf = malloc(sizeof(TextBuffer));
    if (!buf) return NULL;
    buf->text = malloc(initial_capacity);
    if (!buf->text) {
        free(buf);
        return NULL;
    }
    buf->text[0] = '\0';
    buf->size = 0;
    buf->capacity = initial_capacity;
    return buf;
}

static void TextBuffer_Free(TextBuffer *buf) {
    if (!buf) return;
    if (buf->text) free(buf->text);
    free(buf);
}

static bool TextBuffer_Append(TextBuffer *buf, const char *text, int len) {
    if (!buf || !text) return false;
    if (len < 0) len = strlen(text);
    if (buf->size + len + 1 > buf->capacity) {
        int new_cap = (buf->capacity * 3) / 2 + len + 1;
        char *new_text = realloc(buf->text, new_cap);
        if (!new_text) return false;
        buf->text = new_text;
        buf->capacity = new_cap;
    }
    memcpy(buf->text + buf->size, text, len);
    buf->size += len;
    buf->text[buf->size] = '\0';
    return true;
}

static void TextBuffer_Clear(TextBuffer *buf) {
    if (!buf) return;
    buf->text[0] = '\0';
    buf->size = 0;
}

static bool TextBuffer_Backspace(TextBuffer *buf) {
    if (!buf || buf->size <= 0) return false;
    buf->size--;
    buf->text[buf->size] = '\0';
    return true;
}

// (Network code moved to network.c)

// Lightweight pretty-printer for JSON (not a full parser; handles strings and basic structure)
static char *pretty_print_json(const char *input)
{
    if (!input)
        return NULL;
    size_t len = strlen(input);
    // allocate a buffer with some headroom
    size_t cap = len * 3 + 256;
    char *out = malloc(cap);
    if (!out)
        return NULL;
    size_t oi = 0;
    int indent = 0;
    bool in_string = false;
    bool escape = false;

    for (size_t i = 0; i < len; ++i)
    {
        char c = input[i];
        if (oi + 4 >= cap)
        {
            cap *= 1.5;
            char *n = realloc(out, cap);
            if (!n)
                break;
            out = n;
        }

        if (in_string)
        {
            out[oi++] = c;
            if (escape)
            {
                escape = false;
            }
            else if (c == '\\')
            {
                escape = true;
            }
            else if (c == '"')
            {
                in_string = false;
            }
            continue;
        }

        if (c == '"')
        {
            in_string = true;
            out[oi++] = c;
        }
        else if (c == '{' || c == '[')
        {
            out[oi++] = c;
            out[oi++] = '\n';
            indent++;
            for (int k = 0; k < indent; ++k)
                out[oi++] = '\t';
        }
        else if (c == '}' || c == ']')
        {
            out[oi++] = '\n';
            indent--;
            if (indent < 0)
                indent = 0;
            for (int k = 0; k < indent; ++k)
                out[oi++] = '\t';
            out[oi++] = c;
        }
        else if (c == ',')
        {
            out[oi++] = c;
            out[oi++] = '\n';
            for (int k = 0; k < indent; ++k)
                out[oi++] = '\t';
        }
        else if (c == ':')
        {
            out[oi++] = c;
            out[oi++] = ' ';
        }
        else if (c == ' ' || c == '\n' || c == '\t' || c == '\r')
        {
            // skip extra whitespace outside strings
        }
        else
        {
            out[oi++] = c;
        }
    }
    out[oi] = 0;
    return out;
}

// Network code moved to network.c

// Simple wrapped text drawer using MeasureTextEx + DrawTextEx so we don't depend on
// a specific raylib version's DrawTextRec availability.
static void DrawTextRecWrapped(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
{
    if (!text)
        return;
    float x = rec.x;
    float y = rec.y;
    float maxWidth = rec.width;
    Vector2 pos = {x, y};
    size_t len = strlen(text);
    // temporary buffer for building a line
    char *line = malloc(len + 1);
    if (!line)
        return;

    const char *ptr = text;
    while (*ptr)
    {
        // handle explicit newlines
        const char *nl = strchr(ptr, '\n');
        size_t seglen = nl ? (size_t)(nl - ptr) : strlen(ptr);

        // build the segment into words and wrap if needed
        size_t start = 0;
        while (start < seglen)
        {
            size_t end = start;
            size_t last_space = SIZE_MAX;
            // expand until we hit width or end
            while (end < seglen)
            {
                size_t copylen = end - start + 1;
                memcpy(line, ptr + start, copylen);
                line[copylen] = '\0';
                float w = MeasureTextEx(font, line, fontSize, spacing).x;
                if (w > maxWidth)
                    break;
                if (ptr[start + (end - start)] == ' ')
                    last_space = end;
                end++;
            }

            if (end == start)
            {
                // a single character is wider than the box; force at least one char
                size_t copylen = 1;
                memcpy(line, ptr + start, copylen);
                line[copylen] = '\0';
                DrawTextEx(font, line, pos, fontSize, spacing, tint);
                pos.y += fontSize + spacing;
                start += 1;
            }
            else
            {
                size_t copylen;
                if (!wordWrap || (end >= seglen && last_space == SIZE_MAX))
                {
                    copylen = end - start;
                }
                else if (last_space != SIZE_MAX && last_space > start)
                {
                    copylen = last_space - start;
                }
                else
                {
                    copylen = end - start;
                }
                // trim trailing spaces
                while (copylen > 0 && ptr[start + copylen - 1] == ' ')
                    copylen--;
                memcpy(line, ptr + start, copylen);
                line[copylen] = '\0';
                DrawTextEx(font, line, pos, fontSize, spacing, tint);
                pos.y += fontSize + spacing;
                start += (copylen == 0 ? 1 : copylen);
                // skip any spaces
                while (start < seglen && ptr[start] == ' ')
                    start++;
            }
        }

        // if there was a newline, advance past it
        if (nl)
            ptr = nl + 1;
        else
            break;
    }
    free(line);
}

int main(void) {
    const int screenW = 1024;
    const int screenH = 768;
    InitWindow(screenW, screenH, "PostWoman — tiny JSON inspector");
    SetTargetFPS(60);

    // Initialize network subsystem
    network_init();

    // UI state
    TextBuffer *url = TextBuffer_Create(2048);
    TextBuffer *body = TextBuffer_Create(8192);
    if (!url || !body) return 1;

    bool editing_url = false;
    bool editing_body = false;
    float body_scroll_y = 0.0f;
    float response_scroll_y = 0.0f;
    char *pretty = NULL;

    Rectangle urlRect = { 20, 20, 820, 32 };
    Rectangle sendRect = { 860, 20, 120, 32 };
    Rectangle bodyRect = { 20, 70, 960, 120 };
    Rectangle displayRect = { 20, 210, screenW - 40, screenH - 230 };

    while (!WindowShouldClose()) {
        // Input handling
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            editing_url = CheckCollisionPointRec(mouse, urlRect);
            editing_body = CheckCollisionPointRec(mouse, bodyRect);
            
            if (CheckCollisionPointRec(mouse, sendRect) && !network_is_loading()) {
                // Start fetch with URL and optional body
                network_fetch_async(url->text, body->size > 0 ? body->text : NULL);
                if (pretty) { free(pretty); pretty = NULL; }
            }
        }

        // Handle clipboard paste (Cmd+V on macOS, Ctrl+V on others)
        bool is_paste = (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER) || 
                        IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && 
                       IsKeyPressed(KEY_V);
        
        if (is_paste) {
            const char *clipboard = GetClipboardText();
            if (clipboard) {
                if (editing_url) {
                    TextBuffer_Clear(url);
                    TextBuffer_Append(url, clipboard, -1);
                } else if (editing_body) {
                    TextBuffer_Clear(body);
                    TextBuffer_Append(body, clipboard, -1);
                }
            }
        }

        // Regular keyboard input
        if (!is_paste) {
            TextBuffer *active = editing_url ? url : (editing_body ? body : NULL);
            if (active) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= 32 && key <= 125) {
                        char c = (char)key;
                        TextBuffer_Append(active, &c, 1);
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    TextBuffer_Backspace(active);
                }

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                    if (editing_url && !network_is_loading()) {
                        network_fetch_async(url->text, body->size > 0 ? body->text : NULL);
                        if (pretty) { free(pretty); pretty = NULL; }
                    } else if (editing_body) {
                        TextBuffer_Append(body, "\n", 1);
                    }
                }
            }
        }

        // Scrolling for body and response
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            if (CheckCollisionPointRec(mouse, bodyRect)) {
                body_scroll_y += wheel * 20.0f;
            } else if (CheckCollisionPointRec(mouse, displayRect)) {
                response_scroll_y += wheel * 20.0f;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // URL input box
        DrawRectangleRec(urlRect, editing_url ? WHITE : LIGHTGRAY);
        DrawRectangleLinesEx(urlRect, 1, editing_url ? DARKGRAY : GRAY);
        DrawText(url->text[0] ? url->text : "Enter URL (including http:// or https://)", 
                (int)urlRect.x + 6, (int)urlRect.y + 8, 12, 
                url->text[0] ? BLACK : GRAY);

        // Send button
        Color sendColor = network_is_loading() ? GRAY : LIGHTGRAY;
        DrawRectangleRec(sendRect, sendColor);
        DrawRectangleLinesEx(sendRect, 1, GRAY);
        DrawText(network_is_loading() ? "Loading..." : "Send", 
                (int)sendRect.x + 18, (int)sendRect.y + 8, 14, BLACK);

        // Body input area
        DrawRectangleRec(bodyRect, editing_body ? WHITE : LIGHTGRAY);
        DrawRectangleLinesEx(bodyRect, 1, editing_body ? DARKGRAY : GRAY);
        BeginScissorMode((int)bodyRect.x, (int)bodyRect.y, (int)bodyRect.width, (int)bodyRect.height);
        DrawTextRecWrapped(GetFontDefault(),
                         body->text[0] ? body->text : "Enter request body (optional)",
                         (Rectangle){bodyRect.x + 6, bodyRect.y + 6 + body_scroll_y,
                                   bodyRect.width - 12, bodyRect.height - 12},
                         12, 2, true, body->text[0] ? BLACK : GRAY);
        EndScissorMode();

        // Response display area
        DrawRectangleRec(displayRect, BLACK);
        BeginScissorMode((int)displayRect.x, (int)displayRect.y, 
                         (int)displayRect.width, (int)displayRect.height);
        
        const struct HttpResponse *resp = network_get_response();
        const char *resp_text = resp ? resp->data : NULL;
        if (resp_text && !pretty) {
            pretty = pretty_print_json(resp_text);
        }
        const char *show = pretty ? pretty : (resp_text ? resp_text : "(no response yet)");
        
        DrawTextRecWrapped(GetFontDefault(), show,
                          (Rectangle){displayRect.x + 6, displayRect.y + 6 + response_scroll_y,
                                    displayRect.width - 12, displayRect.height - 12},
                          12, 2, true, RAYWHITE);
        EndScissorMode();

        // Instructions
        DrawText("Click to focus input areas. Use mouse wheel to scroll. Cmd/Ctrl+V to paste.", 
                20, screenH - 18, 10, DARKGRAY);

        EndDrawing();
    }

    // Cleanup
    if (pretty) free(pretty);
    TextBuffer_Free(url);
    TextBuffer_Free(body);
    network_cleanup();
    CloseWindow();
    return 0;
}
