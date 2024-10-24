#include "util.h"
#include "stdlib.h"

const int wf_dirx[] = {-1, 1, 0, 0};
const int wf_diry[] = {0, 0, -1, 1};
void draw_line(float* buf, int x0, int y0, int x1, int y1, int width, float val, wf_draw_mode draw_mode) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    int dLong = abs(dx);
    int dShort = abs(dy);

    int offsetLong = dx > 0 ? 1 : -1;
    int offsetShort = dy > 0 ? width : -width;

    if(dLong < dShort) {
        dShort = dShort ^ dLong;
        dLong = dShort ^ dLong;
        dShort = dShort ^ dLong;

        offsetShort = offsetShort ^ offsetLong;
        offsetLong = offsetShort ^ offsetLong;
        offsetShort = offsetShort ^ offsetLong;
    }

    int error = 2 * dShort - dLong;
    int index = y0 * width + x0;
    const int offset[] = {offsetLong, offsetLong + offsetShort};
    const int abs_d[]  = {2 * dShort, 2 * (dShort - dLong)};
    for (int i = 0; i <= dLong; ++i) {
        if (draw_mode == WF_DRAW_MODE_ADDITIVE) {
            buf[index] += val;  // or a call to your painting method
        } else if (draw_mode == WF_DRAW_MODE_NORMAL) {
            buf[index] = val;  // or a call to your painting method
        }
        const int errorIsTooBig = error >= 0;
        index += offset[errorIsTooBig];
        error += abs_d[errorIsTooBig];
    }
}
