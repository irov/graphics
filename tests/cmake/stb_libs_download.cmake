if(NOT EXISTS ${GRAPHICS_DEPENDENCIES_DIR}/stb)
    message("Downloading stb libs, this may take some time!")
    file(DOWNLOAD "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" "${GRAPHICS_DEPENDENCIES_DIR}/stb/stb_image.h" SHOW_PROGRESS)    
endif()
