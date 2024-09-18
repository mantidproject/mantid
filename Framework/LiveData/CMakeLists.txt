set(SRC_FILES
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
)

set(INC_FILES
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
    src/ISIS/DAE/idc.h
    src/ISIS/DAE/isisds_command.h
)

set(TEST_FILES
    # Needs fixing to not rely on network. SNSLiveEventDataListenerTest.h
    ADARAPacketTest.h
    FakeEventDataListenerTest.h
    FileEventDataListenerTest.h
    ISISHistoDataListenerTest.h
    LiveDataAlgorithmTest.h
    LoadLiveDataTest.h
    MonitorLiveDataTest.h
    StartLiveDataTest.h
)

find_package(LibRDKafka 0.11)
if(LIBRDKAFKA_FOUND)
  set(SRC_FILES
      ${SRC_FILES}
      src/Kafka/IKafkaStreamDecoder.cpp
      src/Kafka/KafkaEventListener.cpp
      src/Kafka/KafkaEventStreamDecoder.cpp
      src/Kafka/KafkaHistoListener.cpp
      src/Kafka/KafkaHistoStreamDecoder.cpp
      src/Kafka/KafkaBroker.cpp
      src/Kafka/KafkaTopicSubscriber.cpp
  )
  set(INC_FILES
      ${INC_FILES}
      inc/MantidLiveData/Kafka/KafkaEventListener.h
      inc/MantidLiveData/Kafka/KafkaEventStreamDecoder.h
      inc/MantidLiveData/Kafka/IKafkaStreamSubscriber.h
      inc/MantidLiveData/Kafka/IKafkaBroker.h
      inc/MantidLiveData/Kafka/IKafkaStreamDecoder.h
      inc/MantidLiveData/Kafka/IKafkaStreamDecoder.tcc
      inc/MantidLiveData/Kafka/KafkaBroker.h
      inc/MantidLiveData/Kafka/KafkaHistoListener.h
      inc/MantidLiveData/Kafka/KafkaHistoStreamDecoder.h
      inc/MantidLiveData/Kafka/KafkaTopicSubscriber.h
      src/Kafka/private/Schema/flatbuffers/flatbuffers.h
      src/Kafka/private/Schema/flatbuffers/base.h
      src/Kafka/private/Schema/flatbuffers/stl_emulation.h
      src/Kafka/private/Schema/6s4t_run_stop_generated.h
      src/Kafka/private/Schema/df12_det_spec_map_generated.h
      src/Kafka/private/Schema/dtdb_adc_pulse_debug_generated.h
      src/Kafka/private/Schema/ev42_events_generated.h
      src/Kafka/private/Schema/f142_logdata_generated.h
      src/Kafka/private/Schema/fwdi_forwarder_internal_generated.h
      src/Kafka/private/Schema/is84_isis_events_generated.h
      src/Kafka/private/Schema/pl72_run_start_generated.h
      src/Kafka/private/Schema/tdct_timestamps_generated.h
      src/Kafka/private/Schema/hs00_event_histogram_generated.h
  )
  set(TEST_FILES ${TEST_FILES} KafkaEventStreamDecoderTest.h KafkaHistoStreamDecoderTest.h KafkaTopicSubscriberTest.h)
endif()

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Use a precompiled header where they are supported
enable_precompiled_headers(inc/MantidLiveData/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(LiveData ${SRC_FILES} ${INC_FILES})
add_library(Mantid::LiveData ALIAS LiveData)

target_include_directories(
  LiveData PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)
# Set the name of the generated library
set_target_properties(LiveData PROPERTIES OUTPUT_NAME MantidLiveData)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(LiveData PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(LiveData PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET LiveData PROPERTY FOLDER "MantidFramework")

# Have to link to winsock library on Windows
if(WIN32)
  set(WINSOCK ws2_32)
endif()

set(PRIVATE_LINK_LIBRARIES ${WINSOCK})
if(LIBRDKAFKA_FOUND)
  set(PRIVATE_LINK_LIBRARIES ${PRIVATE_LINK_LIBRARIES} ${LibRDKafka_LIBRARIES})
  target_include_directories(LiveData PRIVATE ${LibRDKafka_INCLUDE_DIR})
endif()
target_link_libraries(
  LiveData
  PUBLIC Mantid::Kernel Mantid::API Mantid::DataObjects Mantid::Geometry Mantid::NexusGeometry
  PRIVATE ${PRIVATE_LINK_LIBRARIES} Mantid::Indexing
)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  LiveData PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/LiveData> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(LiveData FALSE)

# Installation settings
mtd_install_framework_lib(TARGETS LiveData PLUGIN_LIB)
