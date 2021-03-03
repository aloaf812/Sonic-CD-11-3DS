#ifndef RENDER_3DS_H
#define RENDER_3DS_H

#define MAX_TILES_PER_LAYER   (26 * 16 * 6)    // 400x240 resolution -> 25x15 tiles onscreen per layer at once
#define MAX_SPRITES_PER_LAYER (200)             // just guessing here lol

#define TILE_MAXSIZE 6    	// also just guessing here lol

typedef struct {
	Tex3DS_SubTexture subtex;
	C2D_DrawParams params;
} _3ds_tile;

typedef struct {
	ushort x;
	ushort y;
	ushort w;
	ushort h;
	u32 color;
} _3ds_rectangle;

typedef struct {
	int sid;

	float xscale;
	float yscale;

	float angle;

	C3D_Tex* tex;
	Tex3DS_SubTexture subtex;
	C2D_DrawParams params;

	byte isTinted;
	C2D_ImageTint tint;

	byte isRect;
	_3ds_rectangle rect;
} _3ds_sprite;

extern int spriteIndex[7];
extern int tileIndex[4];

extern byte paletteIndex;
extern byte cachedPalettes;
extern byte maxPaletteCycles;

extern u32 clearColor;
extern u32 fadeColor;
extern byte clearScreen;

// actual texture data
extern C3D_Tex      _3ds_textureData[SURFACE_MAX];
extern C3D_Tex      _3ds_tilesetData[TILE_MAXSIZE];

// cropped textures for sprites and tiles
extern _3ds_sprite    _3ds_sprites[7][MAX_SPRITES_PER_LAYER];
extern _3ds_tile      _3ds_tiles[4][MAX_TILES_PER_LAYER];

void _3ds_cacheSpriteSurface(int sheetID);
void _3ds_delSpriteSurface(int sheetID);
void _3ds_cacheTileSurface(byte* tilesetGfxPtr);
void _3ds_delTileSurface();
void _3ds_cacheGfxSurface(byte* gfxDataPtr, C3D_Tex* dst,
			  int width, int height, bool write);

inline void SWTilePosToHWTilePos(int sx, int sy, int* dx, int* dy) {
	*dx = sx + (16 * (sy / 512));
	*dy = sy % 512;
}

inline void _3ds_prepSprite(int XPos, int YPos, int width, int height, 
		     int sprX, int sprY, int sheetID, int direction,
		     float scaleX, float scaleY, float angle, int alpha, 
		     int layer) {
    	// we don't actually draw the sprite immediately, we only 
    	// set up a sprite to be drawn next C2D_SceneBegin
    	if (spriteIndex[layer] < MAX_SPRITES_PER_LAYER) {
		_3ds_sprite spr;

		// set up reference to texture
		spr.sid = sheetID;

		// set up subtexture
		spr.subtex.width  = gfxSurface[sheetID].width;
		spr.subtex.height = gfxSurface[sheetID].height;
		spr.subtex.left   = (float) sprX                / _3ds_textureData[sheetID].width;
		spr.subtex.top    = 1 - (float) sprY            / _3ds_textureData[sheetID].height;
		spr.subtex.right  = (float) (sprX + width)      / _3ds_textureData[sheetID].width;
		spr.subtex.bottom = 1 - (float) (sprY + height) / _3ds_textureData[sheetID].height;

		// set up draw params
		spr.params.pos.x = XPos;
		spr.params.pos.y = YPos;
		switch (direction) {
			case FLIP_X:
				spr.params.pos.w = -width * scaleX;
				spr.params.pos.h = height * scaleY;
				break;
			case FLIP_Y:
				spr.params.pos.w = width   * scaleX;
				spr.params.pos.h = -height * scaleY;
				break;
			case FLIP_XY:
				spr.params.pos.w = -width  * scaleX;
				spr.params.pos.h = -height * scaleY;
				break;
			default:
				spr.params.pos.w = width  * scaleX;
				spr.params.pos.h = height * scaleY;
				break;
		}

		if (angle) {
			spr.params.center.x = (spr.subtex.right - spr.subtex.left)   / 2;
			spr.params.center.y = (spr.subtex.top   - spr.subtex.bottom) / 2;
		} else {
			spr.params.center.x = 0.0f;
			spr.params.center.y = 0.0f;
		}

		spr.params.depth = 0;
		spr.params.angle = angle;

		spr.isRect = 0;

		C2D_AlphaImageTint(&spr.tint, alpha / 255.0f);

		_3ds_sprites[layer][spriteIndex[layer]] = spr;
                spriteIndex[layer]++;
    	}
}

inline void _3ds_prepTile(int XPos, int YPos, int dataPos, int direction, int layer) {
	const int tileSize = 16;

	// the original gif for tilesets is only 16x16384
	// any y coordinates beyond this are invalid
	if (dataPos > 262144) {
		printf("Invalid position: %d\n", dataPos);
		return;
	}

	if (tileIndex[layer] >= MAX_TILES_PER_LAYER) {
		printf("Tile limit hit!\n");
	}

	int tileX;
	int tileY;

	if (tileIndex[layer] < MAX_TILES_PER_LAYER) {
		_3ds_tile tile;

		//int ty = tileY;

		// convert coordinates to work with the 3DS tile texture data
		SWTilePosToHWTilePos(dataPos % 16, dataPos / 16, &tileX, &tileY);

		tile.subtex.width  = 512;
		tile.subtex.height = 512;
		tile.subtex.left = (float) tileX / _3ds_tilesetData[paletteIndex].width;
		tile.subtex.top = 1 - (float) tileY / _3ds_tilesetData[paletteIndex].height;
		tile.subtex.right = (float) (tileX + tileSize) / _3ds_tilesetData[paletteIndex].width;
		tile.subtex.bottom = 1 - (float) (tileY + tileSize) / _3ds_tilesetData[paletteIndex].height;

		tile.params.pos.x = XPos;
		tile.params.pos.y = YPos;

		switch (direction) {
			case FLIP_X:
				tile.params.pos.w = -tileSize;
				tile.params.pos.h = tileSize;
				break;
			case FLIP_Y:
				tile.params.pos.w = tileSize;
				tile.params.pos.h = -tileSize;
				break;
			case FLIP_XY:
				tile.params.pos.w = -tileSize;
				tile.params.pos.h = -tileSize;
				break;
			default:
				tile.params.pos.w = tileSize;
				tile.params.pos.h = tileSize;
				break;
		}


		tile.params.center.x = 0;
		tile.params.center.y = 0;

		tile.params.depth = 0;
		tile.params.angle = 0;

		_3ds_tiles[layer][tileIndex[layer]] = tile;
		tileIndex[layer]++;
	}
}

inline void _3ds_prepRect(int x, int y, int w, int h, int R, int G, int B, int A, int layer) {
	if (layer >= 8) {
		printf("Invalid layer: %d\n", layer);
		return;
	}

	if (spriteIndex[layer] < MAX_SPRITES_PER_LAYER) {
		_3ds_sprite spr;

		spr.isRect = 1;
		spr.rect.x = x;
		spr.rect.y = y;
		spr.rect.w = w;
		spr.rect.h = h;
		spr.rect.color = C2D_Color32(R, G, B, A);

		_3ds_sprites[layer][spriteIndex[layer]] = spr;
		spriteIndex[layer]++;
	}
}
#endif
