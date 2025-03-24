set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

find_package(rlImGui QUIET)
if(NOT raylib_FOUND)
  FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/SafenM/rlImGui-CMake.git
    GIT_TAG main
    GIT_SHALLOW 1)
  FetchContent_MakeAvailable(rlImGui)
endif()
