#include "../RetroEngine.hpp"

// https://github.com/bubble2k16/snes9x_3ds/blob/3e5cdba3577aafefb0860966a3daf694ece8e168/source/pixform.h#L248
#define BUILD_PIXEL_RGB5551(R,G,B) (((int) (R) << 11) | ((int) (G) << 6) | (int) ((B) << 1) | 1)
#define RGB565_to_RGBA5551(px) (BUILD_PIXEL_RGB5551( (px & 0xf800) >> 11, (px & 0x07e0) >> 6, (px & 0x001f)))

int spriteIndex = 0;
int tileIndex = 0;

C3D_Tex      _3ds_textureData[SURFACE_MAX];
C3D_Tex      _3ds_tilesetData[TILE_MAXSIZE];
_3ds_sprite  _3ds_sprites[SPRITES_MAX];
_3ds_tile    _3ds_tiles[TILES_MAX_3DS];

byte paletteIndex = 0;
byte cachedPalettes = 0;
byte maxPaletteCycles = 0;

/*
   JeffRuLz's texture handling code from OpenHCL was referenced heavily here
   swapPixel, powOfTwo, and swizzle were also directly lifted from its code
   See here:  https://github.com/JeffRuLz/OpenHCL/blob/master/platform/3ds/source/graphics.cpp
*/

static inline void swapPixel(u16 &a, u16 &b)
{
	u32 tmp = a;
	a = b;
	b = tmp;
}

static inline void swizzle(u16* p, bool reverse)
{
	// swizzle foursome table
	static const unsigned char table[][4] =
	{
		{  2,  8, 16,  4, },
		{  3,  9, 17,  5, },
		{  6, 10, 24, 20, },
		{  7, 11, 25, 21, },
		{ 14, 26, 28, 22, },
		{ 15, 27, 29, 23, },
		{ 34, 40, 48, 36, },
		{ 35, 41, 49, 37, },
		{ 38, 42, 56, 52, },
		{ 39, 43, 57, 53, },
		{ 46, 58, 60, 54, },
		{ 47, 59, 61, 55, }
	};

	if (!reverse) {
		for (const auto &entry : table)
		{
			u16 tmp = p[entry[0]];
			p[entry[0]] = p[entry[1]];
			p[entry[1]] = p[entry[2]];
			p[entry[2]] = p[entry[3]];
			p[entry[3]] = tmp;
		}
	} else {
		for (const auto &entry : table) {
			u16 tmp = p[entry[3]];
			p[entry[3]] = p[entry[2]];
			p[entry[2]] = p[entry[1]];
			p[entry[1]] = p[entry[0]];
			p[entry[0]] = tmp;
		}
	}

	swapPixel(p[12], p[18]);
	swapPixel(p[13], p[19]);
	swapPixel(p[44], p[50]);
	swapPixel(p[45], p[51]);
}

static inline int powOfTwo(int in)
{
	int out = 8;
	while (out < in)
		out *= 2;
	return out;
}

// 3DS doesn't like 16x16384 textures
// who knew
void _3ds_rearrangeTileData(byte* gfxDataPtr, byte* dstPtr) {
	const int swidth = 16,  sheight = 16384;
	const int dwidth = 512, dheight = 512;
	int sx = 0, sy = 0, dx = 0, dy = 0;
	byte* sptr = gfxDataPtr;
	byte* dptr = dstPtr;

	for (int i = 0; i < 512 * 512; i++) {
		dptr = dstPtr + (dwidth * dy) + dx;

		*dptr = *sptr;

		sptr++;
		sx++;
		if (sx == swidth) {
			sx = 0;
			sy++;
		}

		SWTilePosToHWTilePos(sx, sy, &dx, &dy);
	}
}

void _3ds_cacheSpriteSurface(int sheetID) {
	GFXSurface* surf = &gfxSurface[sheetID];
	int height = surf->height;
	int width  = surf->width;
	int depth  = surf->depth;
    	byte *gfxDataPtr   = &graphicData[surf->dataPosition];
	_3ds_cacheGfxSurface(gfxDataPtr, &_3ds_textureData[sheetID], width, height, false);
}

void _3ds_delSpriteSurface(int sheetID) {
	C3D_TexDelete(&_3ds_textureData[sheetID]);
}

void _3ds_cacheTileSurface(byte* tilesetGfxPtr) {
	//if (cachedPalettes >= TILE_MAXSIZE)
	//	return;

	byte* temp = (byte*) malloc(512 * 512 * sizeof(byte));
	_3ds_rearrangeTileData(tilesetGfxPtr, temp);
	_3ds_cacheGfxSurface(temp, &_3ds_tilesetData[cachedPalettes], 512, 512, true);
	cachedPalettes++;
	free(temp);
}

void _3ds_delTileSurface() {
	for (int i = 0; i < cachedPalettes; i++) 
		C3D_TexDelete(&_3ds_tilesetData[i]);

	// does this fix that memory leak?
	cachedPalettes = 0;
}

void _3ds_cacheGfxSurface(byte* gfxDataPtr, C3D_Tex* dst,
			  int width, int height, bool write) {
        byte *lineBuffer   = &gfxLineBuffer[0];
	activePalette = fullPalette[*lineBuffer];

	if (width <= 0 || height <= 0)
		return;

	int x = 0, y = 0;
	int w = powOfTwo(width);
	int h = powOfTwo(height);

	u16* buffer     = (u16*) linearAlloc(w * h * sizeof(u16));
	u16* bufferPtr  = buffer;
	byte* gptr = gfxDataPtr;
 	for (int i = 0; i < w * h; ) {
		u16  tile[8 * 8] = { 0 };
		u16* tilePtr = tile;
		
		for (int ty = 0; ty < 8; ty++) {
			gptr = gfxDataPtr + (width * y) + x;
			for (int tx = 0; tx < 8; tx++) {
				if (x < width && y < height) {
					if (*gptr > 0)
						*tilePtr = RGB565_to_RGBA5551(activePalette[*gptr]);
					else
						*tilePtr = 0;
				}
				tilePtr++;

				gptr++;
				x++;
				i++;
			}

			x -= 8;
			y++;
		}

		x += 8;
		y -= 8;

		if (x >= w - 1) {
			x =  0;
			y += 8;
		}

		swizzle(tile, false);
		
		for (int a = 0; a < 8 * 8; a++)
			*(bufferPtr++) = tile[a];
	}

	C3D_TexInit(dst, w, h, GPU_RGBA5551);
	C3D_TexUpload(dst, buffer);

	// write buffer to file
	/*
	if (write) {
		FILE* f = fopen("/temp.buf", "w+");
		fwrite(buffer, w * h * sizeof(s16), 1, f);
		fclose(f);
		printf("saved to temp.buf\n");
	}
	*/

	linearFree(buffer);
}


