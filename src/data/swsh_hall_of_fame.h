struct Wallpaper
{
    const u32 *tiles;
    const u32 *tilemap;
    const u16 *palette;
};

static const u32 sWallpaperTiles_Normal[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/normal.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Normal[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/normal.bin", ".smolTM");
static const u16 sWallpaperPalette_Normal[]   = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/normal.png", ".gbapal");

static const u32 sWallpaperTiles_Fighting[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/fighting.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Fighting[] = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/fighting.bin", ".smolTM");
static const u16 sWallpaperPalette_Fighting[] = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/fighting.png", ".gbapal");

static const u32 sWallpaperTiles_Flying[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/flying.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Flying[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/flying.bin", ".smolTM");
static const u16 sWallpaperPalette_Flying[]   = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/flying.png", ".gbapal");

static const u32 sWallpaperTiles_Poison[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/poison.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Poison[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/poison.bin", ".smolTM");
static const u16 sWallpaperPalette_Poison[]   = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/poison.png", ".gbapal");

static const u32 sWallpaperTiles_Ground[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/ground.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Ground[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/ground.bin", ".smolTM");
static const u16 sWallpaperPalette_Ground[]   = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/ground.png", ".gbapal");

static const u32 sWallpaperTiles_Rock[]       = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/rock.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Rock[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/rock.bin", ".smolTM");
static const u16 sWallpaperPalette_Rock[]     = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/rock.png", ".gbapal");

static const u32 sWallpaperTiles_Bug[]        = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/bug.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Bug[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/bug.bin", ".smolTM");
static const u16 sWallpaperPalette_Bug[]      = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/bug.png", ".gbapal");

static const u32 sWallpaperTiles_Ghost[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/ghost.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Ghost[]    = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/ghost.bin", ".smolTM");
static const u16 sWallpaperPalette_Ghost[]    = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/ghost.png", ".gbapal");

static const u32 sWallpaperTiles_Steel[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/steel.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Steel[]    = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/steel.bin", ".smolTM");
static const u16 sWallpaperPalette_Steel[]    = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/steel.png", ".gbapal");

static const u32 sWallpaperTiles_Fire[]       = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/fire.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Fire[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/fire.bin", ".smolTM");
static const u16 sWallpaperPalette_Fire[]     = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/fire.png", ".gbapal");

static const u32 sWallpaperTiles_Water[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/water.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Water[]    = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/water.bin", ".smolTM");
static const u16 sWallpaperPalette_Water[]    = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/water.png", ".gbapal");

static const u32 sWallpaperTiles_Grass[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/grass.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Grass[]    = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/grass.bin", ".smolTM");
static const u16 sWallpaperPalette_Grass[]    = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/grass.png", ".gbapal");

static const u32 sWallpaperTiles_Electric[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/electric.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Electric[] = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/electric.bin", ".smolTM");
static const u16 sWallpaperPalette_Electric[] = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/electric.png", ".gbapal");

static const u32 sWallpaperTiles_Psychic[]    = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/psychic.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Psychic[]  = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/psychic.bin", ".smolTM");
static const u16 sWallpaperPalette_Psychic[]  = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/psychic.png", ".gbapal");

static const u32 sWallpaperTiles_Ice[]        = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/ice.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Ice[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/ice.bin", ".smolTM");
static const u16 sWallpaperPalette_Ice[]      = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/ice.png", ".gbapal");

static const u32 sWallpaperTiles_Dragon[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/dragon.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Dragon[]   = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/dragon.bin", ".smolTM");
static const u16 sWallpaperPalette_Dragon[]   = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/dragon.png", ".gbapal");

static const u32 sWallpaperTiles_Dark[]       = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/dark.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Dark[]     = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/dark.bin", ".smolTM");
static const u16 sWallpaperPalette_Dark[]     = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/dark.png", ".gbapal");

static const u32 sWallpaperTiles_Fairy[]      = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/fairy.png", ".4bpp.smol");
static const u32 sWallpaperTilemap_Fairy[]    = INCGFX_U32("graphics/pokemon_storage/swsh/wallpapers/fairy.bin", ".smolTM");
static const u16 sWallpaperPalette_Fairy[]    = INCGFX_U16("graphics/pokemon_storage/swsh/wallpapers/fairy.png", ".gbapal");

#define WALLPAPER_ENTRY(name) {sWallpaperTiles_##name, sWallpaperTilemap_##name, sWallpaperPalette_##name}

static const struct Wallpaper sTypeWallpapers[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = WALLPAPER_ENTRY(Normal),
    [TYPE_FIGHTING] = WALLPAPER_ENTRY(Fighting),
    [TYPE_FLYING]   = WALLPAPER_ENTRY(Flying),
    [TYPE_POISON]   = WALLPAPER_ENTRY(Poison),
    [TYPE_GROUND]   = WALLPAPER_ENTRY(Ground),
    [TYPE_ROCK]     = WALLPAPER_ENTRY(Rock),
    [TYPE_BUG]      = WALLPAPER_ENTRY(Bug),
    [TYPE_GHOST]    = WALLPAPER_ENTRY(Ghost),
    [TYPE_STEEL]    = WALLPAPER_ENTRY(Steel),
    [TYPE_FIRE]     = WALLPAPER_ENTRY(Fire),
    [TYPE_WATER]    = WALLPAPER_ENTRY(Water),
    [TYPE_GRASS]    = WALLPAPER_ENTRY(Grass),
    [TYPE_ELECTRIC] = WALLPAPER_ENTRY(Electric),
    [TYPE_PSYCHIC]  = WALLPAPER_ENTRY(Psychic),
    [TYPE_ICE]      = WALLPAPER_ENTRY(Ice),
    [TYPE_DRAGON]   = WALLPAPER_ENTRY(Dragon),
    [TYPE_DARK]     = WALLPAPER_ENTRY(Dark),
    [TYPE_FAIRY]    = WALLPAPER_ENTRY(Fairy),
};

static const u32 sHallOfFameTiles_Gfx[]       = INCGFX_U32("graphics/hall_of_fame/tiles.png", ".4bpp.smol");
static const u16 sHallOfFameTiles_Pal[]       = INCGFX_U16("graphics/hall_of_fame/tiles.png", ".gbapal");
static const u32 sIntroBg2_Tilemap[]          = INCGFX_U32("graphics/hall_of_fame/intro_bg2.bin", ".smolTM");
static const u32 sIntroBg1_Tilemap[]          = INCGFX_U32("graphics/hall_of_fame/intro_bg1.bin", ".smolTM");
static const u32 sAssembleBg2_Tilemap[]       = INCGFX_U32("graphics/hall_of_fame/assemble_bg2.bin", ".smolTM");
static const u32 sAssembleBg1_Tilemap[]       = INCGFX_U32("graphics/hall_of_fame/assemble_bg1.bin", ".smolTM");

static const struct CompressedSpriteSheet sSpriteSheet_Confetti =
{
    .data = gConfetti_Gfx,
    .size = CONFETTI_SHEET_SIZE,
    .tag = TAG_CONFETTI
};

static const struct SpritePalette sSpritePalette_Confetti =
{
    .data = gConfetti_Pal,
    .tag = TAG_CONFETTI
};

static const struct OamData sOamData_Confetti =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_PinkConfettiA[] =
{
    ANIMCMD_FRAME(0, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_RedConfettiA[] =
{
    ANIMCMD_FRAME(1, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BlueConfettiA[] =
{
    ANIMCMD_FRAME(2, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_RedConfettiB[] =
{
    ANIMCMD_FRAME(3, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BlueConfettiB[] =
{
    ANIMCMD_FRAME(4, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_YellowConfettiA[] =
{
    ANIMCMD_FRAME(5, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_WhiteConfettiA[] =
{
    ANIMCMD_FRAME(6, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GreenConfettiA[] =
{
    ANIMCMD_FRAME(7, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PinkConfettiB[] =
{
    ANIMCMD_FRAME(8, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BlueConfettiC[] =
{
    ANIMCMD_FRAME(9, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_YellowConfettiB[] =
{
    ANIMCMD_FRAME(10, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_WhiteConfettiB[] =
{
    ANIMCMD_FRAME(11, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GreenConfettiB[] =
{
    ANIMCMD_FRAME(12, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PinkConfettiC[] =
{
    ANIMCMD_FRAME(13, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_RedConfettiC[] =
{
    ANIMCMD_FRAME(14, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_YellowConfettiC[] =
{
    ANIMCMD_FRAME(15, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_WhiteConfettiC[] =
{
    ANIMCMD_FRAME(16, 30),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_Confetti[] =
{
    sAnim_PinkConfettiA,
    sAnim_RedConfettiA,
    sAnim_BlueConfettiA,
    sAnim_RedConfettiB,
    sAnim_BlueConfettiB,
    sAnim_YellowConfettiA,
    sAnim_WhiteConfettiA,
    sAnim_GreenConfettiA,
    sAnim_PinkConfettiB,
    sAnim_BlueConfettiC,
    sAnim_YellowConfettiB,
    sAnim_WhiteConfettiB,
    sAnim_GreenConfettiB,
    sAnim_PinkConfettiC,
    sAnim_RedConfettiC,
    sAnim_YellowConfettiC,
    sAnim_WhiteConfettiC
};

static const struct SpriteTemplate sSpriteTemplate_Confetti =
{
    .tileTag = TAG_CONFETTI,
    .paletteTag = TAG_CONFETTI,
    .oam = &sOamData_Confetti,
    .anims = sAnims_Confetti,
    .callback = SpriteCB_Confetti
};
