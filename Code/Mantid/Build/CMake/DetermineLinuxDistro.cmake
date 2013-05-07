
find_program (LSB_CMD lsb_release)
if ( LSB_CMD )
  # get the distribution
  execute_process ( COMMAND ${LSB_CMD} -i
    OUTPUT_VARIABLE UNIX_DIST
    OUTPUT_STRIP_TRAILING_WHITESPACE )
  string ( REGEX REPLACE "Distributor ID:" "" UNIX_DIST ${UNIX_DIST} )
  string ( STRIP ${UNIX_DIST} UNIX_DIST )
  string ( REGEX REPLACE "RedHatEnterpriseClient" "RedHatEnterprise" UNIX_DIST ${UNIX_DIST} )
  string ( REGEX REPLACE "RedHatEnterpriseWorkstation" "RedHatEnterprise" UNIX_DIST ${UNIX_DIST} )
  string ( REGEX REPLACE "Scientific" "RedHatEnterprise" UNIX_DIST ${UNIX_DIST} )
  string ( REGEX REPLACE "CentOS" "RedHatEnterprise" UNIX_DIST ${UNIX_DIST} )
  # get the codename
  execute_process ( COMMAND ${LSB_CMD} -c
    OUTPUT_VARIABLE UNIX_CODENAME
    OUTPUT_STRIP_TRAILING_WHITESPACE )
  string ( REGEX REPLACE "Codename:" "" UNIX_CODENAME ${UNIX_CODENAME} )
  string ( STRIP ${UNIX_CODENAME} UNIX_CODENAME )
else ( LSB_CMD )
  set ( UNIX_DIST "" )
  set ( UNIX_CODENAME "" )
endif ( LSB_CMD )
message ( STATUS " DIST: ${UNIX_DIST} CODENAME: ${UNIX_CODENAME}" )
