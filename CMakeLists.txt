cmake_minimum_required(VERSION 3.16)
project(CrossCraftWebPP)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build.cfg")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/build.cfg" BUILD_CFG_CONTENT)
    string(REGEX MATCH "gamemode=([0-9]+)" _ ${BUILD_CFG_CONTENT})
    set(GAMEMODE ${CMAKE_MATCH_1})
    string(REGEX MATCH "debug=([0-9]+)" _ ${BUILD_CFG_CONTENT})
    set(DEBUG_MODE ${CMAKE_MATCH_1})
else()
    set(GAMEMODE 1)
    set(DEBUG_MODE 0)
endif()
message(STATUS "Game mode: ${GAMEMODE} (0=survival, 1=creative), Debug: ${DEBUG_MODE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/level)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/phys)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/render)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/stb)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS 
    "src/*.hpp" 
    "src/*.h"
    "include/*.h" 
    "include/*.hpp"
)

message(STATUS "Source files: ${SOURCES}")

add_library(stb_vorbis STATIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/stb/stb_impl.c")
target_include_directories(stb_vorbis PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/stb")

add_executable(CrossCraftWebPP ${SOURCES} ${HEADERS})

if(EMSCRIPTEN)
    add_compile_options(-sUSE_ZLIB=1)
    target_link_libraries(CrossCraftWebPP PRIVATE 
        glfw
        websocket.js
        stb_vorbis
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/libgc.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/libGLU.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/libGL.a"
    )

    if(GAMEMODE EQUAL 0)
        set(OUTPUT_NAME "survival")
        set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell_survival.html")
    else()
        set(OUTPUT_NAME "game")
        set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell_creative.html")
    endif()

    if(DEBUG_MODE EQUAL 1)
        set(OPTIM_FLAGS "-O0 -g")
    elseif(DEBUG_MODE EQUAL 2)
        set(OPTIM_FLAGS "\
            -O2 \
            -flto \
            -msimd128 \
            -gline-tables-only \
            -sASSERTIONS=1 \
            -sSTACK_OVERFLOW_CHECK=1 \
        ")
    else()
        set(OPTIM_FLAGS "-O3 -flto -msimd128")
    endif()

    set_target_properties(CrossCraftWebPP PROPERTIES 
        COMPILE_FLAGS "-sUSE_ZLIB=1 ${OPTIM_FLAGS}"
        LINK_FLAGS "\
            -s WASM=1 \
            -s USE_GLFW=3 \
            -s FULL_ES2=1 \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s MAXIMUM_MEMORY=4GB \
            -s ASYNCIFY=1 \
            -s ASYNCIFY_IGNORE_INDIRECT=0 \
            -s ASYNCIFY_STACK_SIZE=2097152 \
            -s USE_ZLIB=1 \
            -s FETCH=1 \
            -s LEGACY_GL_EMULATION=0 \
            -s GL_FFP_ONLY=0 \
            -s GL_POOL_TEMP_BUFFERS=1 \
            -s MAX_WEBGL_VERSION=2 \
            -s EXPORTED_FUNCTIONS=['_main','_setAppletParams','_startApplet','_testAsyncify','_setServerParams','_showCrashScreen'] \
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','intArrayFromString','UTF8ToString'] \
            -s ASYNCIFY_REMOVE=[\"Chunk::*\",\"Tessellator::*\",\"LevelRenderer::*\",\"Frustum::*\",\"Tile::*\",\"Level::*\"] \
            -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
            -s STACK_SIZE=8388608 \
            -s INITIAL_MEMORY=536870912 \
            -s NO_EXIT_RUNTIME=1 \
            -l openal \
            -lidbfs.js \
            --allow-multiple-definition \
            --shell-file \"${SHELL_FILE}\" \
            --preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/resources@/\" \
            ${OPTIM_FLAGS} \
        "
    )
    set_target_properties(CrossCraftWebPP PROPERTIES OUTPUT_NAME "${OUTPUT_NAME}")
    set_target_properties(CrossCraftWebPP PROPERTIES SUFFIX ".html")
    
    set(TEXTURE_FILES
        terrain.png
        char.png 
        rock.png
        rock2.png
        dirt.png
        water.png
        default.png
        clouds.png
        gui/gui.png
        armor/plate.png
        armor/chain.png
        mob/zombie.png
        mob/skeleton.png
        mob/pig.png
        rain.png
        particles.png
        mob/spider.png
        mob/sheep.png
        mob/sheep_fur.png
    )
    
    add_custom_command(TARGET CrossCraftWebPP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Copying resources to build directory"
    )

endif()