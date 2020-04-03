include(ExternalProject)

ExternalProject_Add(imguicolortextedit_download PREFIX imguicolortextedit
        GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
		GIT_PROGRESS TRUE
            
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy 
			${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeLists_imguicolortextedit.txt
			${CMAKE_CURRENT_BINARY_DIR}/imguicolortextedit/src/imguicolortextedit_download/CMakeLists.txt
      
        CMAKE_ARGS 
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DIMGUI_INCLUDE_DIR=${IMGUI_INCLUDE_DIR}
    )
    
ExternalProject_Add_StepDependencies(imguicolortextedit_download build imgui_download)
    
ExternalProject_Get_Property(imguicolortextedit_download INSTALL_DIR)
set(IMGUICOLORTEXTEDIT_INCLUDE_DIR ${INSTALL_DIR}/include)
set(IMGUICOLORTEXTEDIT_LIBRARY_DIR ${INSTALL_DIR}/lib)

add_library(imguicolortextedit STATIC IMPORTED)

set_target_properties(imguicolortextedit PROPERTIES IMPORTED_LOCATION ${IMGUICOLORTEXTEDIT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}imguicolortextedit${CMAKE_STATIC_LIBRARY_SUFFIX})