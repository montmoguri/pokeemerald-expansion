#include "global.h"
#include "swsh_hall_of_fame.h"
#include "hall_of_fame.h"
#include "bg.h"
#include "comfy_anim.h"
#include "credits.h"
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

struct HallOfFameSwSh
{
    struct HallofFameTeam team;
    u8 bg0TilemapBuffer[BG_SCREEN_SIZE];
    u32 fadePalettes;
    u32 fadeColor;
    u32 fadeAnimId;
    bool8 dontSaveData;
};

static EWRAM_DATA struct HallOfFameSwSh *sHof = NULL;

static void Task_HofSwSh_InitTeam(u8 taskId);
static void Task_HofSwSh_InsertTeam(u8 taskId);
static void Task_HofSwSh_TrySave(u8 taskId);
static void Task_HofSwSh_WaitSaveMessage(u8 taskId);
static void Task_HofSwSh_StartExit(u8 taskId);
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

static void StartFade(u32 palettes, u32 fromCoeff, u32 toCoeff, u32 frames, u32 color)
{
    struct ComfyAnimEasingConfig config =
    {
        .durationFrames = frames,
        .from = Q_24_8(fromCoeff),
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

    BlendPalettes(palettes, fromCoeff, color);
    if (sHof->fadeAnimId == INVALID_COMFY_ANIM)
    {
        BlendPalettes(palettes, toCoeff, color);
    }
}

static bool32 UpdateFade(void)
{
    if (sHof->fadeAnimId == INVALID_COMFY_ANIM)
    {
        return FALSE;
    }

    BlendPalettes(sHof->fadePalettes, ReadComfyAnimValueSmooth(&gComfyAnims[sHof->fadeAnimId]), sHof->fadeColor);
    return !gComfyAnims[sHof->fadeAnimId].completed;
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
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        ResetTempTileDataBuffers();
        ResetAllPicSprites();
        FreeAllSpritePalettes();
        ReleaseComfyAnims();
        gMain.state++;
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetBgTilemapBuffer(0, sHof->bg0TilemapBuffer);
        ResetAllBgsCoordinates();
        InitWindows(sWindowTemplates);
        InitTextBoxGfxAndPrinters();
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        gMain.state++;
        break;
    case 3:
        SetVBlankCallback(VBlankCB_HallOfFameSwSh);
        SetMainCallback2(CB2_HallOfFameSwSh);
        PlayBGM(MUS_HALL_OF_FAME);
        return FALSE;
    }
    return TRUE;
}

static void FreeHallOfFame(void)
{
    HideBg(0);
    FreeAllWindowBuffers();
    UnsetBgTilemapBuffer(0);
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
        gTasks[taskId].func = Task_HofSwSh_StartExit;
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
        gTasks[taskId].func = Task_HofSwSh_StartExit;
    }
}

static void Task_HofSwSh_StartExit(u8 taskId)
{
    FadeOutBGM(EXIT_BGM_FADE_SPEED);
    StartFade(PALETTES_ALL, 0, FADE_COEFF_MAX, EXIT_FADE_FRAMES, RGB_BLACK);
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
