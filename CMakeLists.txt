cmake_minimum_required(VERSION 3.16)

project(qmdlr VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig)
include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

find_package(imgui CONFIG REQUIRED)
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(glm CONFIG REQUIRED)

find_package(zstd CONFIG REQUIRED)

# Pass -DQMDLR_ASAN=YES to enable for all targets
if (QMDLR_ASAN)
    message(STATUS "Enabling ASan on all targets")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

set(PROJECT_SOURCES
    main.cpp
    Types.h
    System.h
    System.cpp
    UI.h
    UI.cpp
    Events.h
    Events.cpp
    Math.h
    ModelData.h
    ModelMutator.h
    ModelMutator.cpp
    ModelLoader.h
    ModelLoader.cpp
    Stream.h
    Camera.h
    Camera.cpp
    renderdoc_app.h
    MDLRenderer.h
    MDLRenderer.cpp
    Settings.h
    Settings.cpp
    Editor3D.h
    Editor3D.cpp
    EditorUV.h
    EditorUV.cpp
    Widgets.h
    EnumArray.h
    UVRenderer.h
    UVRenderer.cpp
    UndoRedo.h
    UndoRedo.cpp
    Format.h
    Log.h
    Log.cpp
    Images.h
    Images.cpp
)

set(THIRD_PARTY_SOURCES
    glad/src/glad.c
    thirdparty/stb_image/stb_image.h
    thirdparty/tcpp/tcppLibrary.hpp
)

add_subdirectory(thirdparty/nativefiledialog-extended)

add_executable(qmdlr
    ${PROJECT_SOURCES}
    ${THIRD_PARTY_SOURCES}
)

target_include_directories(qmdlr PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty>
)

target_link_libraries(qmdlr PRIVATE imgui::imgui)

if(TARGET SDL2::SDL2main)
    target_link_libraries(qmdlr PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(qmdlr PRIVATE SDL2::SDL2 nfd)

target_link_libraries(qmdlr PRIVATE glm::glm-header-only)

target_link_libraries(qmdlr PRIVATE tomlplusplus::tomlplusplus)

target_link_libraries(qmdlr PRIVATE zstd::libzstd)

#file(GLOB SHADER_SOURCE_FILES LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/res/shaders/**")
#file(GLOB RESOURCE_SOURCE_FILES LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/res/**")

#add_custom_command(
    #TARGET qmdlr POST_BUILD
    #COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:qmdlr>/res
    #COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:qmdlr>/res/shaders
    #COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:qmdlr>/themes

    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RESOURCE_SOURCE_FILES} $<TARGET_FILE_DIR:qmdlr>/res
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_SOURCE_FILES} $<TARGET_FILE_DIR:qmdlr>/res/shaders
#)

set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.paril.qmdlr)
set_target_properties(qmdlr PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS qmdlr
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)