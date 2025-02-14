#include "RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
#include <Windows.h>
#include <codecvt>
#include "../dependencies/windows/ValveFileVDF/vdf_parser.hpp"

HKEY hKey;

LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue)
{
    nValue = nDefaultValue;
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nResult(0);
    LONG nError = ::RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
    if (ERROR_SUCCESS == nError) {
        nValue = nResult;
    }
    return nError;
}

LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
{
    strValue = strDefaultValue;
    WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError) {
        strValue = szBuffer;
    }
    return nError;
}

inline std::string utf16ToUtf8(const std::wstring &utf16Str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(utf16Str);
}

inline bool dirExists(const std::wstring &dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false; // something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true; // this is a directory!

    return false; // this is not a directory!
}
#endif

int globalVariablesCount;
int globalVariables[GLOBALVAR_COUNT];
char globalVariableNames[GLOBALVAR_COUNT][0x20];

char gamePath[0x100];
char modsPath[0x100];
int saveRAM[SAVEDATA_MAX];
Achievement achievements[ACHIEVEMENT_MAX];
LeaderboardEntry leaderboard[LEADERBOARD_MAX];

#if RETRO_PLATFORM == RETRO_OSX
#include <sys/stat.h>
#include <sys/types.h>
#endif

int controlMode = -1;
bool disableTouchControls = false;

#if RETRO_USE_MOD_LOADER
std::vector<ModInfo> modList;
bool forceUseScripts = false;
#endif

void InitUserdata()
{
    // userdata files are loaded from this directory
    sprintf(gamePath, "%s", BASE_PATH);
    sprintf(modsPath, "%s", BASE_PATH);
    
#if RETRO_PLATFORM == RETRO_OSX
    sprintf(gamePath, "%s/RSDKv3", getResourcesPath());
    sprintf(modsPath, "%s/RSDKv3/", getResourcesPath());
    
    mkdir(gamePath, 0777);
#elif RETRO_PLATFORM == RETRO_ANDROID
    {
        char buffer[0x200];

        JNIEnv *env      = (JNIEnv *)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        jclass cls(env->GetObjectClass(activity));
        jmethodID method = env->GetMethodID(cls, "getBasePath", "()Ljava/lang/String;");
        auto ret         = env->CallObjectMethod(activity, method);

        strcpy(buffer, env->GetStringUTFChars((jstring)ret, NULL));

        sprintf(gamePath, "%s", buffer);
        sprintf(modsPath, "%s", buffer);

        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(cls);
    }
#endif

    char buffer[0x200];
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/settings.ini", getResourcesPath());
    else
        sprintf(buffer, "%ssettings.ini", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_ANDROID
    sprintf(buffer, "%s/settings.ini", gamePath);
#elif RETRO_PLATFORM == RETRO_iOS
    sprintf(buffer, "%s/settings.ini", getDocumentsPath());
#else
    sprintf(buffer, BASE_PATH"settings.ini");
#endif
    FileIO *file = fOpen(buffer, "rb");
    IniParser* ini;
    if (!file) {
	ini = new IniParser();
        ini->SetBool("Dev", "DevMenu", Engine.devMenu = false);
        ini->SetBool("Dev", "EngineDebugMode", engineDebugMode = false);
        ini->SetInteger("Dev", "StartingCategory", Engine.startList = 0);
        ini->SetInteger("Dev", "StartingScene", Engine.startStage = 0);
        ini->SetInteger("Dev", "FastForwardSpeed", Engine.fastForwardSpeed = 8);
        ini->SetBool("Dev", "UseSteamDir", Engine.useSteamDir = true);
        ini->SetBool("Dev", "UseHQModes", Engine.useHQModes = true);

        ini->SetInteger("Game", "Language", Engine.language = RETRO_EN);
        ini->SetInteger("Game", "OriginalControls", -1);

        ini->SetBool("Window", "FullScreen", Engine.startFullScreen = DEFAULT_FULLSCREEN);
        ini->SetBool("Window", "Borderless", Engine.borderless = false);
        ini->SetBool("Window", "VSync", Engine.vsync = false);
        ini->SetInteger("Window", "WindowScale", Engine.windowScale = 2);
        ini->SetInteger("Window", "ScreenWidth", SCREEN_XSIZE = DEFAULT_SCREEN_XSIZE);
        ini->SetInteger("Window", "RefreshRate", Engine.refreshRate = 60);

        ini->SetFloat("Audio", "BGMVolume", bgmVolume / (float)MAX_VOLUME);
        ini->SetFloat("Audio", "SFXVolume", sfxVolume / (float)MAX_VOLUME);

#if RETRO_USING_SDL
        ini->SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_Scancode)");
        ini->SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings = SDL_SCANCODE_UP);
        ini->SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings = SDL_SCANCODE_DOWN);
        ini->SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings = SDL_SCANCODE_LEFT);
        ini->SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings = SDL_SCANCODE_RIGHT);
        ini->SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings = SDL_SCANCODE_Z);
        ini->SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings = SDL_SCANCODE_X);
        ini->SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings = SDL_SCANCODE_C);
        ini->SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings = SDL_SCANCODE_RETURN);

        ini->SetComment("Controller 1", "IC1Comment", "Controller Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_GameControllerButton)");
        ini->SetInteger("Controller 1", "Up", inputDevice[0].contMappings = SDL_CONTROLLER_BUTTON_DPAD_UP);
        ini->SetInteger("Controller 1", "Down", inputDevice[1].contMappings = SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        ini->SetInteger("Controller 1", "Left", inputDevice[2].contMappings = SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        ini->SetInteger("Controller 1", "Right", inputDevice[3].contMappings = SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        ini->SetInteger("Controller 1", "A", inputDevice[4].contMappings = SDL_CONTROLLER_BUTTON_A);
        ini->SetInteger("Controller 1", "B", inputDevice[5].contMappings = SDL_CONTROLLER_BUTTON_B);
        ini->SetInteger("Controller 1", "C", inputDevice[6].contMappings = SDL_CONTROLLER_BUTTON_X);
        ini->SetInteger("Controller 1", "Start", inputDevice[7].contMappings = SDL_CONTROLLER_BUTTON_START);

        ini->SetFloat("Controller 1", "LStickDeadzone", LSTICK_DEADZONE = 0.3);
        ini->SetFloat("Controller 1", "RStickDeadzone", RSTICK_DEADZONE = 0.3);
        ini->SetFloat("Controller 1", "LTriggerDeadzone", LTRIGGER_DEADZONE = 0.3);
        ini->SetFloat("Controller 1", "RTriggerDeadzone", RTRIGGER_DEADZONE = 0.3);
        #endif

        ini->Write(BASE_PATH"settings.ini");
    }
    else {
        fClose(file);
        ini = new IniParser(buffer);
        if (!ini->GetBool("Dev", "DevMenu", &Engine.devMenu))
            Engine.devMenu = false;
        if (!ini->GetBool("Dev", "EngineDebugMode", &engineDebugMode))
            engineDebugMode = false;
        if (!ini->GetInteger("Dev", "StartingCategory", &Engine.startList))
            Engine.startList = 0;
        if (!ini->GetInteger("Dev", "StartingScene", &Engine.startStage))
            Engine.startStage = 0;
        if (!ini->GetInteger("Dev", "FastForwardSpeed", &Engine.fastForwardSpeed))
            Engine.fastForwardSpeed = 8;
        if (!ini->GetBool("Dev", "UseSteamDir", &Engine.useSteamDir))
            Engine.useSteamDir = true;
        if (!ini->GetBool("Dev", "UseHQModes", &Engine.useHQModes))
            Engine.useHQModes = true;


        if (!ini->GetString("Dev", "DataFile", Engine.dataFile))
            StrCopy(Engine.dataFile, "Data.rsdk");

        if (!ini->GetInteger("Game", "Language", &Engine.language))
            Engine.language = RETRO_EN;
        
        if (!ini->GetInteger("Game", "OriginalControls", &controlMode))
            controlMode = -1;
        if (!ini->GetBool("Game", "DisableTouchControls", &disableTouchControls))
            disableTouchControls = false;

        #if RETRO_USING_SDL
        if (!ini->GetBool("Window", "FullScreen", &Engine.startFullScreen))
            Engine.startFullScreen = DEFAULT_FULLSCREEN;
        if (!ini->GetBool("Window", "Borderless", &Engine.borderless))
            Engine.borderless = false;
        if (!ini->GetBool("Window", "VSync", &Engine.vsync))
            Engine.vsync = false;
        if (!ini->GetBool("Window", "EnhancedScaling", &Engine.enhancedScaling))
            Engine.enhancedScaling = true;
        if (!ini->GetInteger("Window", "WindowScale", &Engine.windowScale))
            Engine.windowScale = 2;
        if (!ini->GetInteger("Window", "ScreenWidth", &SCREEN_XSIZE))
            SCREEN_XSIZE = DEFAULT_SCREEN_XSIZE;
        if (!ini->GetInteger("Window", "RefreshRate", &Engine.refreshRate))
            Engine.refreshRate = 60;
        if (!ini->GetInteger("Window", "DimLimit", &Engine.dimLimit))
            Engine.dimLimit = 300; //5 mins
        if (Engine.dimLimit >= 0)
            Engine.dimLimit *= Engine.refreshRate;
#endif

        float bv = 0, sv = 0;
        if (!ini->GetFloat("Audio", "BGMVolume", &bv))
            bv = 1.0f;
        if (!ini->GetFloat("Audio", "SFXVolume", &sv))
            sv = 1.0f;

        bgmVolume = bv * MAX_VOLUME;
        sfxVolume = sv * MAX_VOLUME;

        if (bgmVolume > MAX_VOLUME)
            bgmVolume = MAX_VOLUME;
        if (bgmVolume < 0)
            bgmVolume = 0;

        if (sfxVolume > MAX_VOLUME)
            sfxVolume = MAX_VOLUME;
        if (sfxVolume < 0)
            sfxVolume = 0;

#if RETRO_USING_SDL2
        if (!ini->GetInteger("Keyboard 1", "Up", &inputDevice[INPUT_UP].keyMappings))
            inputDevice[0].keyMappings = SDL_SCANCODE_UP;
        if (!ini->GetInteger("Keyboard 1", "Down", &inputDevice[INPUT_DOWN].keyMappings))
            inputDevice[1].keyMappings = SDL_SCANCODE_DOWN;
        if (!ini->GetInteger("Keyboard 1", "Left", &inputDevice[INPUT_LEFT].keyMappings))
            inputDevice[2].keyMappings = SDL_SCANCODE_LEFT;
        if (!ini->GetInteger("Keyboard 1", "Right", &inputDevice[INPUT_RIGHT].keyMappings))
            inputDevice[3].keyMappings = SDL_SCANCODE_RIGHT;
        if (!ini->GetInteger("Keyboard 1", "A", &inputDevice[INPUT_BUTTONA].keyMappings))
            inputDevice[4].keyMappings = SDL_SCANCODE_Z;
        if (!ini->GetInteger("Keyboard 1", "B", &inputDevice[INPUT_BUTTONB].keyMappings))
            inputDevice[5].keyMappings = SDL_SCANCODE_X;
        if (!ini->GetInteger("Keyboard 1", "C", &inputDevice[INPUT_BUTTONC].keyMappings))
            inputDevice[6].keyMappings = SDL_SCANCODE_C;
        if (!ini->GetInteger("Keyboard 1", "Start", &inputDevice[INPUT_START].keyMappings))
            inputDevice[7].keyMappings = SDL_SCANCODE_RETURN;

        if (!ini->GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings))
            inputDevice[0].contMappings = SDL_CONTROLLER_BUTTON_DPAD_UP;
        if (!ini->GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings))
            inputDevice[1].contMappings = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        if (!ini->GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings))
            inputDevice[2].contMappings = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        if (!ini->GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings))
            inputDevice[3].contMappings = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        if (!ini->GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings))
            inputDevice[4].contMappings = SDL_CONTROLLER_BUTTON_A;
        if (!ini->GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings))
            inputDevice[5].contMappings = SDL_CONTROLLER_BUTTON_B;
        if (!ini->GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings))
            inputDevice[6].contMappings = SDL_CONTROLLER_BUTTON_X;
        if (!ini->GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings))
            inputDevice[7].contMappings = SDL_CONTROLLER_BUTTON_START;

	/*
        if (!ini->GetFloat("Controller 1", "LStickDeadzone", &LSTICK_DEADZONE))
            LSTICK_DEADZONE = 0.3;
        if (!ini->GetFloat("Controller 1", "RStickDeadzone", &RSTICK_DEADZONE))
            RSTICK_DEADZONE = 0.3;
        if (!ini->GetFloat("Controller 1", "LTriggerDeadzone", &LTRIGGER_DEADZONE))
            LTRIGGER_DEADZONE = 0.3;
        if (!ini->GetFloat("Controller 1", "RTriggerDeadzone", &RTRIGGER_DEADZONE))
            RTRIGGER_DEADZONE = 0.3;
	    */
#endif

#if RETRO_USING_SDL1
        if (!ini->GetInteger("Keyboard 1", "Up", &inputDevice[INPUT_UP].keyMappings))
            inputDevice[0].keyMappings = SDLK_UP;
        if (!ini->GetInteger("Keyboard 1", "Down", &inputDevice[INPUT_DOWN].keyMappings))
            inputDevice[1].keyMappings = SDLK_DOWN;
        if (!ini->GetInteger("Keyboard 1", "Left", &inputDevice[INPUT_LEFT].keyMappings))
            inputDevice[2].keyMappings = SDLK_LEFT;
        if (!ini->GetInteger("Keyboard 1", "Right", &inputDevice[INPUT_RIGHT].keyMappings))
            inputDevice[3].keyMappings = SDLK_RIGHT;
        if (!ini->GetInteger("Keyboard 1", "A", &inputDevice[INPUT_BUTTONA].keyMappings))
            inputDevice[4].keyMappings = SDLK_z;
        if (!ini->GetInteger("Keyboard 1", "B", &inputDevice[INPUT_BUTTONB].keyMappings))
            inputDevice[5].keyMappings = SDLK_x;
        if (!ini->GetInteger("Keyboard 1", "C", &inputDevice[INPUT_BUTTONC].keyMappings))
            inputDevice[6].keyMappings = SDLK_c;
        if (!ini->GetInteger("Keyboard 1", "Start", &inputDevice[INPUT_START].keyMappings))
            inputDevice[7].keyMappings = SDLK_RETURN;

        if (!ini->GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings))
            inputDevice[0].contMappings = 1;
        if (!ini->GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings))
            inputDevice[1].contMappings = 2;
        if (!ini->GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings))
            inputDevice[2].contMappings = 3;
        if (!ini->GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings))
            inputDevice[3].contMappings = 4;
        if (!ini->GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings))
            inputDevice[4].contMappings = 5;
        if (!ini->GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings))
            inputDevice[5].contMappings = 6;
        if (!ini->GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings))
            inputDevice[6].contMappings = 7;
        if (!ini->GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings))
            inputDevice[7].contMappings = 8;

        if (!ini.GetFloat("Controller 1", "LStickDeadzone", &LSTICK_DEADZONE))
            LSTICK_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "RStickDeadzone", &RSTICK_DEADZONE))
            RSTICK_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "LTriggerDeadzone", &LTRIGGER_DEADZONE))
            LTRIGGER_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "RTriggerDeadzone", &RTRIGGER_DEADZONE))
            RTRIGGER_DEADZONE = 0.3;
#endif
    }

    delete ini;
    SetScreenSize(SCREEN_XSIZE, SCREEN_YSIZE);

    // Support for extra controller types SDL doesn't recognise
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/controllerdb.txt", getResourcesPath());
    else
        sprintf(buffer, "%scontrollerdb.txt", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_ANDROID
    sprintf(buffer, "%s/controllerdb.txt", gamePath);
#else
    sprintf(buffer, BASE_PATH "controllerdb.txt");
#endif

#if RETRO_USING_SDL2
    file = fOpen(buffer, "rb");
    if (file) {
        fClose(file);

	#if RETRO_USING_SDL
        int nummaps = SDL_GameControllerAddMappingsFromFile(buffer);
        if (nummaps >= 0)
            printLog("loaded %d controller mappings from '%s'\n", buffer, nummaps);
        #endif
    }
#endif

#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/Udata.bin", getResourcesPath());
    else
        sprintf(buffer, "%sUdata.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_ANDROID
    sprintf(buffer, "%s/UData.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_iOS
    sprintf(buffer, "%s/UData.bin", getDocumentsPath());
#else
    sprintf(buffer, "%sUdata.bin", gamePath);
#endif
    file = fOpen(buffer, "rb");
    if (file) {
        fClose(file);
        ReadUserdata();
    }
    else {
        WriteUserdata();
    }

    StrCopy(achievements[0].name, "88 Miles Per Hour");
    StrCopy(achievements[1].name, "Just One Hug is Enough");
    StrCopy(achievements[2].name, "Paradise Found");
    StrCopy(achievements[3].name, "Take the High Road");
    StrCopy(achievements[4].name, "King of the Rings");
    StrCopy(achievements[5].name, "Statue Saviour");
    StrCopy(achievements[6].name, "Heavy Metal");
    StrCopy(achievements[7].name, "All Stages Clear");
    StrCopy(achievements[8].name, "Treasure Hunter");
    StrCopy(achievements[9].name, "Dr Eggman Got Served");
    StrCopy(achievements[10].name, "Just In Time");
    StrCopy(achievements[11].name, "Saviour of the Planet");

    // Loaded here so it can be disabled
#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
    if (Engine.useSteamDir) {
#if _WIN64
        LONG lRes             = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Wow6432Node\\Valve\\Steam", 0, KEY_READ, &hKey);
        bool existsAndSuccess = lRes == ERROR_SUCCESS;
        std::wstring steamPath;

        if (existsAndSuccess) {
            GetStringRegKey(hKey, L"InstallPath", steamPath, L"");

            std::ifstream file(steamPath + L"/config/loginusers.vdf");
            auto root = tyti::vdf::read(file);

            std::vector<long long> SIDs;
            for (auto &child : root.childs) {
                long long sidVal = std::stoll(child.first);
                SIDs.push_back(sidVal & 0xFFFFFFFF);
            }

            for (auto &sid : SIDs) {
                std::wstring udataPath = steamPath.c_str() + std::wstring(L"/userdata/") + std::to_wstring(sid) + std::wstring(L"/200940/local/");

                if (dirExists(udataPath)) {
                    sprintf(gamePath, "%s", utf16ToUtf8(udataPath).c_str());
                    break;
                }
            }
        }

#elif _WIN32
        LONG lRes             = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", 0, KEY_READ, &hKey);
        bool existsAndSuccess = lRes == ERROR_SUCCESS;
        std::wstring steamPath;

        if (existsAndSuccess) {
            GetStringRegKey(hKey, L"InstallPath", steamPath, L"");

            std::ifstream file(steamPath + L"/config/loginusers.vdf");
            auto root = tyti::vdf::read(file);

            std::vector<long long> SIDs;
            for (auto &child : root.childs) {
                long long sidVal = std::stoll(child.first);
                SIDs.push_back(sidVal & 0xFFFFFFFF);
            }

            for (auto &sid : SIDs) {
                std::wstring udataPath = steamPath.c_str() + std::wstring(L"/userdata/") + std::to_wstring(sid) + std::wstring(L"/200940/local/");

                if (dirExists(udataPath)) {
                    sprintf(gamePath, "%s", utf16ToUtf8(udataPath).c_str());
                    break;
                }
            }
        }
#endif
    }
#endif

#if RETRO_PLATFORM == RETRO_3DS
    Engine.useHQModes = false; //fixes broken special stage floors
#endif
}

void writeSettings() {
    IniParser *ini = new IniParser();

    ini->SetComment("Dev", "DevMenuComment", "Enable this flag to activate dev menu via the ESC key");
    ini->SetBool("Dev", "DevMenu", Engine.devMenu);
    ini->SetComment("Dev", "DebugModeComment", "Enable this flag to activate features used for debugging the engine (may result in slightly slower game speed)");
    ini->SetBool("Dev", "EngineDebugMode", engineDebugMode);
    ini->SetComment("Dev", "SCComment", "Sets the starting category ID");
    ini->SetInteger("Dev", "StartingCategory", Engine.startList);
    ini->SetComment("Dev", "SSComment", "Sets the starting scene ID");
    ini->SetInteger("Dev", "StartingScene", Engine.startStage);
    ini->SetComment("Dev", "FFComment", "Determines how fast the game will be when fastforwarding is active");
    ini->SetInteger("Dev", "FastForwardSpeed", Engine.fastForwardSpeed);
    ini->SetComment("Dev", "SDComment", "Determines if the game will try to use the steam directory for the game if it can locate it (windows only)");
    ini->SetBool("Dev", "UseSteamDir", Engine.useSteamDir);
    ini->SetComment("Dev", "UseHQComment","Determines if applicable rendering modes (such as 3D floor from special stages) will render in \"High Quality\" mode or standard mode");
    ini->SetBool("Dev", "UseHQModes", Engine.useHQModes);

    ini->SetComment("Game", "LangComment", "Sets the game language (0 = EN, 1 = FR, 2 = IT, 3 = DE, 4 = ES, 5 = JP)");
    ini->SetInteger("Game", "Language", Engine.language);
    ini->SetComment("Game", "OGCtrlComment", "Sets the game's spindash style (-1 = let save file decide, 0 = S2, 1 = CD)");
    ini->SetInteger("Game", "OriginalControls", controlMode);

    ini->SetComment("Window", "FSComment", "Determines if the window will be fullscreen or not");
    ini->SetBool("Window", "FullScreen", Engine.startFullScreen);
    ini->SetComment("Window", "BLComment", "Determines if the window will be borderless or not");
    ini->SetBool("Window", "Borderless", Engine.borderless);
    ini->SetComment("Window", "VSComment", "Determines if VSync will be active or not");
    ini->SetBool("Window", "VSync", Engine.vsync);
    ini->SetComment("Window", "ESComment", "Determines if Enhanced Scaling will be active or not. Only affects non-multiple resolutions.");
    ini->SetComment("Window", "WSComment", "How big the window will be");
    ini->SetInteger("Window", "WindowScale", Engine.windowScale);
    ini->SetComment("Window", "SWComment", "How wide the base screen will be in pixels");
    ini->SetInteger("Window", "ScreenWidth", SCREEN_XSIZE);
    ini->SetComment("Window", "RRComment", "Determines the target FPS");
    ini->SetInteger("Window", "RefreshRate", Engine.refreshRate);

    ini->SetFloat("Audio", "BGMVolume", bgmVolume / (float)MAX_VOLUME);
    ini->SetFloat("Audio", "SFXVolume", sfxVolume / (float)MAX_VOLUME);
#if RETRO_USING_SDL2
    ini->SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_Scancode)");
#endif
#if RETRO_USING_SDL1
    ini->SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDLKeycodeLookup)");
#endif
    ini->SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_Scancode)");
    ini->SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings);
    ini->SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings);
    ini->SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings);
    ini->SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings);
    ini->SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings);
    ini->SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings);
    ini->SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings);
    ini->SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings);

#if RETRO_USING_SDL2
    ini->SetComment("Controller 1", "IC1Comment", "Controller Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_GameControllerButton)");
    ini->SetComment("Controller 1", "IC1Comment2", "Extra buttons can be mapped with the following IDs:");
    ini->SetComment("Controller 1", "IC1Comment3", "CONTROLLER_BUTTON_ZL             = 16");
    ini->SetComment("Controller 1", "IC1Comment4", "CONTROLLER_BUTTON_ZR             = 17");
    ini->SetComment("Controller 1", "IC1Comment5", "CONTROLLER_BUTTON_LSTICK_UP      = 18");
    ini->SetComment("Controller 1", "IC1Comment6", "CONTROLLER_BUTTON_LSTICK_DOWN    = 19");
    ini->SetComment("Controller 1", "IC1Comment7", "CONTROLLER_BUTTON_LSTICK_LEFT    = 20");
    ini->SetComment("Controller 1", "IC1Comment8", "CONTROLLER_BUTTON_LSTICK_RIGHT   = 21");
    ini->SetComment("Controller 1", "IC1Comment9", "CONTROLLER_BUTTON_RSTICK_UP      = 22");
    ini->SetComment("Controller 1", "IC1Comment10", "CONTROLLER_BUTTON_RSTICK_DOWN    = 23");
    ini->SetComment("Controller 1", "IC1Comment11", "CONTROLLER_BUTTON_RSTICK_LEFT    = 24");
    ini->SetComment("Controller 1", "IC1Comment12", "CONTROLLER_BUTTON_RSTICK_RIGHT   = 25");
#endif
    ini->SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings);
    ini->SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings);
    ini->SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings);
    ini->SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings);
    ini->SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings);
    ini->SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings);
    ini->SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings);
    ini->SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings);

    ini->Write(BASE_PATH"settings.ini");
    delete ini;
}

void ReadUserdata()
{
    char buffer[0x200];
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/Udata.bin", getResourcesPath());
    else
        sprintf(buffer, "%sUdata.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX
    sprintf(buffer, "%s/UData.bin", gamePath);
#else
    sprintf(buffer, "%sUdata.bin", gamePath);
#endif
    FileIO *userFile = fOpen(buffer, "rb");
    if (!userFile)
        return;

    int buf = 0;
    for (int a = 0; a < ACHIEVEMENT_MAX; ++a) {
        fRead(&buf, 4, 1, userFile);
        achievements[a].status = buf;
    }
    for (int l = 0; l < LEADERBOARD_MAX; ++l) {
        fRead(&buf, 4, 1, userFile);
        leaderboard[l].status = buf;
    }

    fClose(userFile);

    if (Engine.onlineActive) {
        // Load from online
    }
}

void WriteUserdata()
{
    char buffer[0x200];
#if RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/Udata.bin", getResourcesPath());
    else
        sprintf(buffer, "%sUdata.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX
    sprintf(buffer, "%s/UData.bin", gamePath);
#else
    sprintf(buffer, "%sUdata.bin", gamePath);
#endif
    FileIO *userFile = fOpen(buffer, "wb");
    if (!userFile)
        return;

    for (int a = 0; a < ACHIEVEMENT_MAX; ++a) fWrite(&achievements[a].status, 4, 1, userFile);
    for (int l = 0; l < LEADERBOARD_MAX; ++l) fWrite(&leaderboard[l].status, 4, 1, userFile);

    fClose(userFile);

    if (Engine.onlineActive) {
        // Load from online
    }
}

void AwardAchievement(int id, int status)
{
    if (id < 0 || id >= ACHIEVEMENT_MAX)
        return;

        if (status != achievements[id].status)
            printLog("Achieved achievement: %s (%d)!", achievements[id].name, status);

    achievements[id].status = status;

    if (Engine.onlineActive) {
        // Set Achievement online
    }
    WriteUserdata();
}

void SetAchievement(int achievementID, int achievementDone)
{
    if (!Engine.trialMode && !debugMode) {
        AwardAchievement(achievementID, achievementDone);
    }
}
void SetLeaderboard(int leaderboardID, int result)
{
    if (!Engine.trialMode && !debugMode) {
        printLog("Set leaderboard (%d) value to %d", leaderboard, result);
        switch (leaderboardID) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
                leaderboard[leaderboardID].status = result;
                WriteUserdata();
                return;
        }
    }
}

#if RETRO_USE_MOD_LOADER
#include <string>
#include <filesystem>

#if RETRO_PLATFORM == RETRO_ANDROID
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

void initMods()
{
    modList.clear();
    forceUseScripts = false;

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);
    fs::path modPath(modBuf);

    if (fs::exists(modPath) && fs::is_directory(modPath)) {
        //std::string mod_config = modPath.string() + "modconfig.ini";
	sprintf(modBuf, "%smods/modconfig.ini", modsPath);
        FileIO *configFile     = fOpen(modBuf, "r");
        if (configFile) {
            fClose(configFile);
            IniParser modConfig(modBuf, false);

            for (int m = 0; m < modConfig.items.size(); ++m) {
                bool active = false;
                ModInfo info;
                modConfig.GetBool("mods", modConfig.items[m].key, &active);
                if (loadMod(&info, modPath.string(), modConfig.items[m].key, active))
                    modList.push_back(info);
            }
        }

        try {
            auto rdi = fs::directory_iterator(modPath);
            for (auto de : rdi) {
                if (de.is_directory()) {
                    fs::path modDirPath = de.path();

                    ModInfo info;

                    std::string modDir            = modDirPath.string().c_str();
                    const std::string mod_inifile = modDir + "/mod.ini";
                    std::string folder            = modDirPath.filename().string();

                    bool flag = true;
                    for (int m = 0; m < modList.size(); ++m) {
                        if (modList[m].folder == folder) {
                            flag = false;
                            break;
                        }
                    }

                    if (flag) {
                        if (loadMod(&info, modPath.string(), modDirPath.filename().string(), false))
                            modList.push_back(info);
                    }
                }
            }
        } catch (fs::filesystem_error fe) {
            printLog("Mods Folder Scanning Error: ");
            printLog(fe.what());
        }
    }
}

bool loadMod(ModInfo *info, std::string modsPath, std::string folder, bool active)
{
    if (!info)
        return false;

    info->fileMap.clear();
    info->name    = "";
    info->desc    = "";
    info->author  = "";
    info->version = "";
    info->folder  = "";
    info->active  = false;

    const std::string modDir = modsPath + "/" + folder;

    FileIO *f = fOpen((modDir + "/mod.ini").c_str(), "r");
    if (f) {
        fClose(f);
        IniParser modSettings((modDir + "/mod.ini").c_str(), false);

        info->name    = "Unnamed Mod";
        info->desc    = "";
        info->author  = "Unknown Author";
        info->version = "1.0.0";
        info->folder  = folder;

        char infoBuf[0x100];
        // Name
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Name", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->name = infoBuf;
        // Desc
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Description", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->desc = infoBuf;
        // Author
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Author", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->author = infoBuf;
        // Version
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Version", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->version = infoBuf;

        info->active = active;

        // Check for Data/ replacements
        fs::path dataPath(modDir + "/Data");

        if (fs::exists(dataPath) && fs::is_directory(dataPath)) {
            try {
                auto data_rdi = fs::recursive_directory_iterator(dataPath);
                for (auto data_de : data_rdi) {
                    if (data_de.is_regular_file()) {
                        char modBuf[0x100];
                        StrCopy(modBuf, data_de.path().string().c_str());
                        char folderTest[4][0x10] = {
                            "Data/",
                            "Data\\",
                            "data/",
                            "data\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x80];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // printLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                }
            } catch (fs::filesystem_error fe) {
                printLog("Data Folder Scanning Error: ");
                printLog(fe.what());
            }
        }

        // Check for Scripts/ replacements
        fs::path scriptPath(modDir + "/Scripts");

        if (fs::exists(scriptPath) && fs::is_directory(scriptPath)) {
            try {
                auto data_rdi = fs::recursive_directory_iterator(scriptPath);
                for (auto data_de : data_rdi) {
                    if (data_de.is_regular_file()) {
                        char modBuf[0x100];
                        StrCopy(modBuf, data_de.path().string().c_str());
                        char folderTest[4][0x10] = {
                            "Scripts/",
                            "Scripts\\",
                            "scripts/",
                            "scripts\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x80];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // printLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                }
            } catch (fs::filesystem_error fe) {
                printLog("Script Folder Scanning Error: ");
                printLog(fe.what());
            }
        }

        // Check for Bytecode/ replacements
        fs::path bytecodePath(modDir + "/Videos");

        if (fs::exists(bytecodePath) && fs::is_directory(bytecodePath)) {
            try {
                auto data_rdi = fs::recursive_directory_iterator(bytecodePath);
                for (auto data_de : data_rdi) {
                    if (data_de.is_regular_file()) {
                        char modBuf[0x100];
                        StrCopy(modBuf, data_de.path().string().c_str());
                        char folderTest[4][0x10] = {
                            "Videos/",
                            "Videos\\",
                            "videos/",
                            "videos\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x80];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // printLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                }
            } catch (fs::filesystem_error fe) {
                printLog("Videos Folder Scanning Error: ");
                printLog(fe.what());
            }
        }

        info->useScripts = false;
        modSettings.GetBool("", "TxtScripts", &info->useScripts);
        if (info->useScripts && info->active)
            forceUseScripts = true;
        return true;
    }
    return false;
}
void saveMods()
{
    char modBuf[0x100];
    sprintf(modBuf, "%smods/", modsPath);
    fs::path modPath(modBuf);

    if (fs::exists(modPath) && fs::is_directory(modPath)) {
	sprintf(modBuf, "%smods/modconfig.ini", modsPath);
        //std::string mod_config = modPath.string() + "/modconfig.ini";
        IniParser modConfig;

        for (int m = 0; m < modList.size(); ++m) {
            ModInfo *info = &modList[m];

            modConfig.SetBool("mods", info->folder.c_str(), info->active);
        }

        modConfig.Write(modBuf, false);
    }
}
#endif
