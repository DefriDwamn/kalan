set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) 
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW 1
    )
    FetchContent_MakeAvailable(raylib)
endif()
