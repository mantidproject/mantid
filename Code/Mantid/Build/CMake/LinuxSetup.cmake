###########################################################################
# tcmalloc stuff. Only used on linux for now.
###########################################################################

# Look for tcmalloc. Make it optional, for now at least, but on by default
set ( USE_TCMALLOC ON CACHE BOOL "Flag for replacing regular malloc with tcmalloc" )
# Note that this is not mandatory, so no REQUIRED
find_package ( Tcmalloc )
# If not found, or not wanted, just carry on without it
if ( USE_TCMALLOC AND TCMALLOC_FOUND )
  set ( TCMALLOC_LIBRARY ${TCMALLOC_LIBRARIES} )
  # Make a C++ define to use as flags in, e.g. MemoryManager.cpp
  add_definitions ( -DUSE_TCMALLOC )
endif ()

###########################################################################
# Set installation variables
###########################################################################

set ( BIN_DIR bin )
set ( LIB_DIR lib )
set ( PLUGINS_DIR plugins )

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set ( CMAKE_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME} CACHE PATH "Install path" FORCE )
ENDIF()

set ( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_DIR};${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR} )

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh  "#!/bin/sh\n"
                                                    "MANTIDPATH=${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
                                                    "PATH=$PATH:$MANTIDPATH\n"
                                                    "export MANTIDPATH PATH"
)
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh  "#!/bin/csh\n"
                                                    "setenv MANTIDPATH \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
                                                    "setenv PATH \"$PATH:$MANTIDPATH\"\n"
)

# Note: On older versions of CMake, this line may mean that to do a "make package" without being root
# you will need to set the cache variable CPACK_SET_DESTDIR to ON.
install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
          DESTINATION /etc/profile.d 
)

  IF ( UNIX )
    # define the source generators
    set ( CPACK_SOURCE_GENERATOR TGZ )

    FIND_PROGRAM (LSB_CMD lsb_release)
    IF ( LSB_CMD )
      # get the distribution
      execute_process ( COMMAND ${LSB_CMD} -i
                        OUTPUT_VARIABLE UNIX_DIST
                        OUTPUT_STRIP_TRAILING_WHITESPACE )
      string ( REGEX REPLACE "Distributor ID:" "" UNIX_DIST ${UNIX_DIST} )
      string ( STRIP ${UNIX_DIST} UNIX_DIST )
      string ( REGEX REPLACE "RedHatEnterpriseClient" "RedHatEnterprise" UNIX_DIST ${UNIX_DIST} )
      string ( REGEX REPLACE "RedHatEnterpriseWorkstation" "RedHatEnterprise" UNIX_DIST ${UNIX_DIST} )
      # get the codename
      execute_process ( COMMAND ${LSB_CMD} -c
                        OUTPUT_VARIABLE UNIX_CODENAME
                        OUTPUT_STRIP_TRAILING_WHITESPACE )
      string ( REGEX REPLACE "Codename:" "" UNIX_CODENAME ${UNIX_CODENAME} )
      string ( STRIP ${UNIX_CODENAME} UNIX_CODENAME )
    ELSE ( LSB_CMD )
      set ( UNIX_DIST "" )
      set ( UNIX_CODENAME "" )
    ENDIF ( LSB_CMD )
    message ( STATUS " DIST: ${UNIX_DIST} CODENAME: ${UNIX_CODENAME}" )

    # define which binary generators to use
    IF ( ${UNIX_DIST} MATCHES "Ubuntu" )
      FIND_PROGRAM(DPKG_CMD dpkg)
      IF ( DPKG_CMD )
        set ( CPACK_GENERATOR "DEB" )
        execute_process( COMMAND ${DPKG_CMD} --print-architecture
                         OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
                         OUTPUT_STRIP_TRAILING_WHITESPACE )
        # acording to debian <foo>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb 
        set( CPACK_PACKAGE_FILE_NAME
             "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")   
      ENDIF( DPKG_CMD ) 
    ENDIF ( ${UNIX_DIST} MATCHES "Ubuntu" )

    #RedHatEnterpriseClient RedHatEnterpriseWorkstation
    IF ( ${UNIX_DIST} MATCHES "RedHatEnterprise" )
      FIND_PROGRAM ( RPMBUILD_CMD rpmbuild )
      IF ( RPMBUILD_CMD )
        set ( CPACK_GENERATOR "RPM" )
        FIND_PROGRAM ( UNAME_CMD uname )
        IF (UNAME_CMD)
          execute_process( COMMAND ${UNAME_CMD} -m
                           OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE
                           OUTPUT_STRIP_TRAILING_WHITESPACE )
        ELSE (UNAME_CMD)
          set (CPACK_RPM_PACKAGE_ARCHITECTURE "unknown")
        ENDIF (UNAME_CMD)
        # reset the release name to include the RHEL version if known
        IF ( ${UNIX_CODENAME} MATCHES "Tikanga" )
          set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.el5" )
        ELSEIF ( ${UNIX_CODENAME} MATCHES "Santiago" )
          set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.el6" )
        ENDIF ( ${UNIX_CODENAME} MATCHES "Tikanga" )
        
        # according to rpm.org: name-version-release.architecture.rpm
        set ( CPACK_PACKAGE_FILE_NAME
            "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_PACKAGE_ARCHITECTURE}" )
       
    ENDIF ( RPMBUILD_CMD)
    ENDIF ( ${UNIX_DIST} MATCHES "RedHatEnterprise" )
  ENDIF ( UNIX )