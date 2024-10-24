#ifndef WF_UTIL_H
#define WF_UTIL_H

#define is_in_bounds(X, Y, WIDTH, HEIGHT) ((X) >= 0 && (X) < (WIDTH) && (Y) >= 0 && (Y) < (HEIGHT))
#define row_col_to_index(ROW, COL, WIDTH) ((ROW) * (WIDTH) + (COL))
#define rand_range(MIN, MAX) ((MIN) + rand() / (RAND_MAX / ((MAX) - (MIN) + 1) + 1))
#define clamp(VAL, MIN, MAX) ((VAL) < (MIN) ? (MIN) : (VAL) > (MAX) ? (MAX) : (VAL))
typedef enum {
    WF_DRAW_MODE_NORMAL = 0,
    WF_DRAW_MODE_ADDITIVE = 1 << 1,
} wf_draw_mode;
extern const int wf_dirx[];
extern const int wf_diry[];
extern void draw_line(float* buf, int x0, int y0, int x1, int y1, int width, float val, wf_draw_mode draw_mode);
#endif
