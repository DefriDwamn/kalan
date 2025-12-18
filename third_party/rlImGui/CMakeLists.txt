set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

find_package(imgui QUIET)
if(NOT imgui_FOUND)
  FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5
  )
  FetchContent_MakeAvailable(ImGui)
  add_library(
    imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
  )
  target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})
endif()

find_package(rlimgui QUIET)
if(NOT rlimgui_FOUND)
  FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui
    GIT_TAG main
  )
  FetchContent_MakeAvailable(rlImGui)
  add_library(rlimgui STATIC ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
  target_include_directories(rlimgui INTERFACE ${rlimgui_SOURCE_DIR})
  target_link_libraries(rlimgui PRIVATE imgui raylib)
endif()

# Fix compiling issues
target_compile_options(imgui PRIVATE -Wno-nontrivial-memcall)
target_compile_options(rlimgui PRIVATE -Wno-nontrivial-memcall)