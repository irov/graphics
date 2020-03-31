cmake_minimum_required(VERSION 3.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

macro(ADD_GRAPHICS_TEST testname)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    
    ADD_EXECUTABLE(test_${testname} test_${testname}.c)
    TARGET_LINK_LIBRARIES(test_${testname} graphics)

    set_target_properties (test_${testname} PROPERTIES
        FOLDER tests
    )

    ADD_TEST(NAME ${testname} COMMAND ${ROOT_DIR}/bin/${CMAKE_GENERATOR}/test_${testname} ${ROOT_DIR}/bin/${CMAKE_GENERATOR})
endmacro()

ADD_GRAPHICS_TEST(memory_leak)