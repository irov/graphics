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

if(UNIX AND NOT APPLE)
    find_library(RT_LIBRARY rt)
    target_link_libraries(${PROJECT_NAME} ${RT_LIBRARY})

    find_library(MATH_LIBRARY m)
    target_link_libraries(${PROJECT_NAME} ${MATH_LIBRARY})

    target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})

    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${X11_X11_LIB} ${X11_Xrandr_LIB} ${X11_Xinerama_LIB} ${X11_Xcursor_LIB})

    if(X11_xf86vmode_FOUND)
        target_link_libraries(${PROJECT_NAME} ${X11_Xxf86vm_LIB})
    endif()

    find_package( Threads )
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

    target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})
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