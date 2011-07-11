# Put the binaries in a subfolder of the usual bin directory
function(output_binary_to LIB_NAME SUB_DIR_NAME)

  # Change the destination of the target as Qt expects this in a directory called "SUB_DIR_NAME"
  if ( MSVC )
      # For some reason when the generator is MSVC  10 it ignores the  LIBRARY_OUTPUT_DIRECTORY 
      # property so we have to do something slightly different
      set ( LIB_EXTS ".dll" )
      set ( SRC_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
      set ( DEST_DIR ${SRC_DIR}/${SUB_DIR_NAME} )
      add_custom_command( TARGET ${LIB_NAME} POST_BUILD 
                          COMMAND ${CMAKE_COMMAND} ARGS -E make_directory
                          ${DEST_DIR} )
      foreach ( LIB_EXT ${LIB_EXTS} )
        add_custom_command ( TARGET ${LIB_NAME} POST_BUILD
                             COMMAND ${CMAKE_COMMAND} ARGS -E echo 
                             "Moving \"${LIB_NAME}${LIB_EXT}\" to ${SUB_DIR_NAME} directory."
                             COMMAND ${CMAKE_COMMAND} ARGS -E copy
                             ${SRC_DIR}/${LIB_NAME}${LIB_EXT}
                             ${DEST_DIR}/${LIB_NAME}${LIB_EXT} )
      endforeach ( LIB_EXT )
      # Clean up
      set ( LIB_EXTS )
      set ( SRC_DIR )
      set ( DEST_DIR )
  else ()
    set_target_properties ( ${LIB_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY 
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${SUB_DIR_NAME} )
  endif ( MSVC )
endfunction(output_binary_to)