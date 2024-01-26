#pragma once

#include "misc/led.h"


#define RGB_ENTRY(IND, CNT, rgb) (IND != CNT-1 ? IND * (255 / CNT) : 255), rgb >> 16 & 0xff, rgb >> 8 & 0xff, rgb & 0xff

#define __RGB_PALETTE_16(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_15(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_15(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_14(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_14(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_13(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_13(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_12(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_12(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_11(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_11(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_10(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_10(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_9(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_9(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_8(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_8(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_7(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_7(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_6(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_6(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_5(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_5(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_4(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_4(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_3(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_3(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_2(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_2(IND, CNT, rgb, ...) RGB_ENTRY(IND, CNT, rgb), __RGB_PALETTE_1(IND+1, CNT, __VA_ARGS__)
#define __RGB_PALETTE_1(IND, CNT, rgb) RGB_ENTRY(IND, CNT, rgb)

#define RGB_PALETTE_16(...) __RGB_PALETTE_16(0, 16, __VA_ARGS__)
#define RGB_PALETTE_15(...) __RGB_PALETTE_15(0, 15, __VA_ARGS__)
#define RGB_PALETTE_14(...) __RGB_PALETTE_14(0, 14, __VA_ARGS__)
#define RGB_PALETTE_13(...) __RGB_PALETTE_13(0, 13, __VA_ARGS__)
#define RGB_PALETTE_12(...) __RGB_PALETTE_12(0, 12, __VA_ARGS__)
#define RGB_PALETTE_11(...) __RGB_PALETTE_11(0, 11, __VA_ARGS__)
#define RGB_PALETTE_10(...) __RGB_PALETTE_10(0, 10, __VA_ARGS__)
#define RGB_PALETTE_9(...) __RGB_PALETTE_9(0, 9, __VA_ARGS__)
#define RGB_PALETTE_8(...) __RGB_PALETTE_8(0, 8, __VA_ARGS__)
#define RGB_PALETTE_7(...) __RGB_PALETTE_7(0, 7, __VA_ARGS__)
#define RGB_PALETTE_6(...) __RGB_PALETTE_6(0, 6, __VA_ARGS__)
#define RGB_PALETTE_5(...) __RGB_PALETTE_5(0, 5, __VA_ARGS__)
#define RGB_PALETTE_4(...) __RGB_PALETTE_4(0, 4, __VA_ARGS__)
#define RGB_PALETTE_3(...) __RGB_PALETTE_3(0, 3, __VA_ARGS__)
#define RGB_PALETTE_2(...) __RGB_PALETTE_2(0, 2, __VA_ARGS__)


CRGB color_from_palette(const CRGBPalette16 &pal, uint16_t index, uint8_t scale = 255);
uint16_t inoise_hd(double x, double y, double z);
uint16_t inoise_hd(double x, double y);