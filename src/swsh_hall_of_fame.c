#include "global.h"
#include "swsh_hall_of_fame.h"
#include "hall_of_fame.h"
#include "bg.h"
#include "battle.h"
#include "comfy_anim.h"
#include "credits.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokemon.h"
#include "random.h"
#include "save.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "trig.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Save
#define HALL_OF_FAME_MAX_TEAMS      30
#define SAVE_MESSAGE_FRAMES         32

STATIC_ASSERT(sizeof(struct HallofFameTeam) * HALL_OF_FAME_MAX_TEAMS <= SECTOR_DATA_SIZE * NUM_HOF_SECTORS, HallOfFameSwShFreeSpace);

// Palettes
#define PAL_HOF_MAIN                0
#define PAL_HOF_TYPE_BG             1
#define INTRO_BG_PALETTES           ((1 << PAL_HOF_MAIN) | (1 << PAL_HOF_TYPE_BG))
#define PAL_SLOT_BACK_PIC           0
#define PAL_SLOT_TRAINER_PIC        6
#define PAL_BANK_COLORS             (PLTT_SIZE_4BPP / sizeof(u16))
#define NUM_RESERVED_OBJ_PALETTES   8
#define FADE_COEFF_MAX              16

// BGs
#define TILEMAP_PALETTE_SHIFT       12
#define TILEMAP_TILE_MASK           0x0FFF
#define TYPE_BG_SCROLL_SPEED        64
#define INTRO_BG2_BLEND_EVA         10
#define INTRO_BG2_BLEND_EVB         6

// Intro timing
#define BACK_SLIDE_FRAMES           60
#define FRONT_SLIDE_FRAMES          48
#define INTRO_FADE_IN_FRAMES        48
#define BANNER_FLASH_FRAMES         8
#define INTRO_POST_ANIM_FRAMES      120
#define INTRO_FADE_OUT_FRAMES       16

// Assemble timing
#define ASSEMBLE_SLIDE_FRAMES       30
#define ASSEMBLE_SLIDE_STAGGER      12
#define ASSEMBLE_TO_WHITE_FRAMES    16
#define ASSEMBLE_FROM_WHITE_FRAMES  16
#define CONFETTI_DELAY_FRAMES       60

// Exit
#define EXIT_FADE_FRAMES            16
#define EXIT_BGM_FADE_SPEED         4

// Mon sprites
#define MON_OBJ_PRIORITY            2
#define FRONT_LANDING_X             172
#define FRONT_LANDING_Y             78
#define FRONT_SUBPRIORITY           0
#define SHADOW_SUBPRIORITY          (FRONT_SUBPRIORITY + 1)
#define SHADOW_OFFSET_X             5
#define SHADOW_OFFSET_Y             2
#define TAG_MON_SHADOW              55140
#define MON_SHADOW_COLOR            RGB(15, 15, 15)

// Assemble sprites
#define ASSEMBLE_TRAINER_X          120
#define ASSEMBLE_TRAINER_Y          92
#define ASSEMBLE_TRAINER_SUBPRIORITY 0
#define TAG_MON_SILHOUETTE          55141

// Confetti
#define TAG_CONFETTI                1001
#define CONFETTI_SHEET_SIZE         0x220
#define CONFETTI_FRAMES             400
#define CONFETTI_SPAWN_INTERVAL     4
#define CONFETTI_FALL_FRAMES        110
#define CONFETTI_OFFSCREEN_Y        120
#define CONFETTI_SWAY_BASE          8
#define CONFETTI_SWAY_RANGE         4
#define CONFETTI_SINE_STEP          4
#define CONFETTI_EXTRA_Y_ODDS       4
#define CONFETTI_MAX_SPAWN_Y        8
#define CONFETTI_SINE_SCALE         256

// Front pic sprite data
#define sSpecies                    data[0]
#define sDontFlip                   data[1]

// Shadow sprite data
#define sOwnTileNum                 data[0]
#define sFrontSpriteId              data[1]

// Confetti sprite data
#define sSineIdx                    data[0]
#define sExtraY                     data[1]

// Windows
#define SAVE_MESSAGE_LEFT           2
#define SAVE_MESSAGE_TOP            15
#define SAVE_MESSAGE_WIDTH          27
#define SAVE_MESSAGE_HEIGHT         4
#define SAVE_MESSAGE_BASE_BLOCK     0x194

#define INTRO_TITLE_LEFT            3
#define INTRO_TITLE_TOP             2
#define INTRO_TITLE_WIDTH           19
#define INTRO_TITLE_HEIGHT          2
#define INTRO_TITLE_BASE_BLOCK      1

#define INTRO_NICKNAME_LEFT         10
#define INTRO_NICKNAME_TOP          15
#define INTRO_NICKNAME_WIDTH        10
#define INTRO_NICKNAME_HEIGHT       3
#define INTRO_NICKNAME_BASE_BLOCK   (INTRO_TITLE_BASE_BLOCK + INTRO_TITLE_WIDTH * INTRO_TITLE_HEIGHT)

#define INTRO_LEVEL_LEFT            24
#define INTRO_LEVEL_TOP             15
#define INTRO_LEVEL_WIDTH           5
#define INTRO_LEVEL_HEIGHT          3
#define INTRO_LEVEL_BASE_BLOCK      (INTRO_NICKNAME_BASE_BLOCK + INTRO_NICKNAME_WIDTH * INTRO_NICKNAME_HEIGHT)

#define INTRO_TEXT_TITLE_Y          1
#define INTRO_TEXT_INFO_Y           5

#define CHAMPION_TITLE_LEFT         3
#define CHAMPION_TITLE_TOP          1
#define CHAMPION_TITLE_WIDTH        23
#define CHAMPION_TITLE_HEIGHT       2
#define CHAMPION_TITLE_BASE_BLOCK   (INTRO_LEVEL_BASE_BLOCK + INTRO_LEVEL_WIDTH * INTRO_LEVEL_HEIGHT)

#define CHAMPION_INFO_LEFT          4
#define CHAMPION_INFO_TOP           17
#define CHAMPION_INFO_WIDTH         24
#define CHAMPION_INFO_HEIGHT        2
#define CHAMPION_INFO_BASE_BLOCK    (CHAMPION_TITLE_BASE_BLOCK + CHAMPION_TITLE_WIDTH * CHAMPION_TITLE_HEIGHT)

#define CHAMPION_TEXT_Y             1
#define CHAMPION_ID_X               68

STATIC_ASSERT(CHAMPION_INFO_BASE_BLOCK + CHAMPION_INFO_WIDTH * CHAMPION_INFO_HEIGHT <= SAVE_MESSAGE_BASE_BLOCK, HallOfFameSwShWindowBaseBlocks);

enum
{
    WIN_SAVE_MESSAGE,
    WIN_INTRO_TITLE,
    WIN_INTRO_NICKNAME,
    WIN_INTRO_LEVEL,
    WIN_CHAMPION_TITLE,
    WIN_CHAMPION_INFO,
};

enum
{
    HOF_COLOR_TRANSPARENT,
    HOF_COLOR_WHITE,
    HOF_COLOR_LIGHT_GRAY,
    HOF_COLOR_DARK_GRAY,
    HOF_COLOR_BLACK,
};

enum
{
    INTRO_FADE_PENDING,
    INTRO_FADE_RUNNING,
    INTRO_FADE_DONE,
};

struct HallOfFameSwSh
{
    struct HallofFameTeam team;
    enum Species displaySpecies[PARTY_SIZE];
    u16 bg0TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg1TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg2TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg3TilemapBuffer[BG_SCREEN_SIZE / 2];
    struct SpriteTemplate shadowTemplate;
    u32 fadePalettes;
    u32 fadeColor;
    u32 fadeAnimId;
    u32 slideAnimId;
    u32 assembleAnimIds[PARTY_SIZE];
    s16 slideX;
    s16 slideTo;
    u8 fadeCoeff;
    bool8 dontSaveData;
    u8 introMonIds[PARTY_SIZE];
    u8 numIntroMons;
    u8 numTeamMons;
    u8 monId;
    u8 introFade;
    u8 backSpriteId;
    u8 frontSpriteId;
    u8 shadowSpriteId;
    u8 assembleSpriteIds[PARTY_SIZE];
    u8 trainerSpriteId;
    u8 silhouettePalette;
    u8 flashTimer;
    bool8 createdGfxManager;
};

static EWRAM_DATA struct HallOfFameSwSh *sHof = NULL;

static void Task_HofSwSh_InitTeam(u8 taskId);
static void Task_HofSwSh_InsertTeam(u8 taskId);
static void Task_HofSwSh_TrySave(u8 taskId);
static void Task_HofSwSh_WaitSaveMessage(u8 taskId);
static void Task_HofSwSh_StartExit(u8 taskId);
static void Task_HofSwSh_IntroSetup(u8 taskId);
static void Task_HofSwSh_IntroBackSlide(u8 taskId);
static void Task_HofSwSh_IntroFrontSlide(u8 taskId);
static void Task_HofSwSh_IntroWaitAnim(u8 taskId);
static void Task_HofSwSh_IntroHold(u8 taskId);
static void Task_HofSwSh_IntroFadeOut(u8 taskId);
static void Task_HofSwSh_AssembleSetup(u8 taskId);
static void Task_HofSwSh_AssembleCreateSprites(u8 taskId);
static void Task_HofSwSh_AssembleSlide(u8 taskId);
static void Task_HofSwSh_AssembleToWhite(u8 taskId);
static void Task_HofSwSh_AssembleFromWhite(u8 taskId);
static void Task_HofSwSh_AssembleHold(u8 taskId);
static void Task_HofSwSh_Confetti(u8 taskId);
static void Task_HofSwSh_PostConfettiHold(u8 taskId);
static void SpriteCB_Confetti(struct Sprite *sprite);
static void Task_HofSwSh_Exit(u8 taskId);

static const u8 sTextColors[] = {HOF_COLOR_TRANSPARENT, HOF_COLOR_WHITE, HOF_COLOR_DARK_GRAY};

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 1,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_SAVE_MESSAGE] =
    {
        .bg = 0,
        .tilemapLeft = SAVE_MESSAGE_LEFT,
        .tilemapTop = SAVE_MESSAGE_TOP,
        .width = SAVE_MESSAGE_WIDTH,
        .height = SAVE_MESSAGE_HEIGHT,
        .paletteNum = DLG_WINDOW_PALETTE_NUM,
        .baseBlock = SAVE_MESSAGE_BASE_BLOCK
    },
    [WIN_INTRO_TITLE] =
    {
        .bg = 0,
        .tilemapLeft = INTRO_TITLE_LEFT,
        .tilemapTop = INTRO_TITLE_TOP,
        .width = INTRO_TITLE_WIDTH,
        .height = INTRO_TITLE_HEIGHT,
        .paletteNum = PAL_HOF_MAIN,
        .baseBlock = INTRO_TITLE_BASE_BLOCK
    },
    [WIN_INTRO_NICKNAME] =
    {
        .bg = 0,
        .tilemapLeft = INTRO_NICKNAME_LEFT,
        .tilemapTop = INTRO_NICKNAME_TOP,
        .width = INTRO_NICKNAME_WIDTH,
        .height = INTRO_NICKNAME_HEIGHT,
        .paletteNum = PAL_HOF_MAIN,
        .baseBlock = INTRO_NICKNAME_BASE_BLOCK
    },
    [WIN_INTRO_LEVEL] =
    {
        .bg = 0,
        .tilemapLeft = INTRO_LEVEL_LEFT,
        .tilemapTop = INTRO_LEVEL_TOP,
        .width = INTRO_LEVEL_WIDTH,
        .height = INTRO_LEVEL_HEIGHT,
        .paletteNum = PAL_HOF_MAIN,
        .baseBlock = INTRO_LEVEL_BASE_BLOCK
    },
    [WIN_CHAMPION_TITLE] =
    {
        .bg = 0,
        .tilemapLeft = CHAMPION_TITLE_LEFT,
        .tilemapTop = CHAMPION_TITLE_TOP,
        .width = CHAMPION_TITLE_WIDTH,
        .height = CHAMPION_TITLE_HEIGHT,
        .paletteNum = PAL_HOF_MAIN,
        .baseBlock = CHAMPION_TITLE_BASE_BLOCK
    },
    [WIN_CHAMPION_INFO] =
    {
        .bg = 0,
        .tilemapLeft = CHAMPION_INFO_LEFT,
        .tilemapTop = CHAMPION_INFO_TOP,
        .width = CHAMPION_INFO_WIDTH,
        .height = CHAMPION_INFO_HEIGHT,
        .paletteNum = PAL_HOF_MAIN,
        .baseBlock = CHAMPION_INFO_BASE_BLOCK
    },
    DUMMY_WIN_TEMPLATE
};

static const struct
{
    s16 x;
    s16 y;
    bool8 hFlip;
} sAssembleMonPositions[PARTY_SIZE] =
{
    {  93, 88, FALSE },
    { 147, 88, TRUE  },
    {  64, 80, FALSE },
    { 176, 80, TRUE  },
    {  39, 64, FALSE },
    { 201, 64, TRUE  },
};

#include "data/swsh_hall_of_fame.h"

static void LoadTypeWallpaper(enum Type type)
{
    u32 i;

    DecompressAndCopyTileDataToVram(3, sTypeWallpapers[type].tiles, 0, 0, 0);
    DecompressDataWithHeaderWram(sTypeWallpapers[type].tilemap, sHof->bg3TilemapBuffer);

    for (i = 0; i < BG_SCREEN_SIZE / 2; i++)
    {
        sHof->bg3TilemapBuffer[i] = (sHof->bg3TilemapBuffer[i] & TILEMAP_TILE_MASK) | (PAL_HOF_TYPE_BG << TILEMAP_PALETTE_SHIFT);
    }
    CopyBgTilemapBufferToVram(3);
    LoadPalette(sTypeWallpapers[type].palette, BG_PLTT_ID(PAL_HOF_TYPE_BG), PLTT_SIZE_4BPP);
}

static void StartFade(u32 palettes, u32 toCoeff, u32 frames, u32 color, ComfyAnimEasingFunc easing)
{
    struct ComfyAnimEasingConfig config =
    {
        .durationFrames = frames,
        .from = Q_24_8(sHof->fadeCoeff),
        .to = Q_24_8(toCoeff),
        .easingFunc = easing,
    };

    sHof->fadePalettes = palettes;
    sHof->fadeColor = color;
    if (sHof->fadeAnimId == INVALID_COMFY_ANIM)
    {
        sHof->fadeAnimId = CreateComfyAnim_Easing(&config);
    }
    else
    {
        InitComfyAnim_Easing(&config, &gComfyAnims[sHof->fadeAnimId]);
    }

    if (sHof->fadeAnimId == INVALID_COMFY_ANIM)
    {
        sHof->fadeCoeff = toCoeff;
    }
    BlendPalettes(palettes, sHof->fadeCoeff, color);
}

static bool32 UpdateFade(void)
{
    if (sHof->fadeAnimId == INVALID_COMFY_ANIM)
    {
        return FALSE;
    }

    sHof->fadeCoeff = ReadComfyAnimValueSmooth(&gComfyAnims[sHof->fadeAnimId]);
    BlendPalettes(sHof->fadePalettes, sHof->fadeCoeff, sHof->fadeColor);
    return !gComfyAnims[sHof->fadeAnimId].completed;
}

static void UpdateIntroFadeIn(void)
{
    if (sHof->introFade == INTRO_FADE_RUNNING && !UpdateFade())
    {
        sHof->introFade = INTRO_FADE_DONE;
    }
}

static void StartSlide(s32 from, s32 to, u32 frames, ComfyAnimEasingFunc easing)
{
    struct ComfyAnimEasingConfig config =
    {
        .durationFrames = frames,
        .from = Q_24_8(from),
        .to = Q_24_8(to),
        .easingFunc = easing,
    };

    sHof->slideTo = to;
    if (sHof->slideAnimId == INVALID_COMFY_ANIM)
    {
        sHof->slideAnimId = CreateComfyAnim_Easing(&config);
    }
    else
    {
        InitComfyAnim_Easing(&config, &gComfyAnims[sHof->slideAnimId]);
    }
    sHof->slideX = from;
}

static bool32 UpdateSlide(void)
{
    if (sHof->slideAnimId == INVALID_COMFY_ANIM)
    {
        sHof->slideX = sHof->slideTo;
        return FALSE;
    }

    sHof->slideX = ReadComfyAnimValueSmooth(&gComfyAnims[sHof->slideAnimId]);
    return !gComfyAnims[sHof->slideAnimId].completed;
}

static void StartBannerFlash(void)
{
    FillPalette(RGB_WHITE, BG_PLTT_ID(PAL_HOF_MAIN) + HOF_COLOR_DARK_GRAY, PLTT_SIZEOF(2));
    sHof->flashTimer = BANNER_FLASH_FRAMES;
}

static void UpdateBannerFlash(void)
{
    if (sHof->flashTimer && --sHof->flashTimer == 0)
    {
        LoadPalette(&sHallOfFameTiles_Pal[HOF_COLOR_DARK_GRAY], BG_PLTT_ID(PAL_HOF_MAIN) + HOF_COLOR_DARK_GRAY, PLTT_SIZEOF(2));
    }
}

static void VBlankCB_HallOfFameSwSh(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_HallOfFameSwSh(void)
{
    RunTasks();
    ChangeBgX(3, TYPE_BG_SCROLL_SPEED, BG_COORD_ADD);
    ChangeBgY(3, TYPE_BG_SCROLL_SPEED, BG_COORD_ADD);
    AdvanceComfyAnimations();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
}

static bool32 InitHallOfFameScreen(void)
{
    u32 i;

    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, (void *)OAM, OAM_SIZE);
        DmaClear16(3, (void *)PLTT, PLTT_SIZE);

        FillPalette(RGB_BLACK, 0, PLTT_SIZE);
        ResetPaletteFade();
        sHof = AllocZeroed(sizeof(*sHof));
        sHof->fadeAnimId = INVALID_COMFY_ANIM;
        sHof->slideAnimId = INVALID_COMFY_ANIM;
        for (i = 0; i < PARTY_SIZE; i++)
        {
            sHof->assembleAnimIds[i] = INVALID_COMFY_ANIM;
            sHof->assembleSpriteIds[i] = SPRITE_NONE;
        }
        sHof->trainerSpriteId = SPRITE_NONE;
        sHof->backSpriteId = SPRITE_NONE;
        sHof->frontSpriteId = SPRITE_NONE;
        sHof->shadowSpriteId = SPRITE_NONE;
        sHof->fadeCoeff = FADE_COEFF_MAX;
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        ResetTempTileDataBuffers();
        ResetAllPicSprites();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = NUM_RESERVED_OBJ_PALETTES;
        if (gMonSpritesGfxPtr == NULL)
        {
            CreateMonSpritesGfxManager(MON_SPR_GFX_MANAGER_A, MON_SPR_GFX_MODE_NORMAL);
            sHof->createdGfxManager = TRUE;
        }
        gMain.state++;
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetBgTilemapBuffer(0, sHof->bg0TilemapBuffer);
        SetBgTilemapBuffer(1, sHof->bg1TilemapBuffer);
        SetBgTilemapBuffer(2, sHof->bg2TilemapBuffer);
        SetBgTilemapBuffer(3, sHof->bg3TilemapBuffer);
        ResetAllBgsCoordinates();
        InitWindows(sWindowTemplates);
        InitTextBoxGfxAndPrinters();
        DecompressAndCopyTileDataToVram(2, sHallOfFameTiles_Gfx, 0, 0, 0);
        gMain.state++;
        break;
    case 3:
        if (FreeTempTileDataBuffersIfPossible())
        {
            break;
        }
        LoadPalette(sHallOfFameTiles_Pal, BG_PLTT_ID(PAL_HOF_MAIN), PLTT_SIZE_4BPP);
        SetBackdropFromColor(RGB_BLACK);
        BlendPalettes(1 << PAL_HOF_MAIN, FADE_COEFF_MAX, RGB_BLACK);
        DecompressDataWithHeaderWram(sIntroBg1_Tilemap, sHof->bg1TilemapBuffer);
        CopyBgTilemapBufferToVram(1);
        DecompressDataWithHeaderWram(sIntroBg2_Tilemap, sHof->bg2TilemapBuffer);
        CopyBgTilemapBufferToVram(2);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(INTRO_BG2_BLEND_EVA, INTRO_BG2_BLEND_EVB));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        ShowBg(2);
        ShowBg(3);
        gMain.state++;
        break;
    case 4:
        SetVBlankCallback(VBlankCB_HallOfFameSwSh);
        SetMainCallback2(CB2_HallOfFameSwSh);
        PlayBGM(MUS_HALL_OF_FAME);
        return FALSE;
    }
    return TRUE;
}

static void DestroyBackSprite(void)
{
    if (sHof->backSpriteId != SPRITE_NONE)
    {
        FreeAndDestroyMonPicSprite(sHof->backSpriteId);
        sHof->backSpriteId = SPRITE_NONE;
    }
}

static void SpriteCB_MonShadow(struct Sprite *sprite)
{
    struct Sprite *front = &gSprites[sprite->sFrontSpriteId];

    sprite->oam.tileNum = front->oam.tileNum;
    sprite->oam.affineMode = front->oam.affineMode;
    sprite->oam.matrixNum = front->oam.matrixNum;
    sprite->x2 = front->x2;
    sprite->y2 = front->y2;
    sprite->centerToCornerVecX = front->centerToCornerVecX;
    sprite->centerToCornerVecY = front->centerToCornerVecY;
    sprite->hFlip = front->hFlip;
    sprite->x = front->x + SHADOW_OFFSET_X;
    sprite->y = front->y + SHADOW_OFFSET_Y;
}

static void CreateShadowSprite(void)
{
    struct SpriteTemplate *template = &sHof->shadowTemplate;
    struct Sprite *front = &gSprites[sHof->frontSpriteId];
    struct Sprite *sprite;
    u8 palette;

    *template = gMultiuseSpriteTemplate;
    template->anims = gDummySpriteAnimTable;
    template->affineAnims = gDummySpriteAffineAnimTable;
    template->paletteTag = TAG_NONE;
    template->callback = SpriteCB_MonShadow;
    sHof->shadowSpriteId = CreateSprite(template, front->x + SHADOW_OFFSET_X, front->y + SHADOW_OFFSET_Y, SHADOW_SUBPRIORITY);

    sprite = &gSprites[sHof->shadowSpriteId];
    FreeSpriteOamMatrix(sprite);

    sprite->affineAnimBeginning = FALSE;
    sprite->affineAnimPaused = TRUE;
    palette = AllocSpritePalette(TAG_MON_SHADOW);
    FillPalette(MON_SHADOW_COLOR, OBJ_PLTT_ID(palette), PLTT_SIZE_4BPP);
    sprite->oam.paletteNum = palette;
    sprite->oam.priority = MON_OBJ_PRIORITY;
    sprite->sOwnTileNum = sprite->oam.tileNum;
    sprite->sFrontSpriteId = sHof->frontSpriteId;
}

static void DestroyShadowSprite(void)
{
    if (sHof->shadowSpriteId != SPRITE_NONE)
    {
        struct Sprite *sprite = &gSprites[sHof->shadowSpriteId];

        sprite->oam.tileNum = sprite->sOwnTileNum;
        sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
        sprite->oam.matrixNum = 0;
        DestroySprite(sprite);
        FreeSpritePaletteByTag(TAG_MON_SHADOW);
        sHof->shadowSpriteId = SPRITE_NONE;
    }
}

static void DestroyFrontSprite(void)
{
    if (sHof->frontSpriteId != SPRITE_NONE)
    {
        StopPokemonAnimationDelayTask();
        DestroySpriteAndFreeResources(&gSprites[sHof->frontSpriteId]);
        sHof->frontSpriteId = SPRITE_NONE;
    }
}

static void DestroyAssembleSprites(void)
{
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (sHof->assembleSpriteIds[i] != SPRITE_NONE)
        {
            FreeAndDestroyMonPicSprite(sHof->assembleSpriteIds[i]);
            sHof->assembleSpriteIds[i] = SPRITE_NONE;
        }
    }
    if (sHof->trainerSpriteId != SPRITE_NONE)
    {
        FreeAndDestroyTrainerPicSprite(sHof->trainerSpriteId);
        sHof->trainerSpriteId = SPRITE_NONE;
    }
    FreeSpritePaletteByTag(TAG_MON_SILHOUETTE);
}

static void FreeHallOfFame(void)
{
    DestroyAssembleSprites();
    FreeSpriteTilesByTag(TAG_CONFETTI);
    FreeSpritePaletteByTag(TAG_CONFETTI);
    DestroyBackSprite();
    DestroyShadowSprite();
    DestroyFrontSprite();
    if (sHof->createdGfxManager)
    {
        DestroyMonSpritesGfxManager(MON_SPR_GFX_MANAGER_A);
    }
    HideBg(0);
    HideBg(1);
    HideBg(2);
    HideBg(3);
    FreeAllWindowBuffers();
    UnsetBgTilemapBuffer(0);
    UnsetBgTilemapBuffer(1);
    UnsetBgTilemapBuffer(2);
    UnsetBgTilemapBuffer(3);
    ResetBgsAndClearDma3BusyFlags(0);
    ReleaseComfyAnim(sHof->fadeAnimId);
    ReleaseComfyAnim(sHof->slideAnimId);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    TRY_FREE_AND_SET_NULL(gHoFSaveBuffer);
    TRY_FREE_AND_SET_NULL(sHof);
}

void CB2_DoHallOfFameScreen_SwSh(void)
{
    if (!InitHallOfFameScreen())
    {
        sHof->dontSaveData = FALSE;
        CreateTask(Task_HofSwSh_InitTeam, 0);
    }
}

void CB2_DoHallOfFameScreenDontSaveData_SwSh(void)
{
    if (!InitHallOfFameScreen())
    {
        sHof->dontSaveData = TRUE;
        CreateTask(Task_HofSwSh_InitTeam, 0);
    }
}

#define tFrameCount data[0]
#define tSpriteIndex data[1]

static const enum FormChanges sMegaFormChanges[] =
{
    FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM,
    FORM_CHANGE_BATTLE_MEGA_EVOLUTION_MOVE,
    FORM_CHANGE_BATTLE_PRIMAL_REVERSION,
};

static enum Species GetMegaSpecies(struct FormChangeContext ctx)
{
    u32 i;

    if (!SWSH_HOF_SHOW_MEGA)
    {
        return ctx.currentSpecies;
    }

    for (i = 0; i < ARRAY_COUNT(sMegaFormChanges); i++)
    {
        ctx.method = sMegaFormChanges[i];
        enum Species species = GetFormChangeTargetSpecies_Internal(ctx);
        if (species != ctx.currentSpecies)
        {
            return species;
        }
    }
    return ctx.currentSpecies;
}

static enum Species GetGmaxSpecies(struct FormChangeContext ctx)
{
    if (!SWSH_HOF_SHOW_GMAX)
    {
        return ctx.currentSpecies;
    }

    ctx.method = FORM_CHANGE_BATTLE_GIGANTAMAX;
    return GetFormChangeTargetSpecies_Internal(ctx);
}

static enum Species GetDisplaySpecies(struct Pokemon *mon)
{
    u32 i;
    struct FormChangeContext ctx =
    {
        .currentSpecies = GetMonData(mon, MON_DATA_SPECIES_OR_EGG),
        .heldItem = GetMonData(mon, MON_DATA_HELD_ITEM),
        .gmaxFactor = GetMonData(mon, MON_DATA_GIGANTAMAX_FACTOR),
    };

    for (i = 0; i < MAX_MON_MOVES; i++)
        ctx.moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);

    enum Species mega = GetMegaSpecies(ctx);
    enum Species gmax = GetGmaxSpecies(ctx);

    if (SWSH_HOF_FORM_PRIORITY == SWSH_HOF_FORM_GMAX && gmax != ctx.currentSpecies)
    {
        return gmax;
    }
    if (mega != ctx.currentSpecies)
    {
        return mega;
    }
    return gmax;
}

static void Task_HofSwSh_InitTeam(u8 taskId)
{
    u32 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u8 nickname[POKEMON_NAME_LENGTH + 1];

        if (GetMonData(mon, MON_DATA_SPECIES))
        {
            sHof->team.mon[i].species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
            sHof->team.mon[i].tid = GetMonData(mon, MON_DATA_OT_ID);
            sHof->team.mon[i].isShiny = GetMonData(mon, MON_DATA_IS_SHINY);
            sHof->team.mon[i].personality = GetMonData(mon, MON_DATA_PERSONALITY);
            sHof->team.mon[i].lvl = GetMonData(mon, MON_DATA_LEVEL);
            GetMonData(mon, MON_DATA_NICKNAME, nickname);
            for (j = 0; j < POKEMON_NAME_LENGTH; j++)
                sHof->team.mon[i].nickname[j] = nickname[j];
            sHof->displaySpecies[i] = GetDisplaySpecies(mon);
            sHof->numTeamMons++;
            if (sHof->team.mon[i].species != SPECIES_EGG)
            {
                sHof->introMonIds[sHof->numIntroMons++] = i;
            }
        }
        else
        {
            sHof->team.mon[i].species = SPECIES_NONE;
            sHof->displaySpecies[i] = SPECIES_NONE;
            sHof->team.mon[i].tid = 0;
            sHof->team.mon[i].isShiny = FALSE;
            sHof->team.mon[i].personality = 0;
            sHof->team.mon[i].lvl = 0;
            sHof->team.mon[i].nickname[0] = EOS;
        }
    }

    if (sHof->dontSaveData)
    {
        gTasks[taskId].func = Task_HofSwSh_IntroSetup;
    }
    else
    {
        gTasks[taskId].func = Task_HofSwSh_InsertTeam;
    }
}

static void Task_HofSwSh_InsertTeam(u8 taskId)
{
    u32 i;
    struct HallofFameTeam *lastSavedTeam;

    gHoFSaveBuffer = Alloc(SECTOR_SIZE * NUM_HOF_SECTORS);
    lastSavedTeam = gHoFSaveBuffer;

    if (!gHasHallOfFameRecords)
    {
        memset(gHoFSaveBuffer, 0, SECTOR_SIZE * NUM_HOF_SECTORS);
    }
    else
    {
        if (LoadGameSave(SAVE_HALL_OF_FAME) != SAVE_STATUS_OK)
        {
            memset(gHoFSaveBuffer, 0, SECTOR_SIZE * NUM_HOF_SECTORS);
        }
    }

    for (i = 0; i < HALL_OF_FAME_MAX_TEAMS; i++, lastSavedTeam++)
    {
        if (lastSavedTeam->mon[0].species == SPECIES_NONE)
        {
            break;
        }
    }
    if (i >= HALL_OF_FAME_MAX_TEAMS)
    {
        struct HallofFameTeam *afterTeam = gHoFSaveBuffer;
        struct HallofFameTeam *beforeTeam = gHoFSaveBuffer;
        afterTeam++;
        for (i = 0; i < HALL_OF_FAME_MAX_TEAMS - 1; i++, beforeTeam++, afterTeam++)
        {
            *beforeTeam = *afterTeam;
        }
        lastSavedTeam--;
    }
    *lastSavedTeam = sHof->team;

    DrawDialogueFrame(WIN_SAVE_MESSAGE, FALSE);
    AddTextPrinterParameterized2(WIN_SAVE_MESSAGE, FONT_NORMAL, gText_SavingDontTurnOffPower, 0, NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    CopyWindowToVram(WIN_SAVE_MESSAGE, COPYWIN_FULL);
    gTasks[taskId].func = Task_HofSwSh_TrySave;
}

static void Task_HofSwSh_TrySave(u8 taskId)
{
    gGameContinueCallback = CB2_DoHallOfFameScreenDontSaveData;
    if (TrySavingData(SAVE_HALL_OF_FAME) == SAVE_STATUS_ERROR && gDamagedSaveSectors != 0)
    {
        FreeHallOfFame();
        DestroyTask(taskId);
    }
    else
    {
        PlaySE(SE_SAVE);
        TRY_FREE_AND_SET_NULL(gHoFSaveBuffer);
        gTasks[taskId].tFrameCount = SAVE_MESSAGE_FRAMES;
        gTasks[taskId].func = Task_HofSwSh_WaitSaveMessage;
    }
}

static void Task_HofSwSh_WaitSaveMessage(u8 taskId)
{
    if (gTasks[taskId].tFrameCount)
    {
        gTasks[taskId].tFrameCount--;
    }
    else
    {
        ClearDialogWindowAndFrame(WIN_SAVE_MESSAGE, TRUE);
        gTasks[taskId].func = Task_HofSwSh_IntroSetup;
    }
}

static void CreateFrontSprite(void)
{
    u32 monId = sHof->introMonIds[sHof->monId];
    struct HallofFameMon *mon = &sHof->team.mon[monId];
    enum Species species = sHof->displaySpecies[monId];
    struct Sprite *sprite;

    HandleLoadSpecialPokePic(TRUE, MonSpritesGfxManager_GetSpritePtr(MON_SPR_GFX_MANAGER_A, B_POSITION_OPPONENT_LEFT), species, mon->personality);
    LoadSpritePaletteWithTag(GetMonSpritePalFromSpeciesAndPersonality(species, mon->isShiny, mon->personality), species);
    SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);
    sHof->frontSpriteId = CreateSprite(&gMultiuseSpriteTemplate, -(MON_PIC_WIDTH / 2), FRONT_LANDING_Y, FRONT_SUBPRIORITY);

    sprite = &gSprites[sHof->frontSpriteId];
    FreeSpriteOamMatrix(sprite);
    sprite->callback = SpriteCallbackDummy;
    sprite->oam.priority = MON_OBJ_PRIORITY;
    sprite->sSpecies = species;
}

static void PrintIntroText(void)
{
    struct HallofFameMon *mon = &sHof->team.mon[sHof->introMonIds[sHof->monId]];

    FillWindowPixelBuffer(WIN_INTRO_TITLE, PIXEL_FILL(HOF_COLOR_TRANSPARENT));
    FillWindowPixelBuffer(WIN_INTRO_NICKNAME, PIXEL_FILL(HOF_COLOR_TRANSPARENT));
    FillWindowPixelBuffer(WIN_INTRO_LEVEL, PIXEL_FILL(HOF_COLOR_TRANSPARENT));

    AddTextPrinterParameterized3(WIN_INTRO_TITLE, FONT_SHORT, 0, INTRO_TEXT_TITLE_Y, sTextColors, TEXT_SKIP_DRAW, COMPOUND_STRING("Welcome to the Hall of Fame!"));

    StringCopyN(gStringVar1, mon->nickname, POKEMON_NAME_LENGTH);
    gStringVar1[POKEMON_NAME_LENGTH] = EOS;
    AddTextPrinterParameterized3(WIN_INTRO_NICKNAME, FONT_SHORT, 0, INTRO_TEXT_INFO_Y, sTextColors, TEXT_SKIP_DRAW, gStringVar1);

    ConvertIntToDecimalStringN(StringCopy(gStringVar2, COMPOUND_STRING("{LV}. ")), mon->lvl, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized3(WIN_INTRO_LEVEL, FONT_SHORT, 0, INTRO_TEXT_INFO_Y, sTextColors, TEXT_SKIP_DRAW, gStringVar2);

    PutWindowTilemap(WIN_INTRO_TITLE);
    PutWindowTilemap(WIN_INTRO_NICKNAME);
    PutWindowTilemap(WIN_INTRO_LEVEL);
    CopyWindowToVram(WIN_INTRO_TITLE, COPYWIN_FULL);
    CopyWindowToVram(WIN_INTRO_NICKNAME, COPYWIN_FULL);
    CopyWindowToVram(WIN_INTRO_LEVEL, COPYWIN_FULL);
}

static void LandFrontSprite(void)
{
    struct Sprite *sprite = &gSprites[sHof->frontSpriteId];

    CreateShadowSprite();
    ShowBg(1);
    StartBannerFlash();
    PrintIntroText();

    sprite->sDontFlip = TRUE;
    PlayCry_Normal(sprite->sSpecies, 0);
    PokemonSummaryDoMonAnimation(sprite, sprite->sSpecies, FALSE);
}

static void ClearIntroMon(void)
{
    u32 i;

    DestroyShadowSprite();
    DestroyFrontSprite();
    HideBg(1);
    for (i = WIN_INTRO_TITLE; i <= WIN_INTRO_LEVEL; i++)
    {
        FillWindowPixelBuffer(i, PIXEL_FILL(HOF_COLOR_TRANSPARENT));
        ClearWindowTilemap(i);
        CopyWindowToVram(i, COPYWIN_FULL);
    }
}

static void Task_HofSwSh_IntroSetup(u8 taskId)
{
    u32 monId = sHof->introMonIds[sHof->monId];
    struct HallofFameMon *mon = &sHof->team.mon[monId];
    enum Species species = sHof->displaySpecies[monId];

    LoadTypeWallpaper(GetSpeciesType(species, 0));
    BlendPalettes(INTRO_BG_PALETTES, FADE_COEFF_MAX, RGB_BLACK);
    sHof->introFade = INTRO_FADE_PENDING;

    sHof->backSpriteId = CreateMonPicSprite(species, mon->isShiny, mon->personality, FALSE, DISPLAY_WIDTH + MON_PIC_WIDTH / 2, DISPLAY_HEIGHT - MON_PIC_HEIGHT / 2 + gSpeciesInfo[species].backPicYOffset, PAL_SLOT_BACK_PIC, TAG_NONE);
    gSprites[sHof->backSpriteId].oam.priority = MON_OBJ_PRIORITY;
    StartSlide(DISPLAY_WIDTH + MON_PIC_WIDTH / 2, -(MON_PIC_WIDTH / 2), BACK_SLIDE_FRAMES, ComfyAnimEasing_EaseInOutCubic);
    gTasks[taskId].func = Task_HofSwSh_IntroBackSlide;
}

static void Task_HofSwSh_IntroBackSlide(u8 taskId)
{
    bool32 sliding = UpdateSlide();

    gSprites[sHof->backSpriteId].x = sHof->slideX;

    if (sHof->introFade == INTRO_FADE_PENDING && !sliding && !FreeTempTileDataBuffersIfPossible())
    {
        StartFade(INTRO_BG_PALETTES, 0, INTRO_FADE_IN_FRAMES, RGB_BLACK, ComfyAnimEasing_EaseInCubic);
        sHof->introFade = INTRO_FADE_RUNNING;
    }
    UpdateIntroFadeIn();

    if (sliding || sHof->introFade == INTRO_FADE_PENDING)
    {
        return;
    }

    DestroyBackSprite();
    CreateFrontSprite();
    StartSlide(-(MON_PIC_WIDTH / 2), FRONT_LANDING_X, FRONT_SLIDE_FRAMES, ComfyAnimEasing_EaseOutCubic);
    gTasks[taskId].func = Task_HofSwSh_IntroFrontSlide;
}

static void Task_HofSwSh_IntroFrontSlide(u8 taskId)
{
    struct Sprite *sprite = &gSprites[sHof->frontSpriteId];
    bool32 sliding = UpdateSlide();

    sprite->x = sHof->slideX;
    UpdateIntroFadeIn();
    if (sliding)
    {
        return;
    }

    LandFrontSprite();
    gTasks[taskId].func = Task_HofSwSh_IntroWaitAnim;
}

static void Task_HofSwSh_IntroWaitAnim(u8 taskId)
{
    UpdateIntroFadeIn();
    UpdateBannerFlash();
    if (gSprites[sHof->frontSpriteId].callback != SpriteCallbackDummy)
    {
        return;
    }

    gTasks[taskId].tFrameCount = INTRO_POST_ANIM_FRAMES;
    gTasks[taskId].func = Task_HofSwSh_IntroHold;
}

static void Task_HofSwSh_IntroHold(u8 taskId)
{
    UpdateIntroFadeIn();
    UpdateBannerFlash();
    if (gTasks[taskId].tFrameCount)
    {
        gTasks[taskId].tFrameCount--;
        return;
    }

    StartFade(PALETTES_ALL, FADE_COEFF_MAX, INTRO_FADE_OUT_FRAMES, RGB_BLACK, ComfyAnimEasing_Linear);
    gTasks[taskId].func = Task_HofSwSh_IntroFadeOut;
}

static void Task_HofSwSh_IntroFadeOut(u8 taskId)
{
    UpdateBannerFlash();
    if (UpdateFade())
    {
        return;
    }

    ClearIntroMon();
    sHof->monId++;
    if (sHof->monId < sHof->numIntroMons)
    {
        gTasks[taskId].func = Task_HofSwSh_IntroSetup;
    }
    else
    {
        gTasks[taskId].func = Task_HofSwSh_AssembleSetup;
    }
}

static s16 GetAssembleStartX(u32 i)
{
    return sAssembleMonPositions[i].hFlip ? -(MON_PIC_WIDTH / 2) : DISPLAY_WIDTH + MON_PIC_WIDTH / 2;
}

static void Task_HofSwSh_AssembleSetup(u8 taskId)
{
    FillPalette(RGB_BLACK, 0, PLTT_SIZE);
    sHof->fadeCoeff = 0;
    HideBg(2);
    HideBg(3);

    ReleaseComfyAnim(sHof->slideAnimId);
    sHof->slideAnimId = INVALID_COMFY_ANIM;
    if (sHof->createdGfxManager)
    {
        DestroyMonSpritesGfxManager(MON_SPR_GFX_MANAGER_A);
        sHof->createdGfxManager = FALSE;
    }

    sHof->silhouettePalette = AllocSpritePalette(TAG_MON_SILHOUETTE);
    FillPalette(RGB_WHITE, OBJ_PLTT_ID(sHof->silhouettePalette), PLTT_SIZE_4BPP);
    gTasks[taskId].tSpriteIndex = 0;
    gTasks[taskId].func = Task_HofSwSh_AssembleCreateSprites;
}

static void StartAssembleSlide(u32 i)
{
    struct ComfyAnimEasingConfig config =
    {
        .durationFrames = ASSEMBLE_SLIDE_FRAMES,
        .from = Q_24_8(GetAssembleStartX(i)),
        .to = Q_24_8(sAssembleMonPositions[i].x),
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    };

    sHof->assembleAnimIds[i] = CreateComfyAnim_Easing(&config);
}

static bool32 UpdateAssembleSlides(u32 frame)
{
    u32 i;
    bool32 sliding = FALSE;

    for (i = 0; i < sHof->numTeamMons; i++)
    {
        struct Sprite *sprite = &gSprites[sHof->assembleSpriteIds[i]];

        if (frame < i * ASSEMBLE_SLIDE_STAGGER)
        {
            sliding = TRUE;
            continue;
        }
        if (frame == i * ASSEMBLE_SLIDE_STAGGER)
        {
            StartAssembleSlide(i);
        }

        if (sHof->assembleAnimIds[i] == INVALID_COMFY_ANIM)
        {
            sprite->x = sAssembleMonPositions[i].x;
            continue;
        }

        sprite->x = ReadComfyAnimValueSmooth(&gComfyAnims[sHof->assembleAnimIds[i]]);
        if (gComfyAnims[sHof->assembleAnimIds[i]].completed)
        {
            ReleaseComfyAnim(sHof->assembleAnimIds[i]);
            sHof->assembleAnimIds[i] = INVALID_COMFY_ANIM;
        }
        else
        {
            sliding = TRUE;
        }
    }
    return sliding;
}

static void Task_HofSwSh_AssembleCreateSprites(u8 taskId)
{
    u32 i = gTasks[taskId].tSpriteIndex;
    struct HallofFameMon *mon;
    enum Species species;
    struct Sprite *sprite;

    if (i >= sHof->numTeamMons)
    {
        gTasks[taskId].tFrameCount = 0;
        gTasks[taskId].func = Task_HofSwSh_AssembleSlide;
        return;
    }

    mon = &sHof->team.mon[i];
    species = sHof->displaySpecies[i];
    sHof->assembleSpriteIds[i] = CreateMonPicSprite(species, mon->isShiny, mon->personality, TRUE, GetAssembleStartX(i), sAssembleMonPositions[i].y, i, TAG_NONE);
    sprite = &gSprites[sHof->assembleSpriteIds[i]];
    sprite->oam.paletteNum = sHof->silhouettePalette;
    sprite->oam.priority = MON_OBJ_PRIORITY;
    sprite->subpriority = ASSEMBLE_TRAINER_SUBPRIORITY + 1 + i;
    sprite->hFlip = sAssembleMonPositions[i].hFlip && !IsMonSpriteNotFlipped(species);
    gTasks[taskId].tSpriteIndex++;
}

static void Task_HofSwSh_AssembleSlide(u8 taskId)
{
    if (UpdateAssembleSlides(gTasks[taskId].tFrameCount++))
    {
        return;
    }

    StartFade(PALETTES_ALL, FADE_COEFF_MAX, ASSEMBLE_TO_WHITE_FRAMES, RGB_WHITE, ComfyAnimEasing_Linear);
    gTasks[taskId].func = Task_HofSwSh_AssembleToWhite;
}

static void RevealAssemble(void)
{
    u32 i;
    struct Sprite *sprite;

    for (i = 0; i < sHof->numTeamMons; i++)
    {
        gSprites[sHof->assembleSpriteIds[i]].oam.paletteNum = i;
    }
    FreeSpritePaletteByTag(TAG_MON_SILHOUETTE);

    sHof->trainerSpriteId = CreateTrainerPicSprite(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender), TRUE, ASSEMBLE_TRAINER_X, ASSEMBLE_TRAINER_Y, PAL_SLOT_TRAINER_PIC, TAG_NONE);
    sprite = &gSprites[sHof->trainerSpriteId];
    sprite->oam.priority = MON_OBJ_PRIORITY;
    sprite->subpriority = ASSEMBLE_TRAINER_SUBPRIORITY;

    LoadPalette(sHallOfFameTiles_Pal, BG_PLTT_ID(PAL_HOF_MAIN), PLTT_SIZE_4BPP);
    SetBackdropFromColor(RGB_BLACK);
    LoadPalette(&sHallOfFameTiles_Pal[PAL_BANK_COLORS], BG_PLTT_ID(PAL_HOF_TYPE_BG), PLTT_SIZE_4BPP);

    DecompressDataWithHeaderWram(sAssembleBg2_Tilemap, sHof->bg2TilemapBuffer);
    CopyBgTilemapBufferToVram(2);
    ShowBg(2);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    DecompressDataWithHeaderWram(sAssembleBg1_Tilemap, sHof->bg1TilemapBuffer);
    CopyBgTilemapBufferToVram(1);
}

static void Task_HofSwSh_AssembleToWhite(u8 taskId)
{
    if (UpdateFade())
    {
        return;
    }

    RevealAssemble();
    StartFade(PALETTES_ALL, 0, ASSEMBLE_FROM_WHITE_FRAMES, RGB_WHITE, ComfyAnimEasing_Linear);
    gTasks[taskId].func = Task_HofSwSh_AssembleFromWhite;
}

static void PrintChampionText(void)
{
    u32 hours = gSaveBlock2Ptr->playTimeHours;
    u16 trainerId = (gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0];
    u8 *str;

    FillWindowPixelBuffer(WIN_CHAMPION_TITLE, PIXEL_FILL(HOF_COLOR_TRANSPARENT));
    FillWindowPixelBuffer(WIN_CHAMPION_INFO, PIXEL_FILL(HOF_COLOR_TRANSPARENT));

    AddTextPrinterParameterized3(WIN_CHAMPION_TITLE, FONT_SHORT, 0, CHAMPION_TEXT_Y, sTextColors, TEXT_SKIP_DRAW, COMPOUND_STRING("League Champion! Congratulations!"));
    AddTextPrinterParameterized3(WIN_CHAMPION_INFO, FONT_SHORT, 0, CHAMPION_TEXT_Y, sTextColors, TEXT_SKIP_DRAW, gSaveBlock2Ptr->playerName);

    ConvertIntToDecimalStringN(StringCopy(gStringVar1, COMPOUND_STRING("ID {NO}. ")), trainerId, STR_CONV_MODE_LEADING_ZEROS, 5);
    AddTextPrinterParameterized3(WIN_CHAMPION_INFO, FONT_SHORT, CHAMPION_ID_X, CHAMPION_TEXT_Y, sTextColors, TEXT_SKIP_DRAW, gStringVar1);

    str = ConvertIntToDecimalStringN(gStringVar2, hours, STR_CONV_MODE_LEADING_ZEROS, hours >= 100 ? 3 : 2);
    *str++ = CHAR_COLON;
    ConvertIntToDecimalStringN(str, gSaveBlock2Ptr->playTimeMinutes, STR_CONV_MODE_LEADING_ZEROS, 2);
    AddTextPrinterParameterized3(WIN_CHAMPION_INFO, FONT_SHORT, GetStringRightAlignXOffset(FONT_SHORT, gStringVar2, CHAMPION_INFO_WIDTH * TILE_WIDTH), CHAMPION_TEXT_Y, sTextColors, TEXT_SKIP_DRAW, gStringVar2);

    PutWindowTilemap(WIN_CHAMPION_TITLE);
    PutWindowTilemap(WIN_CHAMPION_INFO);
    CopyWindowToVram(WIN_CHAMPION_TITLE, COPYWIN_FULL);
    CopyWindowToVram(WIN_CHAMPION_INFO, COPYWIN_FULL);
}

static void Task_HofSwSh_AssembleFromWhite(u8 taskId)
{
    if (UpdateFade())
    {
        return;
    }

    ShowBg(1);
    StartBannerFlash();
    PrintChampionText();
    gTasks[taskId].tFrameCount = CONFETTI_DELAY_FRAMES;
    gTasks[taskId].func = Task_HofSwSh_AssembleHold;
}

static void Task_HofSwSh_AssembleHold(u8 taskId)
{
    UpdateBannerFlash();
    if (gTasks[taskId].tFrameCount)
    {
        gTasks[taskId].tFrameCount--;
        return;
    }

    LoadCompressedSpriteSheet(&sSpriteSheet_Confetti);
    LoadSpritePalette(&sSpritePalette_Confetti);
    PlaySE(SE_APPLAUSE);
    gTasks[taskId].tFrameCount = CONFETTI_FRAMES;
    gTasks[taskId].func = Task_HofSwSh_Confetti;
}

static void SpriteCB_Confetti(struct Sprite *sprite)
{
    if (sprite->y2 > CONFETTI_OFFSCREEN_Y)
    {
        DestroySprite(sprite);
        return;
    }

    sprite->y2 += 1 + sprite->sExtraY;
    sprite->x2 = ((Random() % CONFETTI_SWAY_RANGE) + CONFETTI_SWAY_BASE) * gSineTable[(u8)sprite->sSineIdx] / CONFETTI_SINE_SCALE;
    sprite->sSineIdx += CONFETTI_SINE_STEP;
}

static void CreateConfettiSprite(void)
{
    s16 x = Random() % DISPLAY_WIDTH;
    s16 y = -(Random() % CONFETTI_MAX_SPAWN_Y);
    struct Sprite *sprite = &gSprites[CreateSprite(&sSpriteTemplate_Confetti, x, y, 0)];

    StartSpriteAnim(sprite, Random() % ARRAY_COUNT(sAnims_Confetti));
    sprite->sExtraY = (Random() % CONFETTI_EXTRA_Y_ODDS) ? 0 : 1;
}

static void Task_HofSwSh_Confetti(u8 taskId)
{
    if (!gTasks[taskId].tFrameCount)
    {
        gTasks[taskId].func = Task_HofSwSh_PostConfettiHold;
        return;
    }

    gTasks[taskId].tFrameCount--;
    if (gTasks[taskId].tFrameCount % CONFETTI_SPAWN_INTERVAL == 0 && gTasks[taskId].tFrameCount > CONFETTI_FALL_FRAMES)
    {
        CreateConfettiSprite();
    }
}

static void Task_HofSwSh_PostConfettiHold(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        gTasks[taskId].func = Task_HofSwSh_StartExit;
    }
}

static void Task_HofSwSh_StartExit(u8 taskId)
{
    FadeOutBGM(EXIT_BGM_FADE_SPEED);
    StartFade(PALETTES_ALL, FADE_COEFF_MAX, EXIT_FADE_FRAMES, RGB_BLACK, ComfyAnimEasing_Linear);
    gTasks[taskId].func = Task_HofSwSh_Exit;
}

static void Task_HofSwSh_Exit(u8 taskId)
{
    if (UpdateFade())
    {
        return;
    }

    DestroyTask(taskId);
    FreeHallOfFame();
    SetMainCallback2(CB2_StartCreditsSequence);
}

#undef tFrameCount
#undef tSpriteIndex
#undef sSpecies
#undef sDontFlip
#undef sOwnTileNum
#undef sFrontSpriteId
#undef sSineIdx
#undef sExtraY
