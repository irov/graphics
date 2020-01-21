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

target_link_libraries(${PROJECT_NAME}
    graphics
    glad
    glfw
)

add_dependencies(${PROJECT_NAME}
    glad_download
    glfw_download
)