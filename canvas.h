#ifndef CANVAS_H
#define CANVAS_H

#include <stdint.h>

#define CANVAS_WIDTH 615
#define CANVAS_HEIGHT 773

extern uint8_t rgb[CANVAS_HEIGHT][CANVAS_WIDTH][3];

void canvas_init(uint8_t r, uint8_t g, uint8_t b);
void canvas_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, float alpha);
void canvas_fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, float alpha);

int canvas_save_ppm(const char *filename);
int canvas_save_bmp(const char *filename);
void canvas_print_terminal(int term_cols);

#endif /* CANVAS_H */
