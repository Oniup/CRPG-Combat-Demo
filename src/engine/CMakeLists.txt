INCLUDE(FetchContent)

# Dependencies
MACRO(FetchContent name repo tag)
    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY ${repo}
        GIT_TAG ${tag}
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(${name})
ENDMACRO()

# Raylib
SET(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent(libfmt        https://github.com/fmtlib/fmt.git     "12.1.0")
FetchContent(raylib        https://github.com/raysan5/raylib.git "5.5")
FetchContent(raygui        https://github.com/raysan5/raygui.git "4.0")

# Setup Raygui library, because its a STB style library
SET(RAYGUI_IMPL_FILE "${CMAKE_CURRENT_BINARY_DIR}/RayguiImpl.c")
FILE(WRITE ${RAYGUI_IMPL_FILE} "#define RAYGUI_IMPLEMENTATION\n#include \"raygui.h\"\n")
ADD_LIBRARY(raygui STATIC ${RAYGUI_IMPL_FILE})
MESSAGE("Raylib GUI path: ${raygui_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES(raygui
PUBLIC
    ${raygui_SOURCE_DIR}/src
)
TARGET_COMPILE_DEFINITIONS(raygui
PRIVATE
    _CRT_SECURE_NO_WARNINGS
)
TARGET_LINK_LIBRARIES(raygui
PUBLIC
    raylib
)

ADD_LIBRARY(engine STATIC)

# Engine source
ADD_SUBDIRECTORY(engine/ai)
ADD_SUBDIRECTORY(engine/core)
ADD_SUBDIRECTORY(engine/graphics)
ADD_SUBDIRECTORY(engine/utilities)
ADD_SUBDIRECTORY(engine/world)

TARGET_INCLUDE_DIRECTORIES(engine
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_COMPILE_DEFINITIONS(engine
PRIVATE
)

TARGET_PRECOMPILE_HEADERS(engine
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pch.h
)

TARGET_LINK_LIBRARIES(engine
PUBLIC
    fmt::fmt
    raylib
    raygui
)

if (MSVC)
    TARGET_COMPILE_OPTIONS(engine
    PUBLIC
        /Zc:preprocessor # Needed for __VA_OPT__() macro
    )
endif()
