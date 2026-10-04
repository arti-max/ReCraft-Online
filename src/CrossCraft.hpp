#pragma once
#include "gamemode/CreativeGameMode.hpp"
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten.h>
#endif
#include <GLFW/glfw3.h>
#include <string>
#include <array>
#include "util/Mouse.hpp"
#include "util/Keyboard.hpp"
#include "level/Level.hpp"
#include "level/tile/Tile.hpp"
#include "level/render/LevelRenderer.hpp"
#include "player/Player.hpp"
#include "Timer.hpp"
#include "render/Textures.hpp"
#include "HitResult.hpp"
#include "gui/Font.hpp"
#include "util/Ray.hpp"
#include "Entity.hpp"
#include "gui/Screen.hpp"
#include "gui/pause/PauseScreen.hpp"
#include "gui/ingame/ChatScreen.hpp"
#include "gui/ingame/ChatGui.hpp"
#include "gui/ingame/PlayerListScreen.hpp"
#include "gui/ingame/Hud.hpp"
#include "gui/ingame/BlockSelectScreen.hpp"
#include "gui/ingame/DeathScreen.hpp"
#include "gui/ingame/InventoryScreen.hpp"
#include "User.hpp"
#include "util/Logger.hpp"
#include "level/LevelLoaderListener.hpp"
#include "level/levelgen/LevelGen.hpp"
#include "level/LevelIO.hpp"
#include "particle/ParticleEngine.hpp"
#include "net/Client.hpp"
#include "net/NetworkPlayer.hpp"
#include "render/fx/TextureFX.hpp"
#include "render/fx/WaterTextureFX.hpp"
#include "render/fx/LavaTextureFX.hpp"
#include "item/Item.hpp"
#include "sound/SoundManager.hpp"
#include "sound/SoundType.hpp"
#include "mob/HumanMob.hpp"
#include "mob/Zombie.hpp"
#include "mob/Skeleton.hpp"
#include "Settings.hpp"
#include "Data.hpp"
#include "gamemode/GameMode.hpp"
#include "gamemode/SurvivalGameMode.hpp"
#include "render/HeldBlock.hpp"
#include "item/Sign.hpp"
#include "item/Arrow.hpp"
#include "Progressbar.hpp"
#include "particle/WaterDropPatricle.hpp"
#include "net/NetworkData.hpp"

class CrossCraft : public LevelLoaderListener {
public:
    const std::string VERSION_STRING = "0.18  SURVIVAL TEST";
    std::string fpsString = "";
private:
    int lastFpsTime = 0;
    int frames = 0;
    int lastProgress = -1;
    std::string title = "";
    std::string status = "";
    bool inErrorState = false;
    std::string errorTitle = "";
    std::string errorReason = "";
    std::vector<std::string> connectionUrls;
    int currentUrlIndex = 0;
    
    std::array<float, 4> fogColor0;
    std::array<float, 4> fogColor1;
    std::array<int, 16> viewportBuffer;
    std::array<int, 2000> selectBuffer;
    std::vector<TextureFX*> textureEffects;
    bool fullscreen;
    bool running = false;
    bool paused = false;
    std::string parent;
    int editMode = 0;
    int selectedTile = Tile::rock->id;
    bool mouseGrabbed = false;
    float fogDistance = 0.0f;
    int attackTime = 0;
    int clickDelay = 0;
    bool isDrop = false;
    bool raining = false;

    float lastSentX = 0, lastSentY = 0, lastSentZ = 0;
    float lastSentYaw = 0, lastSentPitch = 0;

    HitResult* hitResult;
    LevelGen* levelGen = new LevelGen(this);
    Hud* hud;
    BlockSelectScreen* blockSelectScreen;
    
    std::vector<int> hotbarSlots;
    int hotbarIndex = 0;
    int yMouseAxis = -1;

    float bgR = 0.0f;
    float bgG = 0.0f;
    float bgB = 0.0f;

    // game loop
    void init();
    void tick();
    void render(float partialTicks);
    void raycast();
    void destroy();
    void handleMouseClick(int mode);
    bool isFree(const AABB &aabb);
    void fill(int x0, int y0, int x1, int y1, int col);
    void applyBobbing(float partialTicks);
    void renderHeldBlock(float partialTicks);
    void setLighting(bool enable);
    void hurtEffect(float partialTicks);
    void renderRain(float partialTicks);

    int getSelectedTile();

    // Player camera
    void setupCamera(float partialTicks);
    void moveCameraToPlayer(float partialTicks);
    void setupFog(int layer);
    inline const float* getBuffer(float a, float b, float c, float d) {
        static float buf[4];
        buf[0] = a; buf[1] = b; buf[2] = c; buf[3] = d;
        return buf;
    }
    void drawGui(float partialTicks);

    void mainLoop();

public:
    static CrossCraft* instance;
    int width, height;
    GLFWwindow* window;

    std::vector<Screen*> screenStack;

    Timer* timer = new Timer(20.0f);
    LevelIO* levelIO = new LevelIO(this);
    Level* level = nullptr;
    Screen* screen = nullptr;
    ParticleEngine* particleEngine = nullptr;
    LevelRenderer* levelRenderer = nullptr;
    Font* font = nullptr;
    Textures* textures = nullptr;
    Player* player = nullptr;
    Client* client = nullptr;
    ChatGui* chatGui = nullptr;
    PlayerListScreen* playerListScreen = new PlayerListScreen();
    SoundManager* sound = nullptr;
    Settings* settings = new Settings();
    GameMode* gamemode = new CreativeGameMode(this);
    HeldBlock* heldBlock = new HeldBlock(this);
    Progressbar* progressbar = nullptr;
    NetworkData* netData = new NetworkData(this);

    CrossCraft(const char* parent, int width, int height, bool fullscreen);
    ~CrossCraft();

    bool appletMode = false;
    bool canRender = true;
    bool mpMode = false;
    bool waitingForFocus = false;
    Entity* hitEntity = nullptr;
    int lastClick = 0;
    int ticks = 0;

    int playerId = -1;
    int serverPort;
    std::string serverAddress;

    Data* userData = nullptr;
    int loadMapId = 0;
    std::string loadMapUser = "";
    std::string serverHost = "crosscraftweb.ddns.net";

    Vec3D* getPlayerVector(float partialTicks);
    Vec3D getPlayerVectorO(float partialTicks);

    void run();
    void stop();
    void pause();
    void resume();
    void checkGlError(const char str[]);
    void setScreen(Screen* screen);
    void grabMouse();
    void releaseMouse();
    void drawErrorScreen();

    void generateNewLevel(int width, int height, int depth);
    bool loadLevel(const char username[], int levelId);
    void saveLevel(int levelId, const char levelname[]);
    void beginLevelLoading(const char str[]) override;
    void levelLoadUpdate(const char str[]) override;
    void levelLoadProgress(int progress) override;
    void showErrorScreen(const std::string& title, const std::string& reason);

    void connectToServer();
    void handleNetworkPacket(Packet* packet);
    void connectError(std::string error);
};

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void showCrashScreen(const char* msg);
}