set ( SRC_FILES
	src/ADARA/ADARAPackets.cpp
	src/ADARA/ADARAParser.cpp
	src/FakeEventDataListener.cpp
	src/FileEventDataListener.cpp
	src/ISIS/DAE/idc.cpp
	src/ISIS/DAE/isisds_command.cpp
	src/ISIS/FakeISISEventDAE.cpp
	src/ISIS/FakeISISHistoDAE.cpp
	src/ISIS/ISISHistoDataListener.cpp
	src/ISIS/ISISLiveEventDataListener.cpp
	src/LiveDataAlgorithm.cpp
	src/LoadLiveData.cpp
	src/MonitorLiveData.cpp
	src/SNSLiveEventDataListener.cpp
	src/StartLiveData.cpp
	src/TOPAZLiveEventDataListener.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/ISIS/DAE/idc.cpp
        src/ISIS/DAE/isisds_command.cpp)

set ( INC_FILES
	inc/MantidLiveData/ADARA/ADARA.h
	inc/MantidLiveData/ADARA/ADARAPackets.h
	inc/MantidLiveData/ADARA/ADARAParser.h
	inc/MantidLiveData/Exception.h
	inc/MantidLiveData/FakeEventDataListener.h
	inc/MantidLiveData/FileEventDataListener.h
	inc/MantidLiveData/ISIS/FakeISISEventDAE.h
	inc/MantidLiveData/ISIS/FakeISISHistoDAE.h
	inc/MantidLiveData/ISIS/ISISHistoDataListener.h
	inc/MantidLiveData/ISIS/ISISLiveEventDataListener.h
	inc/MantidLiveData/ISIS/TCPEventStreamDefs.h
	inc/MantidLiveData/LiveDataAlgorithm.h
	inc/MantidLiveData/LoadLiveData.h
	inc/MantidLiveData/MonitorLiveData.h
	inc/MantidLiveData/SNSLiveEventDataListener.h
	inc/MantidLiveData/StartLiveData.h
	inc/MantidLiveData/TOPAZLiveEventDataListener.h
	src/ISIS/DAE/idc.h
	src/ISIS/DAE/isisds_command.h
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

find_package ( LibRDKafka )
if ( LIBRDKAFKA_FOUND )
  set ( SRC_FILES
    ${SRC_FILES}
    src/Kafka/KafkaEventListener.cpp
    src/Kafka/KafkaEventStreamDecoder.cpp
    src/Kafka/KafkaBroker.cpp
    src/Kafka/KafkaTopicSubscriber.cpp
  )
  set ( INC_FILES
    ${INC_FILES}
    inc/MantidLiveData/Kafka/KafkaEventListener.h
    inc/MantidLiveData/Kafka/KafkaEventStreamDecoder.h
    inc/MantidLiveData/Kafka/IKafkaStreamSubscriber.h
    inc/MantidLiveData/Kafka/IKafkaBroker.h
    inc/MantidLiveData/Kafka/KafkaBroker.h
    inc/MantidLiveData/Kafka/KafkaTopicSubscriber.h
    src/Kafka/private/Schema/flatbuffers/flatbuffers.h
    src/Kafka/private/Schema/det_spec_mapping_schema_generated.h
    src/Kafka/private/Schema/event_schema_generated.h
    src/Kafka/private/Schema/run_info_schema_generated.h
  )
  set ( TEST_FILES
    ${TEST_FILES}
    KafkaEventStreamDecoderTest.h
    KafkaTopicSubscriberTest.h
  )
endif()

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

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

# Have to link to winsock library on Windows
if ( WIN32 )
  set ( WINSOCK ws2_32 )
endif ()

set ( PRIVATE_LINK_LIBRARIES ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS} ${WINSOCK})
if ( LIBRDKAFKA_FOUND )
  set ( PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} ${LibRDKafka_LIBRARIES} )
endif()
target_link_libraries ( LiveData LINK_PRIVATE ${PRIVATE_LINK_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS LiveData ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
