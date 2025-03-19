set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

find_package(raylib_cpp QUIET)
if (NOT raylib_cpp_FOUND)
    FetchContent_Declare(
        raylib_cpp
        GIT_REPOSITORY https://github.com/RobLoach/raylib-cpp.git
        GIT_TAG v5.5.0
    )
    FetchContent_MakeAvailable(raylib_cpp)
endif()
