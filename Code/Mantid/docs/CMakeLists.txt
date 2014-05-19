if ( APPLE )
  set ( ENABLE_QTASSISTANT False CACHE BOOL "Build qt-assistant documentation" )
else ()
  set ( ENABLE_QTASSISTANT True CACHE BOOL "Build qt-assistant documentation" )
endif ( APPLE )

if (ENABLE_QTASSISTANT)
  add_subdirectory ( qtassistant )
endif (ENABLE_QTASSISTANT)
