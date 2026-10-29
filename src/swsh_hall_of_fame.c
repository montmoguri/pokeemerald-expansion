#include "global.h"
#include "swsh_hall_of_fame.h"
#include "hall_of_fame.h"
#include "bg.h"
#include "battle.h"
#include "comfy_anim.h"
#include "credits.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokemon.h"
#include "save.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define HALL_OF_FAME_MAX_TEAMS  30
#define SAVE_MESSAGE_FRAMES     32
#define EXIT_FADE_FRAMES        16
#define EXIT_BGM_FADE_SPEED     4
#define FADE_COEFF_MAX          16

#define INTRO_FADE_IN_FRAMES    48
#define INTRO_POST_ANIM_FRAMES  120
#define INTRO_FADE_OUT_FRAMES   16
#define INTRO_BG2_BLEND_EVA     10
#define INTRO_BG2_BLEND_EVB     6
#define TYPE_BG_SCROLL_SPEED    64

#define BACK_SLIDE_FRAMES       60
#define FRONT_SLIDE_FRAMES      48
#define FRONT_LANDING_X         172
#define FRONT_LANDING_Y         78
#define FRONT_SUBPRIORITY       0
#define MON_OBJ_PRIORITY        2
#define PAL_SLOT_BACK_PIC       0

#define NUM_RESERVED_OBJ_PALETTES 8

#define sSpecies                data[0]
#define sDontFlip               data[1]

#define TILEMAP_PALETTE_SHIFT   12
#define TILEMAP_TILE_MASK       0x0FFF

#define PAL_HOF_MAIN            0
#define PAL_HOF_TYPE_BG         1
#define INTRO_BG_PALETTES       ((1 << PAL_HOF_MAIN) | (1 << PAL_HOF_TYPE_BG))

#define SAVE_MESSAGE_LEFT       2
#define SAVE_MESSAGE_TOP        15
#define SAVE_MESSAGE_WIDTH      27
#define SAVE_MESSAGE_HEIGHT     4
#define SAVE_MESSAGE_BASE_BLOCK 0x194

STATIC_ASSERT(sizeof(struct HallofFameTeam) * HALL_OF_FAME_MAX_TEAMS <= SECTOR_DATA_SIZE * NUM_HOF_SECTORS, HallOfFameSwShFreeSpace);

enum
{
    WIN_SAVE_MESSAGE,
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
    u8 bg0TilemapBuffer[BG_SCREEN_SIZE];
    u8 bg2TilemapBuffer[BG_SCREEN_SIZE];
    u16 bg3TilemapBuffer[BG_SCREEN_SIZE / 2];
    u32 fadePalettes;
    u32 fadeColor;
    u32 fadeAnimId;
    u32 slideAnimId;
    s16 slideX;
    s16 slideTo;
    u8 fadeCoeff;
    bool8 dontSaveData;
    u8 monId;
    u8 introFade;
    u8 backSpriteId;
    u8 frontSpriteId;
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
static void Task_HofSwSh_Exit(u8 taskId);

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
    DUMMY_WIN_TEMPLATE
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

static void StartFade(u32 palettes, u32 toCoeff, u32 frames, u32 color)
{
    struct ComfyAnimEasingConfig config =
    {
        .durationFrames = frames,
        .from = Q_24_8(sHof->fadeCoeff),
        .to = Q_24_8(toCoeff),
        .easingFunc = ComfyAnimEasing_Linear,
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
        sHof->backSpriteId = SPRITE_NONE;
        sHof->frontSpriteId = SPRITE_NONE;
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
        ReleaseComfyAnims();
        gMain.state++;
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetBgTilemapBuffer(0, sHof->bg0TilemapBuffer);
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

static void DestroyFrontSprite(void)
{
    if (sHof->frontSpriteId != SPRITE_NONE)
    {
        StopPokemonAnimationDelayTask();
        DestroySpriteAndFreeResources(&gSprites[sHof->frontSpriteId]);
        sHof->frontSpriteId = SPRITE_NONE;
    }
}

static void FreeHallOfFame(void)
{
    DestroyBackSprite();
    DestroyFrontSprite();
    if (sHof->createdGfxManager)
    {
        DestroyMonSpritesGfxManager(MON_SPR_GFX_MANAGER_A);
    }
    HideBg(0);
    HideBg(2);
    HideBg(3);
    FreeAllWindowBuffers();
    UnsetBgTilemapBuffer(0);
    UnsetBgTilemapBuffer(2);
    UnsetBgTilemapBuffer(3);
    ResetBgsAndClearDma3BusyFlags(0);
    ReleaseComfyAnims();
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
        }
        else
        {
            sHof->team.mon[i].species = SPECIES_NONE;
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
            memset(gHoFSaveBuffer, 0, SECTOR_SIZE * NUM_HOF_SECTORS);
    }

    for (i = 0; i < HALL_OF_FAME_MAX_TEAMS; i++, lastSavedTeam++)
    {
        if (lastSavedTeam->mon[0].species == SPECIES_NONE)
            break;
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
    struct HallofFameMon *mon = &sHof->team.mon[sHof->monId];
    struct Sprite *sprite;

    HandleLoadSpecialPokePic(TRUE, MonSpritesGfxManager_GetSpritePtr(MON_SPR_GFX_MANAGER_A, B_POSITION_OPPONENT_LEFT), mon->species, mon->personality);
    LoadSpritePaletteWithTag(GetMonSpritePalFromSpeciesAndPersonality(mon->species, mon->isShiny, mon->personality), mon->species);
    SetMultiuseSpriteTemplateToPokemon(mon->species, B_POSITION_OPPONENT_LEFT);
    sHof->frontSpriteId = CreateSprite(&gMultiuseSpriteTemplate, -(MON_PIC_WIDTH / 2), FRONT_LANDING_Y, FRONT_SUBPRIORITY);

    sprite = &gSprites[sHof->frontSpriteId];
    FreeSpriteOamMatrix(sprite);
    sprite->callback = SpriteCallbackDummy;
    sprite->oam.priority = MON_OBJ_PRIORITY;
    sprite->sSpecies = mon->species;
    sprite->hFlip = IsMonSpriteNotFlipped(mon->species);
}

static void Task_HofSwSh_IntroSetup(u8 taskId)
{
    struct HallofFameMon *mon = &sHof->team.mon[sHof->monId];

    LoadTypeWallpaper(GetSpeciesType(mon->species, 0));
    BlendPalettes(INTRO_BG_PALETTES, FADE_COEFF_MAX, RGB_BLACK);
    sHof->introFade = INTRO_FADE_PENDING;

    sHof->backSpriteId = CreateMonPicSprite(mon->species, mon->isShiny, mon->personality, FALSE, DISPLAY_WIDTH + MON_PIC_WIDTH / 2, DISPLAY_HEIGHT - MON_PIC_HEIGHT / 2 + gSpeciesInfo[mon->species].backPicYOffset, PAL_SLOT_BACK_PIC, TAG_NONE);
    gSprites[sHof->backSpriteId].oam.priority = MON_OBJ_PRIORITY;
    StartSlide(DISPLAY_WIDTH + MON_PIC_WIDTH / 2, -(MON_PIC_WIDTH / 2), BACK_SLIDE_FRAMES, ComfyAnimEasing_Linear);
    gTasks[taskId].func = Task_HofSwSh_IntroBackSlide;
}

static void Task_HofSwSh_IntroBackSlide(u8 taskId)
{
    bool32 sliding = UpdateSlide();

    gSprites[sHof->backSpriteId].x = sHof->slideX;

    if (sHof->introFade == INTRO_FADE_PENDING && !sliding && !FreeTempTileDataBuffersIfPossible())
    {
        StartFade(INTRO_BG_PALETTES, 0, INTRO_FADE_IN_FRAMES, RGB_BLACK);
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

    sprite->sDontFlip = !sprite->hFlip;
    PlayCry_Normal(sprite->sSpecies, 0);
    PokemonSummaryDoMonAnimation(sprite, sprite->sSpecies, FALSE);
    gTasks[taskId].func = Task_HofSwSh_IntroWaitAnim;
}

static void Task_HofSwSh_IntroWaitAnim(u8 taskId)
{
    UpdateIntroFadeIn();
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
    if (gTasks[taskId].tFrameCount)
    {
        gTasks[taskId].tFrameCount--;
        return;
    }

    StartFade(PALETTES_ALL, FADE_COEFF_MAX, INTRO_FADE_OUT_FRAMES, RGB_BLACK);
    gTasks[taskId].func = Task_HofSwSh_IntroFadeOut;
}

static void Task_HofSwSh_IntroFadeOut(u8 taskId)
{
    if (UpdateFade())
    {
        return;
    }

    DestroyFrontSprite();
    sHof->monId++;
    if (sHof->monId < PARTY_SIZE && sHof->team.mon[sHof->monId].species != SPECIES_NONE)
    {
        gTasks[taskId].func = Task_HofSwSh_IntroSetup;
    }
    else
    {
        gTasks[taskId].func = Task_HofSwSh_StartExit;
    }
}

static void Task_HofSwSh_StartExit(u8 taskId)
{
    FadeOutBGM(EXIT_BGM_FADE_SPEED);
    StartFade(PALETTES_ALL, FADE_COEFF_MAX, EXIT_FADE_FRAMES, RGB_BLACK);
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
#undef sSpecies
#undef sDontFlip
