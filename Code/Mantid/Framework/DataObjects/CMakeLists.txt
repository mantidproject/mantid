set ( SRC_FILES src/AbsManagedWorkspace2D.cpp
      src/CompressedWorkspace2D.cpp src/EventList.cpp
      src/EventWorkspace.cpp src/Histogram1D.cpp
      src/ManagedDataBlock2D.cpp src/ManagedWorkspace2D.cpp
      src/TableColumn.cpp src/TableWorkspace.cpp src/Workspace1D.cpp
      src/Workspace2D.cpp src/WorkspaceSingleValue.cpp 
      src/EventWorkspaceHelpers.cpp src/EventWorkspaceHelpers.cpp)

set ( INC_FILES inc/MantidDataObjects/AbsManagedWorkspace2D.h
      inc/MantidDataObjects/CompressedWorkspace2D.h
      inc/MantidDataObjects/EventList.h
      inc/MantidDataObjects/EventWorkspace.h
      inc/MantidDataObjects/Histogram1D.h
      inc/MantidDataObjects/ManagedDataBlock2D.h
      inc/MantidDataObjects/ManagedWorkspace2D.h
      inc/MantidDataObjects/TableColumn.h
      inc/MantidDataObjects/TablePointerColumn.h
      inc/MantidDataObjects/TableWorkspace.h
      inc/MantidDataObjects/Workspace1D.h
      inc/MantidDataObjects/Workspace2D.h
      inc/MantidDataObjects/WorkspaceSingleValue.h )

# For Windows:
add_definitions ( -DIN_MANTID_DATA_OBJECTS )

# Add the target for this directory
add_library ( DataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( DataObjects PROPERTIES OUTPUT_NAME MantidDataObjects )
# Add to the 'Framework' group in VS
set_property ( TARGET DataObjects PROPERTY FOLDER "Framework" )

include_directories ( ${ZLIB_INCLUDE_DIRS} )
	  
target_link_libraries ( DataObjects ${MANTIDLIBS} ${ZLIB_LIBRARIES} )
