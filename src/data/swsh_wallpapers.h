enum {
    SWSH_WALLPAPER_BASE,
    SWSH_WALLPAPER_NORMAL,
    SWSH_WALLPAPER_FIGHTING,
    SWSH_WALLPAPER_FLYING,
    SWSH_WALLPAPER_POISON,
    SWSH_WALLPAPER_GROUND,
    SWSH_WALLPAPER_ROCK,
    SWSH_WALLPAPER_BUG,
    SWSH_WALLPAPER_GHOST,
    SWSH_WALLPAPER_STEEL,
    SWSH_WALLPAPER_FIRE,
    SWSH_WALLPAPER_WATER,
    SWSH_WALLPAPER_GRASS,
    SWSH_WALLPAPER_ELECTRIC,
    SWSH_WALLPAPER_PSYCHIC,
    SWSH_WALLPAPER_ICE,
    SWSH_WALLPAPER_DRAGON,
    SWSH_WALLPAPER_DARK,
    SWSH_WALLPAPER_FAIRY,
    SWSH_WALLPAPER_COUNT
};
// #define MAX_DEFAULT_WALLPAPER WALLPAPER_FLYING

static const u32 sSwShStorage_Gfx[]           = INCBIN_U32("graphics/pokemon_storage/swsh/tiles.4bpp.smol");
static const u16 sSwShStorage_Pal[]           = INCBIN_U16("graphics/pokemon_storage/swsh/tiles.gbapal");
static const u32 sSwShStorage_BG1_Tilemap[]   = INCBIN_U32("graphics/pokemon_storage/swsh/bg1.bin.smolTM");
static const u32 sSwShStorage_BG2_Tilemap[]   = INCBIN_U32("graphics/pokemon_storage/swsh/bg2.bin.smolTM");

static const u32 sWallpaperTiles_Base[]       = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/base.4bpp.smol");
static const u32 sWallpaperTilemap_Base[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/base.bin.smolTM");
static const u16 sWallpaperPalette_Base[]     = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/base.gbapal");

static const u32 sWallpaperTiles_Box[]        = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/box.4bpp.smol");
static const u32 sWallpaperTilemap_Box[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/box.bin.smolTM");
static const u16 sWallpaperPalette_Box[]      = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/box.gbapal");

static const u32 sWallpaperTiles_Normal[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/normal.4bpp.smol");
static const u32 sWallpaperTilemap_Normal[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/normal.bin.smolTM");
static const u16 sWallpaperPalette_Normal[]   = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/normal.gbapal");

static const u32 sWallpaperTiles_Fighting[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/fighting.4bpp.smol");
static const u32 sWallpaperTilemap_Fighting[] = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/fighting.bin.smolTM");
static const u16 sWallpaperPalette_Fighting[] = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/fighting.gbapal");

static const u32 sWallpaperTiles_Flying[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/flying.4bpp.smol");
static const u32 sWallpaperTilemap_Flying[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/flying.bin.smolTM");
static const u16 sWallpaperPalette_Flying[]   = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/flying.gbapal");

static const u32 sWallpaperTiles_Poison[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/poison.4bpp.smol");
static const u32 sWallpaperTilemap_Poison[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/poison.bin.smolTM");
static const u16 sWallpaperPalette_Poison[]   = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/poison.gbapal");

static const u32 sWallpaperTiles_Ground[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/ground.4bpp.smol");
static const u32 sWallpaperTilemap_Ground[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/ground.bin.smolTM");
static const u16 sWallpaperPalette_Ground[]   = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/ground.gbapal");

static const u32 sWallpaperTiles_Rock[]       = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/rock.4bpp.smol");
static const u32 sWallpaperTilemap_Rock[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/rock.bin.smolTM");
static const u16 sWallpaperPalette_Rock[]     = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/rock.gbapal");

static const u32 sWallpaperTiles_Bug[]        = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/bug.4bpp.smol");
static const u32 sWallpaperTilemap_Bug[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/bug.bin.smolTM");
static const u16 sWallpaperPalette_Bug[]      = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/bug.gbapal");

static const u32 sWallpaperTiles_Ghost[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/ghost.4bpp.smol");
static const u32 sWallpaperTilemap_Ghost[]    = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/ghost.bin.smolTM");
static const u16 sWallpaperPalette_Ghost[]    = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/ghost.gbapal");

static const u32 sWallpaperTiles_Steel[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/steel.4bpp.smol");
static const u32 sWallpaperTilemap_Steel[]    = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/steel.bin.smolTM");
static const u16 sWallpaperPalette_Steel[]    = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/steel.gbapal");

static const u32 sWallpaperTiles_Fire[]       = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/fire.4bpp.smol");
static const u32 sWallpaperTilemap_Fire[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/fire.bin.smolTM");
static const u16 sWallpaperPalette_Fire[]     = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/fire.gbapal");

static const u32 sWallpaperTiles_Water[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/water.4bpp.smol");
static const u32 sWallpaperTilemap_Water[]    = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/water.bin.smolTM");
static const u16 sWallpaperPalette_Water[]    = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/water.gbapal");

static const u32 sWallpaperTiles_Grass[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/grass.4bpp.smol");
static const u32 sWallpaperTilemap_Grass[]    = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/grass.bin.smolTM");
static const u16 sWallpaperPalette_Grass[]    = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/grass.gbapal");

static const u32 sWallpaperTiles_Electric[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/electric.4bpp.smol");
static const u32 sWallpaperTilemap_Electric[] = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/electric.bin.smolTM");
static const u16 sWallpaperPalette_Electric[] = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/electric.gbapal");

static const u32 sWallpaperTiles_Psychic[]    = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/psychic.4bpp.smol");
static const u32 sWallpaperTilemap_Psychic[]  = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/psychic.bin.smolTM");
static const u16 sWallpaperPalette_Psychic[]  = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/psychic.gbapal");

static const u32 sWallpaperTiles_Ice[]        = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/ice.4bpp.smol");
static const u32 sWallpaperTilemap_Ice[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/ice.bin.smolTM");
static const u16 sWallpaperPalette_Ice[]      = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/ice.gbapal");

static const u32 sWallpaperTiles_Dragon[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/dragon.4bpp.smol");
static const u32 sWallpaperTilemap_Dragon[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/dragon.bin.smolTM");
static const u16 sWallpaperPalette_Dragon[]   = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/dragon.gbapal");

static const u32 sWallpaperTiles_Dark[]       = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/dark.4bpp.smol");
static const u32 sWallpaperTilemap_Dark[]     = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/dark.bin.smolTM");
static const u16 sWallpaperPalette_Dark[]     = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/dark.gbapal");

static const u32 sWallpaperTiles_Fairy[]      = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/fairy.4bpp.smol");
static const u32 sWallpaperTilemap_Fairy[]    = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/fairy.bin.smolTM");
static const u16 sWallpaperPalette_Fairy[]    = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/fairy.gbapal");
