#include "CrossCraftApplet.hpp"
#include <iostream>
#include <emscripten.h>
#include "gamemode/CreativeGameMode.hpp"
#include "gamemode/GameMode.hpp"
#include "gamemode/SurvivalGameMode.hpp"

CrossCraftApplet* CrossCraftApplet::instance = nullptr;

CrossCraftApplet* CrossCraftApplet::getInstance() {
    if (instance == nullptr) {
        std::cout << "Creating singleton CrossCraftApplet instance." << std::endl;
        instance = new CrossCraftApplet();
    }
    return instance;
}

CrossCraftApplet::CrossCraftApplet() {
    std::cout << "CrossCraftApplet singleton created" << std::endl;
    game = nullptr;
    width = 0;
    height = 0;
    isMultiplayer = false;
    this->initFS();
}

CrossCraftApplet::~CrossCraftApplet() {
    destroy();
}

void CrossCraftApplet::setParams(const std::string& user, const std::string& session, 
                               const std::string& mapUser, int mapId, int w, int h, int gm) {
    username = user;
    sessionid = session;
    loadMapUser = mapUser;
    loadMapId = mapId;
    width = w;
    height = h;
    gamemode = gm;
    
    std::cout << "Applet params set: user=" << username << ", session=" << sessionid 
              << ", size=" << width << "x" << height << std::endl;
    
    if (!username.empty() && !sessionid.empty()) {
        std::cout << "User authenticated: " << username << std::endl;

    } else {
        std::cout << "No authentication provided" << std::endl;
    }
}

void CrossCraftApplet::setServerParams(const std::string& server, int port) {
    serverAddress = server;
    serverPort = port;
    isMultiplayer = true;
    
    std::cout << "Multiplayer params set: server=" << serverAddress 
              << ", port=" << serverPort << std::endl;
}

void CrossCraftApplet::start() {
    std::cout << "=== CrossCraftApplet::start() called ===" << std::endl;
    
    if (game) {
        std::cout << "Game already running" << std::endl;
        return;
    }
    
    try {
        std::cout << "Creating CrossCraft instance..." << std::endl;
        game = new CrossCraft("#canvas", width, height, false);
        game->appletMode = true;
        if (gamemode == 1) game->gamemode = new CreativeGameMode(game);
        else game->gamemode = new SurvivalGameMode(game);
        
        if (!username.empty() && !sessionid.empty()) {
            game->userData = new Data(username, sessionid);
        }

        if (isMultiplayer && !serverAddress.empty() && serverPort > 0) {
            game->mpMode = true;
            game->serverAddress = serverAddress;
            game->serverPort = serverPort;
            
        } else if (!loadMapUser.empty() && loadMapId != -1) {
            std::cout << "Singleplayer mode: setting up map loading..." << std::endl;
            game->loadMapUser = loadMapUser;
            game->loadMapId = loadMapId;
        }
        
        std::cout << "Calling game->run()..." << std::endl;
        emscripten_async_call([](void* arg) {
            CrossCraft* g = static_cast<CrossCraft*>(arg);
            g->run();
        }, game, 0);
        
    } catch (const std::exception& e) {
        std::cout << "ERROR in CrossCraftApplet::start(): " << e.what() << std::endl;
    }
    
    std::cout << "=== CrossCraftApplet::start() finished ===" << std::endl;
}


void CrossCraftApplet::pause() {
    if (game) {
        game->pause();
    }
}

void CrossCraftApplet::resume() {
    if (game) {
        game->resume();
    }
}

void CrossCraftApplet::destroy() {
    if (game) {
        game->stop();
        delete game;
        game = nullptr;
        std::cout << "CrossCraft applet destroyed" << std::endl;
    }
}

void CrossCraftApplet::initFS() {
    mkdir("/.crosscraft", 0777);

    EM_ASM(
        FS.mount(IDBFS, {}, '/.crosscraft');

        FS.syncfs(true, function (err) {
            if (err) console.error('Error loading filesystem:', err);
            else console.log('IndexedDB initialized');
        });
    );
}

extern "C" {
    void EMSCRIPTEN_KEEPALIVE setAppletParams(const char* username, const char* sessionid, 
                                            const char* loadmapUser, int loadmapId, 
                                            int width, int height, int gamemode) {
        std::string user = username ? username : "";
        std::string session = sessionid ? sessionid : "";
        std::string mapUser = loadmapUser ? loadmapUser : "";
        
        std::cout << "C interface: setAppletParams called" << std::endl;
        std::cout << "  username: " << user << std::endl;
        std::cout << "  sessionid: " << session << std::endl;
        std::cout << "  size: " << width << "x" << height << std::endl;
        
        CrossCraftApplet::getInstance()->setParams(user, session, mapUser, loadmapId, width, height, gamemode);
    }

    void EMSCRIPTEN_KEEPALIVE setServerParams(const char* server, int port) {
        std::cout << "C interface: setServerParams called" << std::endl;
        std::cout << "  server: " << (server ? server : "null") << std::endl;
        std::cout << "  port: " << port << std::endl;
        
        if (server && port > 0) {
            CrossCraftApplet::getInstance()->setServerParams(server, port);
        } else {
            std::cout << "Warning: Invalid server parameters" << std::endl;
        }
    }
    
    void EMSCRIPTEN_KEEPALIVE startApplet() {
        std::cout << "C interface: startApplet called" << std::endl;
        if (CrossCraftApplet::getInstance()) {
            CrossCraftApplet::getInstance()->start();
        } else {
            std::cout << "Error: appletInstance is null!" << std::endl;
        }
    }

    void EMSCRIPTEN_KEEPALIVE testAsyncify() {
        std::cout << "Before sleep" << std::endl;
        emscripten_sleep(1000);
        std::cout << "After sleep" << std::endl;
    }
}
