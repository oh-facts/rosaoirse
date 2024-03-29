
#someone please please suggest something better than cmake. if your answer is #make. then your answer is only valid if it looks cleaner than this cmake. #Also,please suggest a better way to manage my source tree. It is absolute #pain.

cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(yk3)

# to disable stupid msvc errors
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

#--------target--------#
include_directories(
    src/
	src/platform/
    lib/
)

# engine code
set(esrc "src/yk_renderer.c" "lib/stb/stb_truetype.c")

# game code
set(gsrc "src/yk_game.c")

# game + engine built as dll
add_library(${PROJECT_NAME} SHARED ${esrc} ${gsrc}) 

# platform code

if(WIN32)
set(psrc "src/platform/win32_platform.c" "lib/miniaudio/miniaudio.c")
elseif(UNIX)
set(psrc "src/platform/linux_platform.c" "lib/miniaudio/miniaudio.c")
endif()

# platform is entry
add_executable(app  ${psrc})

if(UNIX)
    # need to link with m on unix for miniaudio to compile
    target_link_libraries(app m)
endif()

#-------------------------------------------------------------------#