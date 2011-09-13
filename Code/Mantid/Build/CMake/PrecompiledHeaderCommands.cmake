#
# - Defines macros for dealing with precompiled header files on platforms that
#   support them

# A switch to turn them on and off
# 
SET ( USE_PRECOMPILED_HEADERS ON CACHE BOOL "If true, will use precompiled headers on those platforms that support it."  )

# MACRO ( ADD_PRECOMPILED_HEADER HEADER SOURCE PCH_CPP SOURCE_FILES )
#    Adds the given header and source file as precompiled header files 
#       HEADER - The relative path from the current CMakeLists.txt file to the header file
#       INC_PREFIX - The prefix to use for the include file when put into the cpp files
#       PCH_CPP - The path to the source file responsible for creating the precompiled header
#       SOURCE_FILES - The list of source files from the project
#       HEADER_FILES - The list of header files from the project (mainly for MSVC so that they show up in the tree)
MACRO ( ADD_PRECOMPILED_HEADER PCH_H INC_PREFIX PCH_CPP SOURCE_FILES HEADER_FILES )
    IF ( USE_PRECOMPILED_HEADERS )
        IF ( MSVC )
            GET_FILENAME_COMPONENT( HEADER_FILE ${PCH_H} NAME )
            IF ( "${INC_PREFIX}" STREQUAL "" )
                SET ( PRECOMPILED_HDR "${HEADER_FILE}" )
            ELSE ()
                SET ( PRECOMPILED_HDR "${INC_PREFIX}/${HEADER_FILE}" )
            ENDIF ()
            STRING ( REGEX MATCH "test/.*" TESTDIR ${PCH_H} )
            if ( "${TESTDIR}" STREQUAL "" ) 
              SET ( PRECOMPILED_BIN "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/project.pch" )
            else ()
              SET ( PRECOMPILED_BIN "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/projecttest.pch" )
            endif()
              SET_SOURCE_FILES_PROPERTIES ( ${PCH_CPP}
                                          PROPERTIES COMPILE_FLAGS "/Yc\"${PRECOMPILED_HDR}\" /Fp\"${PRECOMPILED_BIN}\" /FI\"${PRECOMPILED_HDR}\""
                                          OBJECT_OUTPUTS "${PRECOMPILED_BIN}"
            )
            SET_SOURCE_FILES_PROPERTIES ( ${${SOURCE_FILES}}
                                          PROPERTIES COMPILE_FLAGS "/Yu\"${PRECOMPILED_HDR}\" /FI\"${PRECOMPILED_HDR}\" /Fp\"${PRECOMPILED_BIN}\""
                                          OBJECT_DEPENDS "${PRECOMPILED_BIN}" 
            )
            LIST ( APPEND ${SOURCE_FILES} ${PCH_CPP} )
            LIST ( APPEND ${HEADER_FILES} ${PCH_H} )
        ENDIF ( MSVC )
    ENDIF()
ENDMACRO( ADD_PRECOMPILED_HEADER )