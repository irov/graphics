CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

include(cmake/glad_download.cmake)
include(cmake/glfw_download.cmake)

project(graphics_test C)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLFW_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR}
)

add_executable(${PROJECT_NAME} graphics_test.c)

message("UNIX: ${UNIX}")
message("APPLE: ${APPLE}")
message("ANDROID: ${ANDROID}")

if(UNIX AND NOT ANDROID)
    target_link_libraries(${PROJECT_NAME} m)
    target_link_libraries(${PROJECT_NAME} c)    
    target_link_libraries(${PROJECT_NAME} dl)
    target_link_libraries(${PROJECT_NAME} X11)
    target_link_libraries(${PROJECT_NAME} Xxf86vm)
    target_link_libraries(${PROJECT_NAME} Xrandr)
    target_link_libraries(${PROJECT_NAME} rt)
    target_link_libraries(${PROJECT_NAME} GL)
endif()

if(APPLE)
    add_apple_framework(${PROJECT_NAME} Cocoa)
    add_apple_framework(${PROJECT_NAME} IOKit)
    add_apple_framework(${PROJECT_NAME} CoreVideo)
    add_apple_framework(${PROJECT_NAME} OpenAL)
endif()

target_link_libraries(${PROJECT_NAME}
    graphics
    glad
    glfw
)

add_dependencies(${PROJECT_NAME}
    glad_download
    glfw_download
)