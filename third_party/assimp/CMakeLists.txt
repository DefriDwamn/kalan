set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

find_package(assimp QUIET)
if (NOT assimp_FOUND)
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3
    )
    FetchContent_MakeAvailable(assimp)
endif()
