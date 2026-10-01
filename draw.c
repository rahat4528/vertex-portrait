#include "draw.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint32_t lcg_state = 7;

void seed_random(uint32_t seed) {
    lcg_state = seed;
}

float lcg_random(void) {
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return (float)lcg_state / 4294967296.0f;
}

void draw_line(Point p0, Point p1, uint8_t r, uint8_t g, uint8_t blue, float alpha) {
    int x0 = (int)roundf(p0.x);
    int y0 = (int)roundf(p0.y);
    int x1 = (int)roundf(p1.x);
    int y1 = (int)roundf(p1.y);

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        canvas_set_pixel(x0, y0, r, g, blue, alpha);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_triangle(Point a, Point b, Point c, uint8_t r, uint8_t g, uint8_t blue, float alpha) {
    draw_line(a, b, r, g, blue, alpha);
    draw_line(b, c, r, g, blue, alpha);
    draw_line(c, a, r, g, blue, alpha);
}

void draw_polyline(const Point *pts, int count, int close, uint8_t r, uint8_t g, uint8_t blue, float alpha) {
    if (count < 2) return;
    for (int i = 0; i < count - 1; i++) {
        draw_line(pts[i], pts[i + 1], r, g, blue, alpha);
    }
    if (close) {
        draw_line(pts[count - 1], pts[0], r, g, blue, alpha);
    }
}

static float tri_area(Point p0, Point p1, Point p2) {
    return 0.5f * fabsf(p0.x * (p1.y - p2.y) + p1.x * (p2.y - p0.y) + p2.x * (p0.y - p1.y));
}

int make_strip(const Point *rowA, int lenA, const Point *rowB, int lenB, Triangle *outTris, int maxTris) {
    if (lenA < 2 && lenB < 2) return 0;
    int count = 0;
    int i = 0;
    int j = 0;

    while (i < lenA - 1 || j < lenB - 1) {
        Triangle candidate;
        if (i == lenA - 1) {
            candidate = (Triangle){ rowA[i], rowB[j], rowB[j + 1] };
            j++;
        } else if (j == lenB - 1) {
            candidate = (Triangle){ rowA[i], rowA[i + 1], rowB[j] };
            i++;
        } else {
            float progA = (float)(i + 1) / (float)(lenA - 1);
            float progB = (float)(j + 1) / (float)(lenB - 1);

            if (progA <= progB) {
                candidate = (Triangle){ rowA[i], rowA[i + 1], rowB[j] };
                i++;
            } else {
                candidate = (Triangle){ rowA[i], rowB[j], rowB[j + 1] };
                j++;
            }
        }

        if (tri_area(candidate.a, candidate.b, candidate.c) > 0.1f) {
            if (count < maxTris) {
                outTris[count++] = candidate;
            }
        }
    }
    return count;
}

int make_fan(Point center, const Point *ring, int count, Triangle *outTris, int maxTris) {
    int n = 0;
    for (int k = 0; k < count; k++) {
        if (n < maxTris) {
            outTris[n++] = (Triangle){ center, ring[k], ring[(k + 1) % count] };
        }
    }
    return n;
}

int point_in_polygon(Point pt, const Point *poly, int count) {
    int inside = 0;
    for (int i = 0, j = count - 1; i < count; j = i++) {
        float xi = poly[i].x, yi = poly[i].y;
        float xj = poly[j].x, yj = poly[j].y;
        int intersect = ((yi > pt.y) != (yj > pt.y)) &&
                        (pt.x < (xj - xi) * (pt.y - yi) / (yj - yi) + xi);
        if (intersect) inside = !inside;
    }
    return inside;
}

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
) {
    Point grid[16][16];
    if (cols > 16) cols = 16;
    if (rows > 16) rows = 16;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float baseX = startX + (float)c * step;
            float baseY = startY + (float)r * step;
            float offX = (lcg_random() - 0.5f) * jitter;
            float offY = (lcg_random() - 0.5f) * jitter;
            grid[r][c] = (Point){ baseX + offX, baseY + offY };
        }
    }

    int count = 0;
    for (int r = 0; r < rows - 1; r++) {
        for (int c = 0; c < cols - 1; c++) {
            Point a = grid[r][c];
            Point b = grid[r][c + 1];
            Point d = grid[r + 1][c];
            Point e = grid[r + 1][c + 1];

            Point c1 = (Point){ (a.x + b.x + d.x) / 3.0f, (a.y + b.y + d.y) / 3.0f };
            if (point_in_polygon(c1, poly, polyCount)) {
                if (count < maxTris) {
                    outTris[count++] = (Triangle){ a, b, d };
                }
            }

            Point c2 = (Point){ (b.x + e.x + d.x) / 3.0f, (b.y + e.y + d.y) / 3.0f };
            if (point_in_polygon(c2, poly, polyCount)) {
                if (count < maxTris) {
                    outTris[count++] = (Triangle){ b, e, d };
                }
            }
        }
    }
    return count;
}

static int compare_floats(const void *a, const void *b) {
    float fa = *(const float *)a;
    float fb = *(const float *)b;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

void fill_polygon(const Point *pts, int count, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    if (count < 3) return;

    float minY = pts[0].y;
    float maxY = pts[0].y;
    for (int i = 1; i < count; i++) {
        if (pts[i].y < minY) minY = pts[i].y;
        if (pts[i].y > maxY) maxY = pts[i].y;
    }

    int startY = (int)floorf(minY);
    int endY = (int)ceilf(maxY);
    if (startY < 0) startY = 0;
    if (endY >= CANVAS_HEIGHT) endY = CANVAS_HEIGHT - 1;

    float intersections[256];

    for (int y = startY; y <= endY; y++) {
        float scanY = (float)y + 0.5f;
        int interCount = 0;

        for (int i = 0, j = count - 1; i < count; j = i++) {
            float yi = pts[i].y;
            float yj = pts[j].y;
            if ((yi <= scanY && yj > scanY) || (yj <= scanY && yi > scanY)) {
                float xi = pts[i].x;
                float xj = pts[j].x;
                float interX = xi + (scanY - yi) * (xj - xi) / (yj - yi);
                if (interCount < 256) {
                    intersections[interCount++] = interX;
                }
            }
        }

        if (interCount > 1) {
            qsort(intersections, interCount, sizeof(float), compare_floats);
            for (int k = 0; k < interCount - 1; k += 2) {
                int x0 = (int)ceilf(intersections[k]);
                int x1 = (int)floorf(intersections[k + 1]);
                if (x0 < 0) x0 = 0;
                if (x1 >= CANVAS_WIDTH) x1 = CANVAS_WIDTH - 1;
                for (int x = x0; x <= x1; x++) {
                    canvas_set_pixel(x, y, r, g, b, alpha);
                }
            }
        }
    }
}

int sample_quadratic_bezier(Point p0, Point cp, Point p1, int segments, Point *outPts, int maxPts) {
    if (segments < 1) segments = 1;
    int count = 0;
    for (int i = 0; i <= segments; i++) {
        if (count >= maxPts) break;
        float t = (float)i / (float)segments;
        float inv = 1.0f - t;
        float x = inv * inv * p0.x + 2.0f * inv * t * cp.x + t * t * p1.x;
        float y = inv * inv * p0.y + 2.0f * inv * t * cp.y + t * t * p1.y;
        outPts[count++] = (Point){ x, y };
    }
    return count;
}
