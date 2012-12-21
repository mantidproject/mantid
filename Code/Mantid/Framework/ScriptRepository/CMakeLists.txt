include_directories (libgit2/include)
#add_subdirectory (libgit2)


####
## SET THE LIB GIT 2 FILES
####
# Find required dependencies
#maybe INCLUDE_DIRECTORIES (libgit2/src as well
INCLUDE_DIRECTORIES(libgit2/include libgit2/deps/http-parser libgit2/deps/zlib libgit2/deps/regex)

IF (NOT WIN32)
  FIND_PACKAGE(ZLIB)
ENDIF()

IF (ZLIB_FOUND)
 INCLUDE_DIRECTORIES (${ZLIB_INCLUDE_DIRS})
 LINK_LIBRARIES(${ZLIB_LIBRARIES})
 SET (SRC_ZLIB "")
ELSE()
  INCLUDE_DIRECTORIES(libgit2/deps/zlib)
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




SET (SRC_REGEX libgit2/deps/regex/regex.c)
SET (INC_REGEX libgit2/deps/regex/regex.h)

SET (SRC_HTTP libgit2/deps/http-parser/http_parser.c)
SET (INC_HTTP libgit2/deps/http-parser/http_parser.h)





set ( SRC_FILES src/GitScriptRepository.cpp
  ${SRC_ZLIB} ${SRC_REGEX} ${SRC_HTTP}
)

set ( INC_FILES inc/MantidScriptRepository/GitScriptRepository.h
  ${INC_ZLIB}
  ${INC_HTTP}

)

set ( TEST_FILES
	GitScriptRepositoryTest.h
  GitConnectionTest.h
  GitNewFeatureTest.h
  ScriptRepositoryFactoryTest.h
)

# Add the target for this directory
add_library ( ScriptRepository ${SRC_FILES} ${INC_FILES})
# Add the unit tests directory
add_subdirectory ( test )
# Set the name of the generated library
set_target_properties ( ScriptRepository PROPERTIES OUTPUT_NAME MantidScriptRepository )

set_property (TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

include_directories(inc)
target_link_libraries(ScriptRepository ${MANTIDLIBS} PocoZip)

install (TARGETS ScriptRepository ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )