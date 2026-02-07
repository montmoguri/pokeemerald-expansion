enum {
    SWSH_WALLPAPER_BASE,
    SWSH_WALLPAPER_NORMAL,
    SWSH_WALLPAPER_FIGHTING,
    SWSH_WALLPAPER_FLYING,
    SWSH_WALLPAPER_COUNT
};
// #define MAX_DEFAULT_WALLPAPER WALLPAPER_FLYING

static const u32 sSwShStorage_Gfx[]         = INCBIN_U32("graphics/pokemon_storage/swsh/tiles.4bpp.smol");
static const u16 sSwShStorage_Pal[]         = INCBIN_U16("graphics/pokemon_storage/swsh/tiles.gbapal");
static const u32 sSwShStorage_BG1_Tilemap[] = INCBIN_U32("graphics/pokemon_storage/swsh/bg1.bin.smolTM");
static const u32 sSwShStorage_BG2_Tilemap[] = INCBIN_U32("graphics/pokemon_storage/swsh/bg2.bin.smolTM");

static const u32 sWallpaperTiles_Base[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/base.4bpp.smol");
static const u32 sWallpaperTilemap_Base[] = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/base.bin.smolTM");
static const u16 sWallpaperPalette_Base[] = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/base.gbapal");

static const u32 sWallpaperTiles_Normal[]   = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/normal.4bpp.smol");
static const u32 sWallpaperTilemap_Normal[] = INCBIN_U32("graphics/pokemon_storage/swsh/wallpapers/normal.bin.smolTM");
static const u16 sWallpaperPalette_Normal[] = INCBIN_U16("graphics/pokemon_storage/swsh/wallpapers/normal.gbapal");
