# This file will setup some common items that later setups depend on

# Set the version of ParaView that is compatible with the Mantid code base

if(${ParaView_VERSION})
  set ( COMPATIBLE_PARAVIEW_VERSION "${ParaView_VERSION}" )
else ()
  set ( COMPATIBLE_PARAVIEW_VERSION "-1.-1.-1" )
endif ()
