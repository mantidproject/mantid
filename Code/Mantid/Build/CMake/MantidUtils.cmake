#######################################################################
#
# Define some useful macros/functions
#
#######################################################################

#######################################################################
# Change the output directory of a library target. Handles the incorrect 
# behaviour of MSVC
#   Parameters:
#      TARGET_NAME - The name of the target
#      OUTPUT_DIR - The directory to output the target
#   Optional:
#      An extension to use other than the standard (only used for MSVC)
function( MOVE_TARGET TARGET_NAME OUTPUT_DIR )
  # Change the destination of the target as Qt expects this in a directory called "SUB_DIR_NAME"
  if ( MSVC )
      # For some reason when the generator is MSVC  10 it ignores the LIBRARY_OUTPUT_DIRECTORY 
      # property so we have to do something slightly different
      if ( ${ARGC} STREQUAL 3 )
        set ( LIB_EXT ${ARGV2} )
      else ()
        set ( LIB_EXT .dll )
      endif()
      set ( SRC_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
      add_custom_command( TARGET ${TARGET_NAME} POST_BUILD 
                          COMMAND ${CMAKE_COMMAND} ARGS -E make_directory
                          ${OUTPUT_DIR} )
      add_custom_command ( TARGET ${TARGET_NAME} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} ARGS -E echo 
                           "Moving \"$(TargetName)${LIB_EXT}\" to ${OUTPUT_DIR}/$(TargetName)${LIB_EXT}"
                           COMMAND ${CMAKE_COMMAND} ARGS -E rename
                           ${SRC_DIR}/$(TargetName)${LIB_EXT}
                           ${OUTPUT_DIR}/$(TargetName)${LIB_EXT} )
      # Clean up
      set ( LIB_EXT )
      set ( SRC_DIR )
  else ()
    set_target_properties ( ${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR} )
  endif ( MSVC )
endfunction( MOVE_TARGET )
