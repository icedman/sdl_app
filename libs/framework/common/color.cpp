#include "color.h"

#include <cstdio>

color_t color_darker(color_t clr, int amount)
{
    clr.r -= amount;
    clr.g -= amount;
    clr.b -= amount;
    if (clr.r < 0)
        clr.r = 0;
    if (clr.g < 0)
        clr.g = 0;
    if (clr.b < 0)
        clr.b = 0;
    return clr;
}

color_t color_lighter(color_t clr, int amount)
{
    clr.r += amount;
    clr.g += amount;
    clr.b += amount;
    if (clr.r > 255)
        clr.r = 255;
    if (clr.g > 255)
        clr.g = 255;
    if (clr.b > 255)
        clr.b = 255;
    return clr;
}

bool color_is_dark(color_t clr)
{
    int luma = 0.2126 * clr.r + 0.7152 * clr.g + 0.0722 * clr.b; // per ITU-R BT.709
    return luma < 128;
}