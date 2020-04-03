include(ExternalProject)

ExternalProject_Add(lua_download PREFIX lua
        GIT_REPOSITORY https://github.com/lua/lua.git
		GIT_TAG "v5.3.5"
		GIT_PROGRESS TRUE
            
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy 
			${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeLists_lua.txt
			${CMAKE_CURRENT_BINARY_DIR}/lua/src/lua_download/CMakeLists.txt
      
        CMAKE_ARGS 
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )
    
ExternalProject_Get_Property(lua_download INSTALL_DIR)
set(LUA_INCLUDE_DIR ${INSTALL_DIR}/include)
set(LUA_LIBRARY_DIR ${INSTALL_DIR}/lib)

add_library(lua STATIC IMPORTED)

set_target_properties(lua PROPERTIES IMPORTED_LOCATION ${LUA_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}lua${CMAKE_STATIC_LIBRARY_SUFFIX})