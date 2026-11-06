cmake_minimum_required(VERSION 3.28)

project(wasm-uvmkit-host
        VERSION 1.0
        DESCRIPTION "WebAssembly UVMKIT host"
        LANGUAGES C ASM)


add_subdirectory("../../" uvmkit)
add_executable(host host.c)
target_link_libraries(host uvmkit)

set(HTML_SHELL ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)

target_link_options(host PRIVATE
        "-sENVIRONMENT=web"
        "-sSINGLE_FILE=1"
        "-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_load_image"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','HEAPU8']"
	--shell-file ${HTML_SHELL}
	)

set_target_properties(host PROPERTIES
        OUTPUT_NAME host
        SUFFIX ".html"
        LINK_DEPENDS ${HTML_SHELL}
    )
    
# add_custom_command(
#     OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/index.html"
#     DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
#     "${CMAKE_CURRENT_BINARY_DIR}/index.html"
# )

# # Create a target for the file copy
# add_custom_target(copy_index ALL
#     DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/index.html"
# )

# # Ensure the host target waits for the copy to happen
# add_dependencies(host copy_index)
