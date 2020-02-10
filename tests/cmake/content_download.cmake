if(NOT EXISTS ${GRAPHICS_CONTENT_DIR}/texture.jpg)
    message("Downloading texture, this may take some time!")
    
    file(DOWNLOAD "https://c.pxhere.com/photos/c5/89/chess_chess_board_game_board_flag_target_start_black_and_white_checkered-1195657.jpg!d" "${GRAPHICS_CONTENT_DIR}/texture.jpg" SHOW_PROGRESS)    
endif()
