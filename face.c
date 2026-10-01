#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "canvas.h"
#include "draw.h"
#include "face_mesh_data.h"

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Render the smooth crescent highlight shapes
static void render_shading(void) {
    uint8_t sr = 255, sg = 255, sb = 255;
    float sa = 0.08f;

    // Jawline & lower cheek crescent curve
    Point shapeA_pts[80];
    int countA = 0;
    countA += sample_quadratic_bezier((Point){190, 560}, (Point){350, 590}, (Point){535, 380}, 30, shapeA_pts + countA, 80 - countA);
    countA += sample_quadratic_bezier((Point){535, 380}, (Point){380, 530}, (Point){190, 560}, 30, shapeA_pts + countA, 80 - countA);
    fill_polygon(shapeA_pts, countA, sr, sg, sb, sa);

    // Cheekbone crescent curve
    Point shapeB_pts[60];
    int countB = 0;
    countB += sample_quadratic_bezier((Point){390, 380}, (Point){470, 350}, (Point){525, 280}, 24, shapeB_pts + countB, 60 - countB);
    countB += sample_quadratic_bezier((Point){525, 280}, (Point){470, 365}, (Point){390, 380}, 24, shapeB_pts + countB, 60 - countB);
    fill_polygon(shapeB_pts, countB, sr, sg, sb, sa * 0.85f);
}

// Subpixel Braille Terminal Renderer in C
static void print_terminal_mesh(void) {
    const int termCols = 78;
    const int gridW = termCols * 2;
    const int gridH = (int)round((double)gridW * (773.0 / 615.0));
    const int termRows = (gridH + 3) / 4;

    uint8_t *dots = (uint8_t *)calloc(termCols * termRows, sizeof(uint8_t));
    if (!dots) return;

    // Helper to set subpixel dot
    #define SET_DOT(gx, gy) do { \
        int _gx = (gx), _gy = (gy); \
        if (_gx >= 0 && _gx < gridW && _gy >= 0 && _gy < gridH) { \
            int _col = _gx / 2; \
            int _row = _gy / 4; \
            int _dx = _gx % 2; \
            int _dy = _gy % 4; \
            uint8_t _bit = 0; \
            if (_dx == 0 && _dy == 0) _bit = 0x01; \
            else if (_dx == 0 && _dy == 1) _bit = 0x02; \
            else if (_dx == 0 && _dy == 2) _bit = 0x04; \
            else if (_dx == 1 && _dy == 0) _bit = 0x08; \
            else if (_dx == 1 && _dy == 1) _bit = 0x10; \
            else if (_dx == 1 && _dy == 2) _bit = 0x20; \
            else if (_dx == 0 && _dy == 3) _bit = 0x40; \
            else if (_dx == 1 && _dy == 3) _bit = 0x80; \
            dots[_row * termCols + _col] |= _bit; \
        } \
    } while (0)

    // Top purple bar
    for (int x = 0; x < gridW; x++) {
        SET_DOT(x, 1);
    }

    // Draw all edges with Bresenham on the Braille dot grid
    for (int i = 0; i < NUM_EDGES; i++) {
        int v0 = FACE_EDGES[i][0];
        int v1 = FACE_EDGES[i][1];
        int gx0 = (int)round((double)FACE_VERTICES[v0].x * (gridW - 1) / 615.0);
        int gy0 = (int)round((double)FACE_VERTICES[v0].y * (gridH - 1) / 773.0);
        int gx1 = (int)round((double)FACE_VERTICES[v1].x * (gridW - 1) / 615.0);
        int gy1 = (int)round((double)FACE_VERTICES[v1].y * (gridH - 1) / 773.0);

        int dx = abs(gx1 - gx0);
        int dy = -abs(gy1 - gy0);
        int sx = gx0 < gx1 ? 1 : -1;
        int sy = gy0 < gy1 ? 1 : -1;
        int err = dx + dy;

        while (1) {
            SET_DOT(gx0, gy0);
            if (gx0 == gx1 && gy0 == gy1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; gx0 += sx; }
            if (e2 <= dx) { err += dx; gy0 += sy; }
        }
    }

    printf("\n\033[1;35m================================================================================\033[0m\n");
    printf("\033[1;37m        LOW-POLY WIREFRAME FACE PORTRAIT (VECTOR TERMINAL RENDER)               \033[0m\n");
    printf("\033[1;35m================================================================================\033[0m\n\n");

    for (int r = 0; r < termRows; r++) {
        for (int c = 0; c < termCols; c++) {
            uint8_t mask = dots[r * termCols + c];
            if (mask == 0) {
                putchar(' ');
            } else {
                // Unicode Braille Patterns: U+2800 + mask
                // In UTF-8: 0xE2, 0xA0 | (mask >> 6), 0x80 | (mask & 0x3F)
                unsigned int codepoint = 0x2800 + mask;
                unsigned char utf8[4];
                utf8[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
                utf8[1] = 0x80 | ((codepoint >> 6) & 0x3F);
                utf8[2] = 0x80 | (codepoint & 0x3F);
                utf8[3] = '\0';
                if (r == 0) {
                    printf("\033[38;2;160;90;190m%s\033[0m", utf8);
                } else {
                    printf("\033[38;2;215;215;220m%s\033[0m", utf8);
                }
            }
        }
        putchar('\n');
    }

    printf("\n\033[38;2;150;150;160m[Wireframe Vector Mesh | Subpixel Braille %dx%d dots | %d Vertices | %d Edges]\033[0m\n\n",
           gridW, gridH, NUM_VERTICES, NUM_EDGES);

    free(dots);
}

int main(int argc, char **argv) {
    int terminal_only = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--print") == 0 || strcmp(argv[i], "--terminal") == 0) {
            terminal_only = 1;
        }
    }

    // 1. Solid background #2F2E33 (RGB: 47, 46, 51)
    canvas_init(0x2F, 0x2E, 0x33);

    // 2. Purple top bar #5A3A6A (RGB: 90, 58, 106, x 0..615, y 0..8)
    canvas_fill_rect(0, 0, CANVAS_WIDTH, 8, 0x5A, 0x3A, 0x6A, 1.0f);

    // 3. Soft shading crescent shapes
    render_shading();

    // 4. Wireframe lines
    uint8_t lr = 215, lg = 215, lb = 220;
    float la = 0.90f;

    for (int i = 0; i < NUM_EDGES; i++) {
        int v0 = FACE_EDGES[i][0];
        int v1 = FACE_EDGES[i][1];
        draw_line(FACE_VERTICES[v0], FACE_VERTICES[v1], lr, lg, lb, la);
    }

    // 5. Save BMP & PPM files
    const char *bmp_path = "face.bmp";
    const char *ppm_path = "face.ppm";

    if (canvas_save_bmp(bmp_path)) {
        printf("Successfully wrote %s (%dx%d 24-bit RGB BMP)\n", bmp_path, CANVAS_WIDTH, CANVAS_HEIGHT);
    } else {
        fprintf(stderr, "Failed to write %s\n", bmp_path);
    }

    if (canvas_save_ppm(ppm_path)) {
        printf("Successfully wrote %s (%dx%d binary PPM P6)\n", ppm_path, CANVAS_WIDTH, CANVAS_HEIGHT);
    } else {
        fprintf(stderr, "Failed to write %s\n", ppm_path);
    }

    // 6. Direct print to terminal
    print_terminal_mesh();

    return 0;
}
