#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include "canvas.h"

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    Point a;
    Point b;
    Point c;
} Triangle;

// Core line & triangle primitives
void draw_line(Point p0, Point p1, uint8_t r, uint8_t g, uint8_t blue, float alpha);
void draw_triangle(Point a, Point b, Point c, uint8_t r, uint8_t g, uint8_t blue, float alpha);
void draw_polyline(const Point *pts, int count, int close, uint8_t r, uint8_t g, uint8_t blue, float alpha);

// Algorithmic triangulation
int make_strip(const Point *rowA, int lenA, const Point *rowB, int lenB, Triangle *outTris, int maxTris);
int make_fan(Point center, const Point *ring, int count, Triangle *outTris, int maxTris);

// Ray-casting geometry test
int point_in_polygon(Point pt, const Point *poly, int count);

// LCG Deterministic PRNG
void seed_random(uint32_t seed);
float lcg_random(void);

// Grid generation
int jittered_grid_triangles(
    float step,
    float jitter,
    const Point *poly,
    int polyCount,
    Triangle *outTris,
    int maxTris,
    float startX,
    float startY,
    int cols,
    int rows
);

// Scanline polygon fill for soft shading
void fill_polygon(const Point *pts, int count, uint8_t r, uint8_t g, uint8_t b, float alpha);

// Flatten quadratic bezier
int sample_quadratic_bezier(Point p0, Point cp, Point p1, int segments, Point *outPts, int maxPts);

#endif /* DRAW_H */
