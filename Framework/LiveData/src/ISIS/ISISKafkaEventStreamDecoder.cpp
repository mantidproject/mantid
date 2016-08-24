#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/WarningSuppressions.h"

GCC_DIAG_OFF(conversion)
#include "private/Kafka/Schema/event_schema_generated.h"
GCC_DIAG_ON(conversion)

#include <iostream>

namespace Mantid {
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------
/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param eventTopic The name of the topic streaming the event data
 * @param runInfoTopic The name of the topic streaming the run information
 * @param spDetTopic The name of the topic streaming the spectrum-detector
 * mapping
 */
ISISKafkaEventStreamDecoder::ISISKafkaEventStreamDecoder(
    const IKafkaBroker &broker, std::string eventTopic,
    std::string runInfoTopic, std::string spDetTopic)
    : m_eventStream(broker.subscribe(eventTopic)) /*,
      m_runInfoStream(broker.subscribe(runInfoTopic)),
      m_spDetStream(broker.subscribe(spDetTopic))*/ {}

/**
 * Start decoding data from the streams into the internal buffers. This can be
 * run in a separate thread by passing the address of this object to
 * Poco::Thread::start()
 */
void ISISKafkaEventStreamDecoder::run() {
  std::string resizableBuffer;
  m_eventStream->consumeMessage(&resizableBuffer);
  if (resizableBuffer.empty()) {
    std::cerr << "Empty event stream - Is anything running?\n";
  } else {
    const uint8_t *buf =
        reinterpret_cast<const uint8_t *>(resizableBuffer.c_str());
    auto messageData = ISISDAE::GetEventMessage(buf);
    if (messageData->message_type() == ISISDAE::MessageTypes_FramePart) {
      auto frameData =
          static_cast<const ISISDAE::FramePart *>(messageData->message());
      auto eventData = frameData->n_events();
      auto evtSpecNum = eventData->spec();
      auto evtTof = eventData->tof();
      std::cerr << "Consumed " << evtSpecNum->size()
                << " events from the stream\n";
      std::cerr << "First TOF = " << (*evtTof)[0] << "\n";
    }
  }
}

} // namespace LiveData
} // namespace Mantid
