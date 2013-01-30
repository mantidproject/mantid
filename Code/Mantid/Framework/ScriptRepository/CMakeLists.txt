
####
## SET THE LIB GIT 2 FILES
####
# Find required dependencies
#maybe INCLUDE_DIRECTORIES (libgit2/src as well
INCLUDE_DIRECTORIES(SYSTEM libgit2/include libgit2/deps/http-parser libgit2/src/ libgit2/deps/regex)

IF (NOT WIN32)
  FIND_PACKAGE(ZLIB)
ENDIF()

IF (WIN32 AND NOT MINGW)
	ADD_DEFINITIONS(-DGIT_WINHTTP)
ELSE ()
	FIND_PACKAGE(OpenSSL)
	SET (SRC_HTTP libgit2/deps/http-parser/http_parser.c)
	SET (INC_HTTP libgit2/deps/http-parser/http_parser.h)
ENDIF()

IF (ZLIB_FOUND)
 INCLUDE_DIRECTORIES (SYSTEM ${ZLIB_INCLUDE_DIRS})
 LINK_LIBRARIES(${ZLIB_LIBRARIES})
 SET (SRC_ZLIB "")
ELSE()
  INCLUDE_DIRECTORIES(SYSTEM libgit2/deps/zlib)
  ADD_DEFINITIONS(-DNO_VIZ -DSTDC -DNO_GZIP)
  SET (SRC_ZLIB
    libgit2/deps/zlib/adler32.c
    libgit2/deps/zlib/crc32.c
    libgit2/deps/zlib/deflate.c
    libgit2/deps/zlib/inffast.c
    libgit2/deps/zlib/inflate.c
    libgit2/deps/zlib/inftrees.c
    libgit2/deps/zlib/trees.c
    libgit2/deps/zlib/zutil.c
    )
ENDIF()  

# Specify sha1 implementation
IF (SHA1_TYPE STREQUAL "ppc")
	ADD_DEFINITIONS(-DPPC_SHA1)
	FILE(GLOB SRC_SHA1 libgit2/src/hash/hash_ppc.c libgit2/src/hash/hash_ppc_core.S)
ELSEIF (WIN32 AND NOT MINGW AND NOT SHA1_TYPE STREQUAL "builtin")
    ADD_DEFINITIONS(-DWIN32_SHA1)
    FILE(GLOB SRC_SHA1 libgit2/src/hash/hash_win32.c)
ELSEIF (OPENSSL_FOUND AND NOT SHA1_TYPE STREQUAL "builtin")
	ADD_DEFINITIONS(-DOPENSSL_SHA1)
ELSE()
	FILE(GLOB SRC_SHA1 libgit2/src/hash/hash_generic.c)
ENDIF()

# On Windows use specific platform sources
IF (WIN32 AND NOT CYGWIN)
	##ADD_DEFINITIONS(-DWIN32 -D_WIN32_WINNT=0x0501)
	FILE(GLOB GIT2_SRC libgit2/src/*.c libgit2/src/transports/*.c libgit2/src/xdiff/*.c libgit2/src/win32/*.c libgit2/src/compat/*.c)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "(Solaris|SunOS)")
	FILE(GLOB GIT2_SRC libgit2/src/*.c libgit2/src/transports/*.c libgit2/src/xdiff/*.c libgit2/src/unix/*.c libgit2/src/compat/*.c)
ELSEIF (AMIGA)
	ADD_DEFINITIONS(-DNO_ADDRINFO -DNO_READDIR_R)
	FILE(GLOB GIT2_SRC libgit2/src/*.c libgit2/src/transports/*.c libgit2/src/xdiff/*.c libgit2/src/amiga/*.c libgit2/src/compat/*.c)
ELSE()
	FILE(GLOB GIT2_SRC libgit2/src/*.c libgit2/src/transports/*.c libgit2/src/xdiff/*.c libgit2/src/unix/*.c)
ENDIF ()

IF (MSVC)
	# Default to stdcall, as that's what the CLR expects and how the Windows API is built
	OPTION (STDCALL "Buildl libgit2 with the __stdcall convention" ON)

ENDIF()

SET (SRC_REGEX libgit2/deps/regex/regex.c)
SET (INC_REGEX libgit2/deps/regex/regex.h)


# Libgit2 source
set ( LIBGIT2_SRC 
  ${SRC_ZLIB} ${SRC_REGEX} ${SRC_HTTP}
  ${SRC_SHA1}  ${GIT2_SRC} 
)

# Mantid source
set ( SRC_FILES 
  src/GitScriptRepository.cpp
)

set ( INC_FILES inc/MantidScriptRepository/GitScriptRepository.h
  ${INC_ZLIB}
  ${INC_HTTP}

)
#
#set ( TEST_FILES
#	GitScriptRepositoryTest.h
#        ScriptRepositoryFactoryTest.h)
#
# uncomment these lines if you want to be able to test the
# features that depend on internet connection
#
#set (TEST_FILES
#    GitConnectionTest.h
#    GitNewFeatureTest.h
#)

## Ignore all warnings from libgit2 source
if (MSVC)
  set_source_files_properties ( ${LIBGIT2_SRC} 
                                PROPERTIES COMPILE_FLAGS "/w" )
else()
  set_source_files_properties ( ${LIBGIT2_SRC} 
                                PROPERTIES COMPILE_FLAGS "-w" )
endif()

# Add the target for this directory
add_library ( ScriptRepository ${SRC_FILES} ${LIBGIT2_SRC} ${INC_FILES} )
# Add the unit tests directory
add_subdirectory ( test )
# Set the name of the generated library
set_target_properties ( ScriptRepository PROPERTIES OUTPUT_NAME MantidScriptRepository 
                        COMPILE_DEFINITIONS IN_MANTID_SCRIPTREPO)

set_property (TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

include_directories(inc)
target_link_libraries(ScriptRepository ${MANTIDLIBS})

install (TARGETS ScriptRepository ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
