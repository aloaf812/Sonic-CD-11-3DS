#include "../RetroEngine.hpp"

void DebugConsoleInit() {
	consoleInit(GFX_BOTTOM, NULL);

	printf("--------------------------------\n");
	printf("|   RSDKv3 3DS Debug Console   |\n");
	printf("--------------------------------\n");

	printf("The programmer has a nap.\n");
	printf("Hold out! Programmer!\n\n");
}

// taken from https://github.com/devkitPro/3ds-examples/blob/master/graphics/gpu/2d_shapes/source/main.c#L56
void PrintStatistics() {
	printf("\x1b[10;1HCPU:     %6.2f%%\x1b[K", C3D_GetProcessingTime() * 6.0f);
	printf("\x1b[11;1HGPU:     %6.2f%%\x1b[K", C3D_GetDrawingTime() * 6.0f);
	printf("\x1b[12;1HCmdBuf:  %6.2f%%\x1b[K", C3D_GetCmdBufUsage() * 100.0f);
}
