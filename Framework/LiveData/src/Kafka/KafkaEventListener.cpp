#include "MantidLiveData/Kafka/KafkaEventListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"
#include "MantidLiveData/Kafka/KafkaBroker.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

namespace {
Mantid::Kernel::Logger g_log("KafkaEventListener");
}

namespace Mantid {
namespace LiveData {

DECLARE_LISTENER(KafkaEventListener)

KafkaEventListener::KafkaEventListener() {
  declareProperty("InstrumentName", "");
}

/// @copydoc ILiveListener::connect
bool KafkaEventListener::connect(const Poco::Net::SocketAddress &address) {
  auto broker = std::make_shared<KafkaBroker>(address.toString());
  try {
    std::string instrumentName = getProperty("InstrumentName");
    const std::string eventTopic(instrumentName +
                                 KafkaTopicSubscriber::EVENT_TOPIC_SUFFIX),
        runInfoTopic(instrumentName + KafkaTopicSubscriber::RUN_TOPIC_SUFFIX),
        spDetInfoTopic(instrumentName +
                       KafkaTopicSubscriber::DET_SPEC_TOPIC_SUFFIX);
    m_decoder = Kernel::make_unique<KafkaEventStreamDecoder>(
        broker, eventTopic, runInfoTopic, spDetInfoTopic);
  } catch (std::exception &exc) {
    g_log.error() << "KafkaEventListener::connect - Connection Error: "
                  << exc.what() << "\n";
    return false;
  }
  return true;
}

/// @copydoc ILiveListener::start
void KafkaEventListener::start(Kernel::DateAndTime startTime) {
  bool startNow = true;
  // Workaround for existing LiveListener interface
  // startTime of 0 means start from now
  // startTime of 1000000000 nanoseconds from epoch means start from start of
  // run and we do not support starting from arbitrary time in this listener
  if (startTime.totalNanoseconds() == 1000000000) {
    startNow = false;
  } else if (startTime != 0) {
    g_log.warning() << "KafkaLiveListener does not currently support starting "
                       "from arbitrary time." << std::endl;
  }
  m_decoder->startCapture(startNow);
}

/// @copydoc ILiveListener::extractData
boost::shared_ptr<API::Workspace> KafkaEventListener::extractData() {
  assert(m_decoder);
  // The first call to extract is very early in the start live data process
  // and we may not be completely ready yet, wait upto a maximum of 5 seconds
  // to become ready
  auto checkEnd = std::chrono::system_clock::now() + std::chrono::seconds(5);
  while (!m_decoder->hasData()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (std::chrono::system_clock::now() > checkEnd) {
      break;
    }
  }
  return m_decoder->extractData();
}

/// @copydoc ILiveListener::isConnected
bool KafkaEventListener::isConnected() {
  return (m_decoder ? m_decoder->isCapturing() : false);
}

/// @copydoc ILiveListener::runStatus
API::ILiveListener::RunStatus KafkaEventListener::runStatus() {
  return m_decoder->hasReachedEndOfRun() ? EndRun : Running;
}

/// @copydoc ILiveListener::runNumber
int KafkaEventListener::runNumber() const {
  return (m_decoder ? m_decoder->runNumber() : -1);
}
}
}
