set ( SRC_FILES
    src/ONCat.cpp
    src/ONCatEntity.cpp
    src/OAuth.cpp
)

set ( INC_FILES
    inc/MantidCatalog/DllConfig.h
    inc/MantidCatalog/Exception.h
    inc/MantidCatalog/ONCat.h
    inc/MantidCatalog/ONCatEntity.h
    inc/MantidCatalog/OAuth.h
)

set ( TEST_FILES
    ONCatTest.h
    ONCatEntityTest.h
    OAuthTest.h
)

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Catalog SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add ssl dependency
include_directories ( ${OPENSSL_INCLUDE_DIR} )
add_definitions ( -DWITH_OPENSSL -DWITH_NONAMESPACES )

# Add a precompiled header where they are supported
enable_precompiled_headers( inc/MantidCatalog/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( Catalog ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Catalog PROPERTIES OUTPUT_NAME MantidCatalog
  COMPILE_DEFINITIONS IN_MANTID_CATALOG
)

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Catalog PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set_target_properties(Catalog PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET Catalog PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( Catalog LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS} ${OPENSSL_LIBRARIES} ${JSONCPP_LIBRARIES})

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Catalog ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
