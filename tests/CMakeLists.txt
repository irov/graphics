cmake_minimum_required(VERSION 3.10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

macro(ADD_GRAPHICS_TEST testname)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    
    ADD_EXECUTABLE(test_${testname} test_${testname}.c)
    TARGET_LINK_LIBRARIES(test_${testname} graphics)
    
    if(UNIX AND NOT ANDROID AND NOT APPLE)
        target_link_libraries(test_${testname} m)
        target_link_libraries(test_${testname} c)
        target_link_libraries(test_${testname} dl)
    endif()

    set_target_properties (test_${testname} PROPERTIES
        FOLDER tests
    )

    ADD_TEST(NAME ${testname} COMMAND $<TARGET_FILE:test_${testname}> $<TARGET_FILE_DIR:test_${testname}>)
endmacro()

ADD_GRAPHICS_TEST(memory_leak)
ADD_GRAPHICS_TEST(api)
ADD_GRAPHICS_TEST(memory)
