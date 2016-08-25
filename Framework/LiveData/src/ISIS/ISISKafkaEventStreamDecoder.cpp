#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/WarningSuppressions.h"

GCC_DIAG_OFF(conversion)
#include "private/Kafka/Schema/event_schema_generated.h"
#include "private/Kafka/Schema/det_spec_mapping_schema_generated.h"
GCC_DIAG_ON(conversion)

#include <boost/make_shared.hpp>

#include <functional>
#include <iostream>

namespace {
Mantid::Kernel::Logger &LOGGER() {
  static Mantid::Kernel::Logger logger("ISISKafkaEventStreamDecoder");
  return logger;
}
}

namespace Mantid {
using API::WorkspaceFactory;
using DataObjects::EventWorkspace;
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------
/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param eventTopic The name of the topic streaming the event data
 * @param spDetTopic The name of the topic streaming the spectrum-detector
 * mapping
 */
ISISKafkaEventStreamDecoder::ISISKafkaEventStreamDecoder(
    const IKafkaBroker &broker, std::string eventTopic,
    std::string runInfoTopic, std::string spDetTopic)
    : m_interrupt(false), m_eventStream(broker.subscribe(eventTopic)),
      m_localEvents(), m_runStream(broker.subscribe(runInfoTopic)),
      m_spDetStream(broker.subscribe(spDetTopic)), m_thread(),
      m_capturing(false), m_exception() {}

/**
 * Destructor.
 * Stops capturing from the stream
 */
ISISKafkaEventStreamDecoder::~ISISKafkaEventStreamDecoder() { stopCapture(); }

/**
 * Start capturing from the stream on a separate thread. This is a non-blocking
 * call and will return after the thread has started
 */
void ISISKafkaEventStreamDecoder::startCapture() noexcept {
  m_thread = std::thread([this]() { this->captureImpl(); });
  m_thread.detach();
}

/**
 * Stop capturing from the stream. This is a blocking call until the capturing
 * function has completed
 */
void ISISKafkaEventStreamDecoder::stopCapture() noexcept {
  // This will interrupt the "event" loop
  m_interrupt = true;
  // wait until the function has completed
  while (m_capturing) {
  }
}

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------
/**
 * Start decoding data from the streams into the internal buffers.
 * Implementation designed to be entry point for new thread of execution.
 * It catches all thrown exceptions.
 */
void ISISKafkaEventStreamDecoder::captureImpl() noexcept {
  try {
    captureImplExcept();
  } catch (std::exception &exc) {
    m_exception = boost::make_shared<std::runtime_error>(exc.what());
  } catch (...) {
    m_exception = boost::make_shared<std::runtime_error>(
        "ISISKafkaEventStreamDecoder: Unknown exception type caught.");
  }
}

/**
 * Exception-throwing variant of captureImpl()
 */
void ISISKafkaEventStreamDecoder::captureImplExcept() {
  LOGGER().debug("Event capture starting");
  // Use RAII for m_capturing...
  m_capturing = true;
  m_interrupt = false;

  while (!m_interrupt) {
    // pull in events
  }

  m_capturing = false;
  LOGGER().debug("Event capture finished");
}

/**
 * Pull information from the run & detector-spectrum stream and initialize
 * the internal EventWorkspace buffer. This includes loading the instrument.
 * By the end of this method the local event buffer is ready to accept
 * events
 */
void ISISKafkaEventStreamDecoder::initLocalEventBuffer() {
  std::string buffer;
  // Load spectra-detector mapping from stream
  m_spDetStream->consumeMessage(&buffer);
  auto messageData = ISISDAE::GetSpectraDetectorMapping(
      reinterpret_cast<const uint8_t *>(buffer.c_str()));
  m_localEvents = boost::static_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace",
                                          messageData->spec()->size(), 2, 1));
}
}

} // namespace Mantid
