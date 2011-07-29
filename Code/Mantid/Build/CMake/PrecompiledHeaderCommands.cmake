#
# - Defines macros for dealing with precompiled header files on platforms that
#   support them
#
# MACRO ( ADD_PRECOMPILED_HEADER HEADER SOURCE BIN_NAME SOURCE_FILES )
#    Adds the given header and source file as precompiled header files 
#       HEADER - The relative path from the current CMakeLists.txt file to the header file
#       INC_PREFIX - The prefix to use for the include file when put into the cpp files
#       BIN_NAME - The name to give compiled binary file
#       SOURCE_FILES - The list of source files from the project
#       HEADER_FILES - The list of header files from the project (mainly for MSVC so that they show up in the tree)
MACRO ( ADD_PRECOMPILED_HEADER HEADER INC_PREFIX BIN_NAME SOURCE_FILES HEADER_FILES )
    IF ( MSVC )
        SET ( PRECOMPILED_BIN "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${BIN_NAME}.pch" )
        GET_FILENAME_COMPONENT( HEADER_FILE ${HEADER} NAME )
        IF ( "${INC_PREFIX}" STREQUAL "" )
            SET ( PRECOMPILED_HDR "${HEADER_FILE}" )
        ELSE ()
            SET ( PRECOMPILED_HDR "${INC_PREFIX}/${HEADER_FILE}" )
        ENDIF ()
        SET ( PRECOMPILED_SRC "${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}PrecompiledHeader.cpp" )
        # Generate a source file
        ADD_CUSTOM_COMMAND( OUTPUT ${PRECOMPILED_SRC}
                            DEPENDS ${PRECOMPILED_HDR}
                            COMMAND ${CMAKE_COMMAND} 
                            ARGS -E touch ${PRECOMPILED_SRC} 
        )
        SET_SOURCE_FILES_PROPERTIES ( ${PRECOMPILED_SRC} PROPERTIES GENERATED true )
        SET_SOURCE_FILES_PROPERTIES ( ${PRECOMPILED_SRC}
                                      PROPERTIES COMPILE_FLAGS "/Yc\"${PRECOMPILED_HDR}\" /Fp\"${PRECOMPILED_BIN}\" /FI\"${PRECOMPILED_HDR}\""
                                      OBJECT_OUTPUTS "${PRECOMPILED_BIN}"
        )
        SET_SOURCE_FILES_PROPERTIES ( ${${SOURCE_FILES}}
                                      PROPERTIES COMPILE_FLAGS "/Yu\"${PRECOMPILED_HDR}\" /FI\"${PRECOMPILED_HDR}\" /Fp\"${PRECOMPILED_BIN}\""
                                      OBJECT_DEPENDS "${PRECOMPILED_BIN}" 
        )
        LIST ( APPEND ${SOURCE_FILES} ${PRECOMPILED_SRC} )
        LIST ( APPEND ${HEADER_FILES} ${HEADER} )
    ENDIF ( MSVC )
ENDMACRO( ADD_PRECOMPILED_HEADER )