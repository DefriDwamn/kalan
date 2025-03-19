set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

find_package(entt QUIET)
if (NOT entt_FOUND)
    FetchContent_Declare(
        entt
        GIT_REPOSITORY "https://github.com/skypjack/entt.git"
        GIT_TAG v3.14.0
    )
    FetchContent_MakeAvailable(entt)
endif()
