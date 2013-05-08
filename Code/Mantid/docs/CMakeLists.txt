set ( ENABLE_QTASSISTANT True CACHE BOOL "Build qt-assistant documentation" )

if (ENABLE_QTASSISTANT)
  add_subdirectory ( qtassistant )
endif (ENABLE_QTASSISTANT)
