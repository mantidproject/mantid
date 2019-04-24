###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################
add_subdirectory ( common )
add_subdirectory ( mplcpp )
add_subdirectory ( instrumentview )
add_subdirectory ( plugins )

if ( ENABLE_MANTIDPLOT ) 
  add_subdirectory ( legacyqwt )
  add_subdirectory ( sliceviewer )
  add_subdirectory ( spectrumviewer )
  add_subdirectory ( refdetectorview)
  add_subdirectory ( factory )
endif() 
