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
    find_library(LIBRT rt)
    target_link_libraries(${PROJECT_NAME} ${LIBRT})
    MESSAGE("LIBRT: ${LIBRT_FOUND}")

    find_library(MATH m)
    target_link_libraries(${PROJECT_NAME} ${MATH})
    MESSAGE("MATH: ${MATH_FOUND}")

    target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
    MESSAGE("CMAKE_DL_LIBS: ${CMAKE_DL_LIBS}")

    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${X11_X11_LIB} ${X11_Xrandr_LIB} ${X11_Xinerama_LIB} ${X11_Xcursor_LIB})
    MESSAGE("X11: ${X11_FOUND}")
    MESSAGE("X11_xf86vmode_FOUND: ${X11_xf86vmode_FOUND}")

    if(X11_xf86vmode_FOUND)
        target_link_libraries(${PROJECT_NAME} ${X11_Xxf86vm_LIB})
    endif()

    find_package(Threads)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
    MESSAGE("Threads: ${Threads}")

    target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})
    MESSAGE("GTK3_LIBRARIES: ${GTK3_LIBRARIES}")
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