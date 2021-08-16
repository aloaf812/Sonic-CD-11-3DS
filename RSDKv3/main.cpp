#include "RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_3DS
void awaitInput() {
     while (aptMainLoop()) {
	hidScanInput();
	if (hidKeysDown())
		break;

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();
    }
}  
#endif

int main(int argc, char *argv[])
{
#if RETRO_PLATFORM == RETRO_3DS
#if RETRO_RENDERTYPE == RETRO_SW_RENDER
    osSetSpeedupEnable(true);
#endif

    if (!Engine.Init()) {
	    printf("Press any button to continue.\n");
	    awaitInput();
	    gfxExit();
	    return 0;
    }
    printf("Build: %s\n", COMMIT);
    printf("Audio enabled: %d\n", audioEnabled);
    printf("Engine succesfully initialised.\n");
    //awaitInput();

    
    printf("Running engine...\n");
    if (!engineDebugMode) {
        consoleClear();
        consoleSelect(NULL);
    }
    Engine.Run();

    printf("Exiting...");
    gfxExit();

#else
    for (int i = 0; i < argc; ++i) {
        if (StrComp(argv[i], "UsingCWD"))
            usingCWD = true;
    }

    Engine.Init();
    Engine.Run();
#endif


#if RETRO_RENDERTYPE == RETRO_SW_RENDER
    osSetSpeedupEnable(false);
#endif

    return 0;
}
