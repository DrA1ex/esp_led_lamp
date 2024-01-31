#include "palette.h"

CRGBPalette16 Custom_p;

const TProgmemRGBPalette16 ElectricBloom_gp = {
        0xffbe0b, 0xfea20a, 0xfd8709, 0xfc6b08,
        0xfb500e, 0xfc3929, 0xfd2245, 0xfe0b60,
        0xee077f, 0xcd16a0, 0xac25c2, 0x8b34e4,
        0x7448f0, 0x615cf5, 0x4d71fa, 0x3a86ff
};

const TProgmemRGBPalette16 OceanicBurst_gp = {
        0x03f9fb, 0x0aeafc, 0x10dafd, 0x17cbfe,
        0x1db5fc, 0x1f8cef, 0x2264e2, 0x253bd5,
        0x3428d2, 0x502ada, 0x6b2ce2, 0x872eea,
        0xa43aef, 0xc14af4, 0xde59f8, 0xfb69fd
};

const TProgmemRGBPalette16 TwilightSymphony_gp = {
        0x6a59e7, 0x5a4ccd, 0x4a3eb4, 0x3a319a,
        0x372587, 0x5c1e86, 0x811685, 0xa60f84,
        0xa81490, 0x8725ab, 0x6636c5, 0x4548df,
        0x473ec3, 0x542b94, 0x611865, 0x6e0536
};

const TProgmemRGBPalette16 MidnightCalm_gp = {
        0x011642, 0x0b2160, 0x142d7e, 0x1e389c,
        0x2444b2, 0x204eb0, 0x1d58ae, 0x1963ad,
        0x1573b3, 0x108ac1, 0x0ca1cf, 0x07b8dd,
        0x32cbe5, 0x6cdceb, 0xa6eef0, 0xe0fff6
};

const TProgmemRGBPalette16 NightNocturne_gp = {
        0x449af6, 0x3373d0, 0x224daa, 0x112684,
        0x040965, 0x04085c, 0x040753, 0x04064a,
        0x040543, 0x04053d, 0x040537, 0x040531,
        0x0f0a4f, 0x1d1279, 0x2c19a2, 0x3a20cc
};

const TProgmemRGBPalette16 CrimsonRhapsody_gp = {
        0xa60422, 0xba032b, 0xcf0334, 0xe3023d,
        0xef084c, 0xe2226c, 0xd53c8b, 0xc855ab,
        0xa857b1, 0x75429e, 0x422c8a, 0x0f1677,
        0x081882, 0x0f2198, 0x172bad, 0x1e34c3
};

const TProgmemRGBPalette16 AzureWaters_gp = {
        0x191970, 0x121274, 0x0c0c79, 0x05057d,
        0x000884, 0x002895, 0x0047a5, 0x0067b6,
        0x0975c3, 0x1a71cc, 0x2b6ed5, 0x3d6adf,
        0x3966c9, 0x2e62aa, 0x235d8a, 0x18596b
};

const TProgmemRGBPalette16 FieryBlend_gp = {
        0x7273fa, 0x8e5cbe, 0xaa4683, 0xc62f47,
        0xd81c19, 0xca1613, 0xbc100e, 0xaf0a09,
        0x970c23, 0x75155d, 0x531e97, 0x3127d1,
        0x2021ca, 0x1616ad, 0x0b0c90, 0x010173
};

const TProgmemRGBPalette16 PastelDreams_gp = {
        0xf9cefd, 0xf1cbfe, 0xe9c8fe, 0xe1c5ff,
        0xcfbbfc, 0x9d9df0, 0x6b7ee5, 0x3a5fd9,
        0x3761d9, 0x6484e5, 0x90a6f0, 0xbdc8fc,
        0xc3c8fb, 0xbcbcf5, 0xb5b0f0, 0xaea4ea
};

const TProgmemRGBPalette16 MossyWhisper_gp = {
        0x9aa88b, 0x879a7a, 0x748c69, 0x617e59,
        0x53734d, 0x547452, 0x557557, 0x56765c,
        0x4e6954, 0x3c4e40, 0x2a342d, 0x181a19,
        0x20231f, 0x30392d, 0x404e3b, 0x506449
};

const TProgmemRGBPalette16 VibrantTropics_gp = {
        0xecddbc, 0xb7b1a1, 0x818486, 0x4c586b,
        0x283d5d, 0x385375, 0x496a8c, 0x5981a4,
        0x6b8db3, 0x7e90ba, 0x9293c0, 0xa595c6,
        0xa1a5cc, 0x95b9d1, 0x88ccd7, 0x7ce0dc
};

const TProgmemRGBPalette16 OceanLook_gp = {
        0x546981, 0x4f7590, 0x4a819e, 0x458dad,
        0x4c9dbd, 0x79b8d0, 0xa6d3e3, 0xd3eef6,
        0xcde0e6, 0x96a9b5, 0x5f7283, 0x283b51,
        0x3e5064, 0x6e808e, 0x9eafb7, 0xcedee1
};

const TProgmemRGBPalette16 BrightSky_gp = {
        0x3a363e, 0x6b4d58, 0x9d6572, 0xce7c8c,
        0xf28c9b, 0xf08288, 0xee7975, 0xec7062,
        0xed7a6b, 0xf29990, 0xf6b7b5, 0xfbd5db,
        0xe4becf, 0xc595b2, 0xa56c96, 0x864379
};

const TProgmemRGBPalette16 UrbanDusk_gp = {
        0x7ccccb, 0x6babaa, 0x5a8b8a, 0x496a69,
        0x3b504f, 0x374646, 0x343d3e, 0x303435,
        0x2c2f31, 0x292e30, 0x252e2f, 0x222d2e,
        0x243031, 0x273434, 0x2b3838, 0x2e3c3b
};

const TProgmemRGBPalette16 AutumnFire_gp = {
        0xed7205, 0xf18606, 0xf49906, 0xf8ad07,
        0xf4b308, 0xd88f0d, 0xbb6b11, 0x9f4716,
        0x893519, 0x78351c, 0x67341f, 0x563422,
        0x693b20, 0x87431c, 0xa64c17, 0xc45513
};

const TProgmemRGBPalette16 QuietWoods_gp = {
        0x6b6461, 0x556165, 0x3f5f68, 0x295c6c,
        0x185569, 0x174351, 0x163139, 0x151e21,
        0x2b1d1d, 0x572d2c, 0x843d3c, 0xb04d4b,
        0x9d5251, 0x755353, 0x4c5456, 0x245558
};

const TProgmemRGBPalette16 GrayHeaven_gp = {
        0xbbbfc4, 0x98a7be, 0x748eb9, 0x5176b3,
        0x3461aa, 0x2b5497, 0x234784, 0x1a3a71,
        0x284473, 0x4c648a, 0x6f84a0, 0x93a4b6,
        0x7e8ea7, 0x55658b, 0x2d3d70, 0x041454
};

const TProgmemRGBPalette16 MarineMystery_gp = {
        0x2b4b74, 0x5f6c7a, 0x928d7f, 0xc6ad85,
        0xe0bc85, 0xae9675, 0x7c6f66, 0x494856,
        0x38415b, 0x485874, 0x586f8d, 0x6886a6,
        0x5b7492, 0x455470, 0x2f344e, 0x19142c
};

const TProgmemRGBPalette16 GoldenSpice_gp = {
        0x984539, 0xb16744, 0xca884f, 0xe3aa5b,
        0xf6c162, 0xf5b75c, 0xf5ae56, 0xf4a550,
        0xe3984f, 0xc18853, 0x9f7857, 0x7d685b,
        0x6f5f5f, 0x695963, 0x625268, 0x5c4c6c
};

const TProgmemRGBPalette16 VintageRouge_gp = {
        0x8d90be, 0xa788a5, 0xc1808c, 0xdb7873,
        0xe96f5d, 0xd56153, 0xc15449, 0xad473f,
        0x964c4f, 0x7b6578, 0x617da1, 0x4796cb,
        0x4195cd, 0x428cc1, 0x4382b6, 0x4479ab
};

const TProgmemRGBPalette16 GrayHarmony_gp = {
        0x4c733c, 0x457a3c, 0x3d823d, 0x36893d,
        0x399345, 0x5ba567, 0x7db789, 0x9fc8ab,
        0xa7c6b3, 0x96b0a1, 0x849a8f, 0x72847d,
        0x647570, 0x566964, 0x495d58, 0x3b514c
};

const TProgmemRGBPalette16 EmeraldOasis_gp = {
        0x072012, 0x36531c, 0x658627, 0x94ba31,
        0xadd737, 0x85b42d, 0x5d9124, 0x356e1b,
        0x226323, 0x24703c, 0x267e55, 0x288c6e,
        0x349a7f, 0x44a88e, 0x54b69d, 0x64c4ac
};

const TProgmemRGBPalette16 SunnyMosaic_gp = {
        0xfbd25f, 0xd6c77d, 0xb0bc9a, 0x8bb1b8,
        0x6ea4c8, 0x6a91ae, 0x657e95, 0x616b7c,
        0x555662, 0x414048, 0x2d2a2e, 0x191414,
        0x1d1a17, 0x292823, 0x36362f, 0x42443b
};

const TProgmemRGBPalette16 SoftPetals_gp = {
        0xfbd1fb, 0xd5a7e0, 0xae7dc5, 0x8853a9,
        0x713896, 0x894e9c, 0xa063a1, 0xb878a6,
        0xc6889a, 0xca937b, 0xcf9e5c, 0xd3a93e,
        0xb99e34, 0x958a31, 0x70772f, 0x4c642c
};

const TProgmemRGBPalette16 MintyBreeze_gp = {
        0x648a66, 0x868b6a, 0xa88d6f, 0xca8e73,
        0xde9077, 0xc8947a, 0xb2997d, 0x9c9d80,
        0x8fa283, 0x8aa788, 0x85ad8c, 0x80b391,
        0x72ae8c, 0x60a584, 0x4e9d7c, 0x3c9474
};

const TProgmemRGBPalette16 RedDance_gp = {
        0xeb353c, 0xef5d37, 0xf38631, 0xf7ae2c,
        0xeec72c, 0xbcb13b, 0x8b9b4b, 0x5a855a,
        0x457f61, 0x4c885e, 0x54925b, 0x5b9c59,
        0x54934b, 0x48843b, 0x3b752a, 0x2f6619
};

const TProgmemRGBPalette16 NaturalBreeze_gp = {
        0xecefe1, 0xcdd9b6, 0xaec38a, 0x8ead5f,
        0x7a9f3c, 0x88ac35, 0x96b92e, 0xa3c627,
        0x9bbf21, 0x7da61b, 0x5f8c15, 0x41720f,
        0x47751e, 0x578134, 0x688d4a, 0x799960
};

const TProgmemRGBPalette16 PeacefulOlive_gp = {
        0xa7bdc9, 0x95a7a1, 0x829179, 0x707b51,
        0x6c7035, 0x92883c, 0xb9a043, 0xe0b84a,
        0xdcba50, 0xada755, 0x7e945a, 0x50815f,
        0x56826b, 0x6d8b79, 0x859388, 0x9c9c96
};

const TProgmemRGBPalette16 BrownishRed_gp = {
        0x452316, 0x743d13, 0xa35710, 0xd3710c,
        0xf58913, 0xf39a39, 0xf0ab5f, 0xedbc85,
        0xdeb791, 0xc29e83, 0xa78475, 0x8b6a67,
        0x9c807f, 0xbca5a3, 0xdccac7, 0xfcefeb
};

const TProgmemRGBPalette16 RosyDawn_gp = {
        0xdf2f52, 0xb0314f, 0x81334c, 0x523548,
        0x323947, 0x3d4349, 0x484d4c, 0x53574f,
        0x6c645e, 0x927479, 0xb88594, 0xde95af,
        0xd992b2, 0xc689ac, 0xb480a6, 0xa177a0
};

const TProgmemRGBPalette16 VioletTwilight_gp = {
        0x866acc, 0x765bba, 0x664ba8, 0x563c96,
        0x483480, 0x42425e, 0x3b503c, 0x345e1a,
        0x2c6008, 0x215607, 0x164c06, 0x0b4204,
        0x0c3417, 0x102631, 0x15174b, 0x1a0965
};

const TProgmemRGBPalette16 RedSunset_gp = {
        0xdd600b, 0xc4530a, 0xaa4609, 0x913909,
        0x7c340e, 0x754826, 0x6d5b3f, 0x666f57,
        0x5f7661, 0x596f5d, 0x536858, 0x4d6154,
        0x475348, 0x41423a, 0x3a322b, 0x34211d
};

const TProgmemRGBPalette16 MysticMidnight_gp = {
        0x2a3533, 0x3b465d, 0x4c5687, 0x5c67b1,
        0x636dcc, 0x4953ba, 0x2f39a8, 0x162097,
        0x0f1683, 0x1a1d6d, 0x262457, 0x312a41,
        0x372f48, 0x3b3359, 0x40386b, 0x443c7c
};

const TProgmemRGBPalette16 RoyalElegance_gp = {
        0x5a83a8, 0x5376a3, 0x4c699d, 0x445d98,
        0x425992, 0x4c708a, 0x578783, 0x629e7b,
        0x73ae81, 0x8bb694, 0xa2bea7, 0xbac6ba,
        0xbeb9b2, 0xbaa5a0, 0xb7908e, 0xb47c7c
};

const TProgmemRGBPalette16 LimeSplash_gp = {
        0xc3e565, 0x96bc5a, 0x69944e, 0x3d6b43,
        0x255539, 0x4d7735, 0x759932, 0x9dbb2e,
        0xb6c327, 0xbfb21c, 0xc8a111, 0xd29007,
        0xab8c25, 0x748d51, 0x3e8d7e, 0x078eaa
};

const TProgmemRGBPalette16 ScarletEssence_gp = {
        0xa06868, 0xac7272, 0xb97d7b, 0xc58785,
        0xc88685, 0xb06467, 0x984249, 0x801f2c,
        0x782023, 0x7f4430, 0x87673d, 0x8e8b4a,
        0xa09d59, 0xb5a969, 0xcbb478, 0xe0c088
};

const TProgmemRGBPalette16 GreenWoods_gp = {
        0xc5e094, 0xb1d180, 0x9ec16d, 0x8ab259,
        0x79a448, 0x709b42, 0x67923c, 0x5e8935,
        0x578431, 0x52832e, 0x4d822c, 0x47812a,
        0x4a8436, 0x508848, 0x558c59, 0x5b906b
};

const TProgmemRGBPalette16 ForestRetreat_gp = {
        0x627653, 0x58744c, 0x4f7246, 0x45703f,
        0x3e6d39, 0x3c6836, 0x3b6332, 0x3a5e2f,
        0x45643d, 0x5d755d, 0x76867c, 0x8e989c,
        0x7b878e, 0x5b6c70, 0x3a5052, 0x193534
};

const TProgmemRGBPalette16 MistyMoor_gp = {
        0xb1bebf, 0x919b9e, 0x71777d, 0x51545d,
        0x3c3e42, 0x465339, 0x506830, 0x5a7d27,
        0x6a8a2c, 0x818e3f, 0x989152, 0xae9564,
        0xbaa378, 0xc3b38c, 0xcbc4a0, 0xd4d5b4
};

const TProgmemRGBPalette16 GreenOasis_gp = {
        0x578461, 0x608d5d, 0x69955a, 0x719e56,
        0x7fa85d, 0x99ba83, 0xb4cba9, 0xcfdccf,
        0xc0d1c5, 0x87a88c, 0x4e7f53, 0x15561a,
        0x0a4916, 0x0d4522, 0x11402f, 0x143c3c
};

const TProgmemRGBPalette16 CalmLake_gp = {
        0x048068, 0x2a947d, 0x50a992, 0x76bda7,
        0x89c2ae, 0x639c8b, 0x3d7667, 0x174f44,
        0x043a30, 0x04352c, 0x043129, 0x042c25,
        0x2b5045, 0x5f8171, 0x94b39c, 0xc8e4c8
};

const TProgmemRGBPalette16 WarmAmber_gp = {
        0xe5270e, 0xdc4e23, 0xd47638, 0xcb9d4d,
        0xc6bc5c, 0xccbe56, 0xd2c050, 0xd7c24b,
        0xdab247, 0xdb9046, 0xdb6e45, 0xdc4c44,
        0xcd544f, 0xb86a5f, 0xa47f6e, 0x8f957d
};

const TProgmemRGBPalette16 CampFire_gp = {
        0x003049, 0x392e40, 0x722c37, 0xab2a2f,
        0xd82e25, 0xe1451b, 0xea5c10, 0xf37305,
        0xf8880a, 0xf9991d, 0xfaaa31, 0xfcbb44,
        0xf8c65f, 0xf4cf7c, 0xefd99a, 0xeae2b7
};

const TProgmemRGBPalette16 EnchantedForest_gp = {
        0x073f04, 0x1e5f0b, 0x367e13, 0x4d9e1a,
        0x63b628, 0x72b649, 0x81b769, 0x90b78a,
        0x85b087, 0x5ea160, 0x38923a, 0x128314,
        0x1f8c26, 0x3e9d4b, 0x5daf71, 0x7cc096
};

const TProgmemRGBPalette16 AquaMarine_gp = {
        0x19c1fc, 0x14a3bd, 0x0e867e, 0x09683e,
        0x055113, 0x054c23, 0x044732, 0x044242,
        0x054549, 0x085147, 0x0a5e45, 0x0c6a43,
        0x156542, 0x1f5a43, 0x2a4f43, 0x344444
};

const TProgmemRGBPalette16 MidnightBlue_gp = {
        0x060809, 0x233142, 0x405a7b, 0x5c82b3,
        0x78a5df, 0x8fb3e5, 0xa7c1ea, 0xbecfef,
        0xb6c5e3, 0x8da3c4, 0x6581a5, 0x3c5f86,
        0x304e6c, 0x2e4253, 0x2c373b, 0x2a2c22
};

const TProgmemRGBPalette16 MonochromeChic_gp = {
        0x92919f, 0x6e6d79, 0x4a4854, 0x26242e,
        0x0e0d19, 0x1b1f35, 0x283152, 0x35436e,
        0x374470, 0x2c3559, 0x212641, 0x16172a,
        0x321b29, 0x5a262f, 0x833136, 0xac3c3c
};

const TProgmemRGBPalette16 SeaDusk_gp = {
        0x0e3055, 0x14395f, 0x1a4268, 0x204b72,
        0x2c597f, 0x4b7396, 0x6b8dae, 0x8aa7c5,
        0x9fb4ce, 0xa8b4c9, 0xb1b4c4, 0xbab4bf,
        0xa0a8b9, 0x7c98b2, 0x5788ab, 0x3278a4
};

const TProgmemRGBPalette16 EtherealSerenade_gp = {
        0x54aec0, 0x62a3a9, 0x709892, 0x7e8d7b,
        0x868066, 0x7b6d54, 0x705a43, 0x654732,
        0x56483b, 0x425d60, 0x2e7185, 0x1a86aa,
        0x3a85b0, 0x6a7dac, 0x9b74a8, 0xcc6ca4
};

const TProgmemRGBPalette16 DarkBlase_gp = {
        0x7f2606, 0x782808, 0x702a0a, 0x692c0c,
        0x6d370d, 0x95590e, 0xbd7b0f, 0xe59e10,
        0xf4a310, 0xe98c0d, 0xde750b, 0xd35e09,
        0xbf5c21, 0xa96142, 0x926763, 0x7c6c84
};

const TProgmemRGBPalette16 RusticTerrain_gp = {
        0x4a0604, 0x3c0504, 0x2d0504, 0x1f0404,
        0x180807, 0x291911, 0x3a2a1c, 0x4b3b27,
        0x615031, 0x7a683b, 0x938045, 0xad984f,
        0xa08a4c, 0x876e44, 0x6d533c, 0x543834
};

const TProgmemRGBPalette16 RaspberrySorbet_gp = {
        0xb04137, 0xbc4748, 0xc74d59, 0xd3536a,
        0xdb5f7d, 0xd77d96, 0xd39aaf, 0xcfb7c8,
        0xcfb4c4, 0xd390a4, 0xd66c83, 0xda4863,
        0xbb3a4e, 0x90323c, 0x652b2a, 0x3a2418
};

const TProgmemRGBPalette16 LuxuriousVelvet_gp = {
        0x706298, 0x7a56a1, 0x854aab, 0x8f3eb4,
        0x9332b8, 0x8426ad, 0x751ba2, 0x660f97,
        0x530c84, 0x3d1167, 0x27164b, 0x111b2e,
        0x303d4a, 0x616879, 0x9294a7, 0xc3bfd6
};

const TProgmemRGBPalette16 MysticalSpectrum_gp = {
        0x3ff3a4, 0x33b98c, 0x287f74, 0x1c455c,
        0x22274a, 0x5c5e4c, 0x97954e, 0xd2cc4f,
        0xd7cc58, 0xa89368, 0x795b78, 0x4a2288,
        0x621f9e, 0x922eb5, 0xc23dcd, 0xf24ce4
};

const TProgmemRGBPalette16 PurpleSky_gp = {
        0x2d3047, 0x372858, 0x42216a, 0x4c197b,
        0x5d1286, 0x800e7d, 0xa40975, 0xc7056c,
        0xdc195d, 0xe34546, 0xea702f, 0xf19c18,
        0xd4a630, 0xaaa557, 0x80a47f, 0x56a3a6
};

const TProgmemRGBPalette16 FieryFusion_gp = {
        0x0a0310, 0x1b022d, 0x2c014b, 0x3c0168,
        0x55007c, 0x860072, 0xb60069, 0xe70060,
        0xff1151, 0xff323d, 0xff5329, 0xff7515,
        0xff8818, 0xff9623, 0xffa42d, 0xffb238
};

const TProgmemRGBPalette16 GoldenHorizon_gp = {
        0xe2a668, 0xb5af85, 0x89b7a3, 0x5cc0c0,
        0x37bcce, 0x2992ad, 0x1a698d, 0x0b406c,
        0x042651, 0x041b3a, 0x041123, 0x04070d,
        0x0e0e07, 0x1a1a07, 0x272708, 0x343408
};

const TProgmemRGBPalette16 OceanAdventure_gp = {
        0x264653, 0x275d63, 0x287473, 0x298c83,
        0x37a08d, 0x6aaa83, 0x9db479, 0xd0bf6f,
        0xeabf69, 0xedb666, 0xf0ad64, 0xf3a462,
        0xf1985e, 0xee8a5a, 0xea7d55, 0xe76f51
};

const TProgmemRGBPalette16 CalmOasis_gp = {
        0xe63946, 0xe96c73, 0xeca0a0, 0xefd3cc,
        0xecf8ed, 0xd9efe8, 0xc5e7e3, 0xb2dede,
        0x9bcdd4, 0x80b4c3, 0x669bb2, 0x4c81a1,
        0x3d6d8f, 0x325a7c, 0x28486a, 0x1d3557
};

const TProgmemRGBPalette16 EtherealEvolution_gp = {
        0x03071e, 0x22061a, 0x410615, 0x600411,
        0x7e030c, 0x9d0208, 0xbc0103, 0xd20900,
        0xda2602, 0xe14103, 0xe85d04, 0xef7905,
        0xf59106, 0xf99e07, 0xfcac07, 0xffba08
};

const TProgmemRGBPalette16 NeonSpectrum_gp = {
        0xf72585, 0xcf1d94, 0xa814a3, 0x7f0cb2,
        0x670ab3, 0x560bad, 0x4e0caa, 0x450ca7,
        0x3d0ca4, 0x3c1db2, 0x3f37c9, 0x4150df,
        0x446bee, 0x478bef, 0x4aaaef, 0x4cc9f0
};

const TProgmemRGBPalette16 BeachElegance_gp = {
        0x001219, 0x00404f, 0x02697a, 0x08898f,
        0x41aca6, 0x94d2bd, 0xc7d6af, 0xeacc85,
        0xeda721, 0xe08601, 0xca6702, 0xc14e03,
        0xb83806, 0xb1260f, 0xa6211a, 0x9b2226
};

const TProgmemRGBPalette16 DarkMystery_gp = {
        0x0b090a, 0x121315, 0x261619, 0x560b0c,
        0x7f0d0f, 0xa4161a, 0xb1171b, 0xc31e21,
        0xdc3235, 0xd06466, 0xb1a7a6, 0xc5c1c1,
        0xdad9da, 0xeeeded, 0xf9f8f8, 0xffffff
};

const TProgmemRGBPalette16 RedTwilight_gp = {
        0xb7094c, 0xab1252, 0x9e1b59, 0x92245f,
        0x862d66, 0x7a366c, 0x6e3f73, 0x62487a,
        0x565280, 0x4a5b87, 0x3d648d, 0x316d93,
        0x25769a, 0x197fa0, 0x0c88a7, 0x0091ad
};

const TProgmemRGBPalette16 ColorfulRainbow_gp = {
        0xff0000, 0xd52a00, 0xab5500, 0xab7f00,
        0xabab00, 0x56d500, 0x00ff00, 0x00d52a,
        0x00ab55, 0x0056aa, 0x0000ff, 0x2a00d5,
        0x5500ab, 0x7f0081, 0xab0055, 0xd5002b
};

const TProgmemRGBPalette16 ShadowBlaze_gp = {
        0xff0000, 0x000000, 0xab5500, 0x000000,
        0xabab00, 0x000000, 0x00ff00, 0x000000,
        0x00ab55, 0x000000, 0x0000ff, 0x000000,
        0x5500ab, 0x000000, 0xab0055, 0x000000
};

const TProgmemRGBPalette16 RedInferno_gp = {
        0x000000, 0x330000, 0x660000, 0x990000,
        0xcc0000, 0xff0000, 0xff3300, 0xff6600,
        0xff9900, 0xffcc00, 0xffff00, 0xffff33,
        0xffff66, 0xffff99, 0xffffcc, 0xffffff
};
