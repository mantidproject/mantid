#
# Now add the packages one-by-one, building up the dependencies as we go
#
add_subdirectory(algorithm_dialogs)
if(ENABLE_MANTIDPLOT)
  add_subdirectory(designer)
endif()
