CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

set(GRAPHICS_DEPENDENCIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
set(GRAPHICS_CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/content)

include(cmake/macro.cmake)
include(cmake/glad_download.cmake)
include(cmake/glfw_download.cmake)
include(cmake/stb_libs_download.cmake)
include(cmake/content_download.cmake)

project(graphics_test C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${GRAPHICS_DEPENDENCIES_DIR})
include_directories(${GLFW_INCLUDE_DIR})
include_directories(${IMGUI_INCLUDE_DIR})
include_directories(${GLAD_INCLUDE_DIR})

ADD_DEFINITIONS(-DGRAPHICS_CONTENT_PATH="${GRAPHICS_CONTENT_DIR}")

add_executable(${PROJECT_NAME} graphics_test.c)

message("UNIX: ${UNIX}")
message("APPLE: ${APPLE}")
message("ANDROID: ${ANDROID}")

target_link_libraries(${PROJECT_NAME}
    graphics
    glad
    glfw
)

add_dependencies(${PROJECT_NAME}
    glad_download
    glfw_download
)

if(UNIX AND NOT ANDROID AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} m)
    target_link_libraries(${PROJECT_NAME} c)
    target_link_libraries(${PROJECT_NAME} dl)
    target_link_libraries(${PROJECT_NAME} X11)
    target_link_libraries(${PROJECT_NAME} Xxf86vm)
    target_link_libraries(${PROJECT_NAME} Xrandr)
    target_link_libraries(${PROJECT_NAME} rt)
    target_link_libraries(${PROJECT_NAME} GL)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

if(APPLE)
    ADD_APPLE_FRAMEWORK(${PROJECT_NAME} Cocoa)
    ADD_APPLE_FRAMEWORK(${PROJECT_NAME} IOKit)
    ADD_APPLE_FRAMEWORK(${PROJECT_NAME} CoreVideo)
    ADD_APPLE_FRAMEWORK(${PROJECT_NAME} OpenAL)
endif()