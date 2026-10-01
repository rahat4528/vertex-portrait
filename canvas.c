#include "canvas.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

uint8_t rgb[CANVAS_HEIGHT][CANVAS_WIDTH][3];

void canvas_init(uint8_t r, uint8_t g, uint8_t b) {
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            rgb[y][x][0] = r;
            rgb[y][x][1] = g;
            rgb[y][x][2] = b;
        }
    }
}

void canvas_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;
    if (alpha <= 0.0f) return;
    if (alpha > 1.0f) alpha = 1.0f;

    float bg_r = (float)rgb[y][x][0];
    float bg_g = (float)rgb[y][x][1];
    float bg_b = (float)rgb[y][x][2];

    float out_r = (1.0f - alpha) * bg_r + alpha * (float)r;
    float out_g = (1.0f - alpha) * bg_g + alpha * (float)g;
    float out_b = (1.0f - alpha) * bg_b + alpha * (float)b;

    rgb[y][x][0] = (uint8_t)(out_r + 0.5f);
    rgb[y][x][1] = (uint8_t)(out_g + 0.5f);
    rgb[y][x][2] = (uint8_t)(out_b + 0.5f);
}

void canvas_fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            canvas_set_pixel(i, j, r, g, b, alpha);
        }
    }
}

int canvas_save_ppm(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;
    fprintf(fp, "P6\n%d %d\n255\n", CANVAS_WIDTH, CANVAS_HEIGHT);
    fwrite(rgb, 1, CANVAS_WIDTH * CANVAS_HEIGHT * 3, fp);
    fclose(fp);
    return 1;
}

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BMPFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

int canvas_save_bmp(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;

    int row_stride = ((CANVAS_WIDTH * 3 + 3) / 4) * 4;
    int padding = row_stride - CANVAS_WIDTH * 3;
    uint32_t image_size = (uint32_t)(row_stride * CANVAS_HEIGHT);

    BMPFileHeader fileHeader;
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + image_size;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    BMPInfoHeader infoHeader;
    memset(&infoHeader, 0, sizeof(infoHeader));
    infoHeader.biSize = sizeof(BMPInfoHeader);
    infoHeader.biWidth = CANVAS_WIDTH;
    infoHeader.biHeight = CANVAS_HEIGHT; // Bottom-up
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0; // BI_RGB
    infoHeader.biSizeImage = image_size;
    infoHeader.biXPelsPerMeter = 2835;
    infoHeader.biYPelsPerMeter = 2835;

    fwrite(&fileHeader, sizeof(fileHeader), 1, fp);
    fwrite(&infoHeader, sizeof(infoHeader), 1, fp);

    uint8_t pad_bytes[4] = {0, 0, 0, 0};

    // Bottom-to-top row scan
    for (int y = CANVAS_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            uint8_t b = rgb[y][x][2];
            uint8_t g = rgb[y][x][1];
            uint8_t r = rgb[y][x][0];
            fputc(b, fp);
            fputc(g, fp);
            fputc(r, fp);
        }
        if (padding > 0) {
            fwrite(pad_bytes, 1, padding, fp);
        }
    }

    fclose(fp);
    return 1;
}

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>

void canvas_print_terminal(int term_cols) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (term_cols <= 0) term_cols = 80;
    int term_rows = (int)roundf((float)term_cols * ((float)CANVAS_HEIGHT / (float)CANVAS_WIDTH) * 0.58f);
    int grid_w = term_cols * 2;
    int grid_h = term_rows * 4;

    uint8_t *dots = (uint8_t *)calloc(term_cols * term_rows, sizeof(uint8_t));
    if (!dots) return;

    // Scan the canvas RGB buffer to map lines to the braille grid
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        int gy = (y * (grid_h - 1)) / (CANVAS_HEIGHT - 1);
        int row = gy / 4;
        int dy = gy % 4;

        for (int x = 0; x < CANVAS_WIDTH; x++) {
            uint8_t r = rgb[y][x][0];
            uint8_t g = rgb[y][x][1];
            uint8_t b = rgb[y][x][2];

            // Wireframe lines have r > 100, g > 100, b > 105 or top bar
            int is_line = (r > 100 && g > 100 && b > 105);
            int is_bar = (y < 8 && r > 70 && b > 80);

            if (is_line || is_bar) {
                int gx = (x * (grid_w - 1)) / (CANVAS_WIDTH - 1);
                int col = gx / 2;
                int dx = gx % 2;

                uint8_t bit = 0;
                if (dx == 0 && dy == 0) bit = 0x01;
                else if (dx == 0 && dy == 1) bit = 0x02;
                else if (dx == 0 && dy == 2) bit = 0x04;
                else if (dx == 1 && dy == 0) bit = 0x08;
                else if (dx == 1 && dy == 1) bit = 0x10;
                else if (dx == 1 && dy == 2) bit = 0x20;
                else if (dx == 0 && dy == 3) bit = 0x40;
                else if (dx == 1 && dy == 3) bit = 0x80;

                dots[row * term_cols + col] |= bit;
            }
        }
    }

    printf("\n\033[1;35m================================================================================\033[0m\n");
    printf("\033[1;37m        LOW-POLY WIREFRAME FACE PORTRAIT (VECTOR TERMINAL RENDER)               \033[0m\n");
    printf("\033[1;35m================================================================================\033[0m\n\n");

    for (int r = 0; r < term_rows; r++) {
        for (int c = 0; c < term_cols; c++) {
            uint8_t mask = dots[r * term_cols + c];
            if (mask == 0) {
                putchar(' ');
            } else {
                if (r == 0) {
                    printf("\033[38;2;160;90;190m");
                } else {
                    printf("\033[38;2;215;215;220m");
                }
                putchar(0xE2);
                putchar(0xA0 | (mask >> 6));
                putchar(0x80 | (mask & 0x3F));
                printf("\033[0m");
            }
        }
        putchar('\n');
    }

    printf("\n\033[38;2;150;150;160m[Wireframe Vector Mesh | Subpixel Braille 160x180 dots | Seed: 7 | 615x773 Canvas]\033[0m\n\n");
    free(dots);
}
