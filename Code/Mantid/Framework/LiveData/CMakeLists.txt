set ( SRC_FILES
	src/ADARA/ADARAPackets.cpp
	src/ADARA/ADARAParser.cpp
	src/FakeEventDataListener.cpp
	src/FakeISISHistoDAE.cpp
	src/FileEventDataListener.cpp
	src/ISISHistoDataListener.cpp
	src/ISISLiveEventDataListener.cpp
	src/LiveDataAlgorithm.cpp
	src/LoadDAE/idc.cpp
	src/LoadDAE/idc.h
	src/LoadDAE/isisds_command.cpp
	src/LoadDAE/isisds_command.h
	src/LoadLiveData.cpp
	src/MonitorLiveData.cpp
	src/SNSLiveEventDataListener.cpp
	src/StartLiveData.cpp
	src/TOPAZLiveEventDataListener.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/LoadDAE/idc.cpp
        src/LoadDAE/isisds_command.cpp)

set ( INC_FILES
	inc/MantidLiveData/ADARA/ADARA.h
	inc/MantidLiveData/ADARA/ADARAPackets.h
	inc/MantidLiveData/ADARA/ADARAParser.h
	inc/MantidLiveData/Exception.h
	inc/MantidLiveData/FakeEventDataListener.h
	inc/MantidLiveData/FakeISISHistoDAE.h
	inc/MantidLiveData/FileEventDataListener.h
	inc/MantidLiveData/ISISHistoDataListener.h
	inc/MantidLiveData/ISISLiveEventDataListener.h
	inc/MantidLiveData/ISIS/TCPEventStreamDefs.h
	inc/MantidLiveData/LiveDataAlgorithm.h
	inc/MantidLiveData/LoadLiveData.h
	inc/MantidLiveData/MonitorLiveData.h
	inc/MantidLiveData/SNSLiveEventDataListener.h
	inc/MantidLiveData/StartLiveData.h
	inc/MantidLiveData/TOPAZLiveEventDataListener.h
)

set ( TEST_FILES
	#Needs fixing to not rely on network. SNSLiveEventDataListenerTest.h
	ADARAPacketTest.h
	FakeEventDataListenerTest.h
	FileEventDataListenerTest.h
	ISISHistoDataListenerTest.h
	LiveDataAlgorithmTest.h
	LoadLiveDataTest.h
	MonitorLiveDataTest.h
	StartLiveDataTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(LiveData SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidLiveData/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( LiveData ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( LiveData PROPERTIES OUTPUT_NAME MantidLiveData )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( LiveData PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
endif ()
 
# Add to the 'Framework' group in VS
set_property ( TARGET LiveData PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( LiveData ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS LiveData ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
