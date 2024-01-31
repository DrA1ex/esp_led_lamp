#include "palette.h"

#include "utils/palette.h"

CRGBPalette16 Custom_p;

const TProgmemRGBGradientPalette_byte Fire_gp[] = {
        0, 0, 0, 0,
        128, 255, 0, 0,
        224, 255, 255, 0,
        255, 255, 255, 255
};

const TProgmemRGBGradientPalette_byte Sunset_Real_gp[] = {
        0, 120, 0, 0,
        22, 179, 22, 0,
        51, 255, 104, 0,
        85, 167, 22, 18,
        135, 100, 0, 103,
        198, 16, 0, 130,
        255, 0, 0, 160
};

const TProgmemRGBGradientPalette_byte Dkbluered_gp[] = {
        0, 1, 0, 4,
        8, 1, 0, 13,
        17, 1, 0, 29,
        25, 1, 0, 52,
        33, 1, 0, 83,
        42, 1, 0, 123,
        51, 1, 0, 174,
        59, 1, 0, 235,
        68, 1, 2, 255,
        76, 4, 17, 255,
        84, 16, 45, 255,
        93, 37, 82, 255,
        102, 69, 127, 255,
        110, 120, 168, 255,
        119, 182, 217, 255,
        127, 255, 255, 255,
        135, 255, 217, 184,
        144, 255, 168, 123,
        153, 255, 127, 73,
        161, 255, 82, 40,
        170, 255, 45, 18,
        178, 255, 17, 5,
        186, 255, 2, 1,
        195, 234, 0, 1,
        204, 171, 0, 1,
        212, 120, 0, 1,
        221, 79, 0, 1,
        229, 48, 0, 1,
        237, 26, 0, 1,
        246, 12, 0, 1,
        255, 4, 0, 1
};
const TProgmemRGBGradientPalette_byte Optimus_Prime_gp[] = {
        0, 5, 16, 18,
        25, 5, 16, 18,
        51, 7, 25, 39,
        76, 8, 38, 71,
        102, 64, 99, 106,
        127, 194, 189, 151,
        153, 182, 63, 42,
        178, 167, 6, 2,
        204, 100, 3, 1,
        229, 53, 1, 1,
        255, 53, 1, 1
};

const TProgmemRGBGradientPalette_byte WarmGrad_gp[] = {
        0, 252, 252, 172,
        25, 239, 255, 61,
        53, 247, 45, 17,
        76, 197, 82, 19,
        96, 239, 255, 61,
        124, 83, 4, 1,
        153, 247, 45, 17,
        214, 23, 15, 17,
        255, 1, 1, 1
};

const TProgmemRGBGradientPalette_byte ColdGrad_gp[] = {
        0, 66, 186, 192,
        43, 1, 22, 71,
        79, 2, 104, 142,
        117, 66, 186, 192,
        147, 2, 104, 142,
        186, 1, 22, 71,
        224, 2, 104, 142,
        255, 4, 27, 28
};

const TProgmemRGBGradientPalette_byte HotGrad_gp[] = {
        0, 157, 21, 2,
        35, 229, 244, 16,
        73, 255, 44, 7,
        107, 142, 7, 1,
        153, 229, 244, 16,
        206, 142, 7, 1,
        255, 135, 36, 0
};


const TProgmemRGBGradientPalette_byte PinkGrad_gp[] = {
        0, 249, 32, 145,
        28, 208, 1, 7,
        43, 249, 1, 19,
        56, 126, 152, 10,
        73, 234, 23, 84,
        89, 224, 45, 119,
        107, 232, 127, 158,
        127, 244, 13, 89,
        150, 188, 6, 52,
        175, 177, 70, 14,
        221, 194, 1, 8,
        255, 112, 0, 1
};

const TProgmemRGBGradientPalette_byte Comfy_gp[] = {
        0, 255, 255, 45,
        43, 208, 93, 1,
        137, 224, 1, 242,
        181, 159, 1, 29,
        255, 63, 4, 68
};

const TProgmemRGBGradientPalette_byte Cyberpunk_gp[] = {
        0, 3, 6, 72,
        38, 12, 50, 188,
        109, 217, 35, 1,
        135, 242, 175, 12,
        178, 161, 32, 87,
        255, 24, 6, 108
};

const TProgmemRGBGradientPalette_byte Girl_gp[] = {
        0, 103, 1, 10,
        33, 109, 1, 12,
        76, 159, 5, 48,
        119, 175, 55, 103,
        127, 175, 55, 103,
        178, 159, 5, 48,
        221, 109, 1, 12,
        255, 103, 1, 10
};

const TProgmemRGBGradientPalette_byte Xmas_gp[] = {
        0, 0, 12, 0,
        40, 0, 55, 0,
        66, 1, 117, 2,
        77, 1, 84, 1,
        81, 0, 55, 0,
        119, 0, 12, 0,
        153, 42, 0, 0,
        181, 121, 0, 0,
        204, 255, 12, 8,
        224, 121, 0, 0,
        244, 42, 0, 0,
        255, 42, 0, 0
};

const TProgmemRGBGradientPalette_byte Acid_gp[] = {
        0, 0, 12, 0,
        61, 153, 239, 112,
        127, 0, 12, 0,
        165, 106, 239, 2,
        196, 167, 229, 71,
        229, 106, 239, 2,
        255, 0, 12, 0
};


const TProgmemRGBGradientPalette_byte BlueSmoke_gp[] = {
        0, 0, 0, 0,
        12, 1, 1, 3,
        53, 8, 1, 22,
        80, 4, 6, 89,
        119, 2, 25, 216,
        145, 7, 10, 99,
        186, 15, 2, 31,
        233, 2, 1, 5,
        255, 0, 0, 0
};

const TProgmemRGBGradientPalette_byte Gummy_gp[] = {
        0, 8, 47, 5,
        31, 77, 122, 6,
        63, 249, 237, 7,
        95, 232, 51, 1,
        127, 215, 0, 1,
        159, 47, 1, 3,
        191, 1, 7, 16,
        223, 52, 22, 6,
        255, 239, 45, 1,
};

const TProgmemRGBGradientPalette_byte Leo_gp[] = {
        0, 0, 0, 0,
        16, 0, 0, 0,
        32, 0, 0, 0,
        18, 0, 0, 0,
        64, 16, 8, 0,
        80, 80, 40, 0,
        96, 16, 8, 0,
        112, 0, 0, 0,
        128, 0, 0, 0,
        144, 0, 0, 0,
        160, 0, 0, 0,
        176, 0, 0, 0,
        192, 0, 0, 0,
        208, 0, 0, 0,
        224, 0, 0, 0,
        240, 0, 0, 0,
        255, 0, 0, 0,
};

const TProgmemRGBGradientPalette_byte Aurora_gp[] = {
        0, 17, 177, 13,    //Greenish
        64, 121, 242, 5,    //Greenish
        128, 25, 173, 121,    //Turquoise
        192, 250, 77, 127,    //Pink
        255, 171, 101, 221     //Purple
};

const TProgmemRGBPalette16 WoodFireColors_p = {
        CRGB::Black, 0x330e00, 0x661c00, 0x992900, 0xcc3700,
        CRGB::OrangeRed, 0xff5800, 0xff6b00, 0xff7f00, 0xff9200,
        CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300, 0xffcd00,
        CRGB::Gold
};

const TProgmemRGBPalette16 NormalFire_p = {
        CRGB::Black, 0x330000, 0x660000, 0x990000, 0xcc0000, CRGB::Red,
        0xff0c00, 0xff1800, 0xff2400, 0xff3000, 0xff3c00, 0xff4800, 0xff5400,
        0xff6000, 0xff6c00, 0xff7800
};

const TProgmemRGBPalette16 LithiumFireColors_p = {
        CRGB::Black, 0x240707, 0x470e0e, 0x6b1414, 0x8e1b1b,
        CRGB::FireBrick, 0xc14244, 0xd16166, 0xe08187, 0xf0a0a9,
        CRGB::Pink, 0xff9ec0, 0xff7bb5, 0xff59a9, 0xff369e,
        CRGB::DeepPink
};

const TProgmemRGBPalette16 SodiumFireColors_p = {
        CRGB::Black, 0x332100, 0x664200, 0x996300, 0xcc8400,
        CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300, 0xffcd00,
        CRGB::Gold, 0xf8cd06, 0xf0c30d, 0xe9b913, 0xe1af1a,
        CRGB::Goldenrod
};

const TProgmemRGBPalette16 CopperFireColors_p = {
        CRGB::Black, 0x001a00, 0x003300, 0x004d00, 0x006600,
        CRGB::Green, 0x239909, 0x45b313, 0x68cc1c, 0x8ae626,
        CRGB::GreenYellow, 0x94f530, 0x7ceb30, 0x63e131, 0x4bd731,
        CRGB::LimeGreen
};

const TProgmemRGBPalette16 AlcoholFireColors_p = {
        CRGB::Black, 0x000033, 0x000066, 0x000099, 0x0000cc,
        CRGB::Blue, 0x0026ff, 0x004cff, 0x0073ff, 0x0099ff,
        CRGB::DeepSkyBlue, 0x1bc2fe, 0x36c5fd, 0x51c8fc, 0x6ccbfb,
        CRGB::LightSkyBlue
};


const TProgmemRGBGradientPalette_byte CornField_gp[] = {
        0, 0x07, 0x15, 0x04,
        28, 0x18, 0x2a, 0x04,
        2 * 28, 0x37, 0x51, 0x11,
        3 * 28, 0x76, 0x96, 0x4b,
        4 * 28, 0x6c, 0xa3, 0x78,
        5 * 28, 0x42, 0x8d, 0x8d,
        6 * 28, 0x09, 0x4e, 0x6e,
        7 * 28, 0x2b, 0x3f, 0x45,
        8 * 28, 0x4e, 0x3d, 0x20,
        255, 0x77, 0x6e, 0x29,
};

const TProgmemRGBGradientPalette_byte Gargantua_gp[] = {
        RGB_PALETTE_9(0x272121, 0x52403e, 0x785c58, 0x6d4d3e, 0x542507, 0xdfafa3, 0xd36323, 0xfa9452, 0xf2eaf5)
};

const TProgmemRGBGradientPalette_byte MoonLight_gp[] = {
        RGB_PALETTE_12(0x020514, 0x09213d, 0x023465, 0x035899, 0x4f83bf, 0x639bd6,
                       0xa8c6e8, 0xc3d7f0, 0xd4dce9, 0xbdbed3, 0xa7a4c3, 0x918bad)
};

const TProgmemRGBGradientPalette_byte LastNight_gp[] = {
        RGB_PALETTE_10(0x030418, 0x040e56, 0x103a9e, 0x1c5ee6, 0x51bbf9, 0x80e5f9, 0xea88f5, 0xe93ddf, 0x9252a9, 0x6c4284)
};

const TProgmemRGBGradientPalette_byte Rhapsody_gp[] = {
        RGB_PALETTE_9(0x192229, 0x60706f, 0xb5c0bc, 0xa57a8d, 0x953752, 0x482545, 0x14183d, 0x1b366b, 0x5ca2bc)
};

const TProgmemRGBGradientPalette_byte Galaxy_gp[] = {
        RGB_PALETTE_5(0x000f26, 0x3f4775, 0x9a6f9c, 0xf9b0a9, 0xffd6d2)
};

const TProgmemRGBGradientPalette_byte NeonStreet_gp[] = {
        RGB_PALETTE_5(0x221e41, 0x3b3f88, 0x7089c1, 0xc51c6d, 0xaf2499)
};

const TProgmemRGBGradientPalette_byte Moody_gp[] = {
        RGB_PALETTE_5(0x253334, 0xcd423b, 0x496c6e, 0x1a1e1f, 0x373739)
};

const TProgmemRGBGradientPalette_byte Fall_gp[] = {
        RGB_PALETTE_5(0x5f0f40, 0x9a031e, 0xfb8b24, 0xe36414, 0x0f4c5c)
};

const TProgmemRGBGradientPalette_byte Matrix_gp[] = {
        RGB_PALETTE_8(0x151a16, 0x1e2a26, 0x23342c, 0x334a40, 0x3e5648, 0x3f7156, 0x5e8358, 0x6d9089)
};

const TProgmemRGBGradientPalette_byte Joker_gp[] = {
        RGB_PALETTE_10(0x051514, 0x10324b, 0x00454a, 0x00b29c, 0xd7eccb, 0xe1b882, 0xe06d34, 0xd85f57, 0xa04938, 0x6d271d)
};

const TProgmemRGBGradientPalette_byte BladeRunner_gp[] = {
        RGB_PALETTE_10(0x1a051d, 0x35083f, 0x780153, 0xb80e86, 0xdd5db2, 0x885bc2, 0x5625a1, 0x291689, 0x190657, 0x456274)
};