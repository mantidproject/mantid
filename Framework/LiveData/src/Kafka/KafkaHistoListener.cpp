#include "MantidLiveData/Kafka/KafkaHistoListener.h"

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/KafkaHistoStreamDecoder.h"
#include "MantidLiveData/Kafka/KafkaBroker.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"


namespace {
Mantid::Kernel::Logger g_log("KafkaHistoListener");
}

namespace Mantid {
namespace LiveData {

DECLARE_LISTENER(KafkaHistoListener)

KafkaHistoListener::KafkaHistoListener() {
  declareProperty("InstrumentName", "");
}

/// @copydoc ILiveListener::connect
bool KafkaHistoListener::connect(const Poco::Net::SocketAddress &address) {
  auto broker = std::make_shared<KafkaBroker>(address.toString());
  try {
    std::string instrumentName = getProperty("InstrumentName");
    const std::string histoTopic(instrumentName +
                                 KafkaTopicSubscriber::HISTO_TOPIC_SUFFIX);
    m_decoder = Kernel::make_unique<KafkaHistoStreamDecoder>(
        broker, histoTopic, "SANS2D"); //instrumentName);
  } catch (std::exception &exc) {
    g_log.error() << "KafkaHistoListener::connect - Connection Error: "
                  << exc.what() << "\n";
    return false;
  }
  return true;
}

/// @copydoc ILiveListener::start
void KafkaHistoListener::start(Types::Core::DateAndTime startTime) {
  if (startTime != 0) {
    g_log.warning() << "KafkaHistoListener does not currently support starting "
                       "from arbitrary time." << std::endl;
  }
  m_decoder->startCapture(true);
}

/// @copydoc ILiveListener::extractData
boost::shared_ptr<API::Workspace> KafkaHistoListener::extractData() {
  if (!m_decoder) {
    g_log.error("KafkaHistoListener::extractData(): Kafka is not connected");
    throw Kernel::Exception::InternetError("Kafka is not connected");
  }

  if (!m_decoder->hasData()) {
    // extractData() is called too early
    throw LiveData::Exception::NotYet("Histo Data not available yet.");
  }

  return m_decoder->extractData();
}

/// @copydoc ILiveListener::isConnected
bool KafkaHistoListener::isConnected() {
  return (m_decoder ? m_decoder->isCapturing() : false);
}

/// @copydoc ILiveListener::runStatus
API::ILiveListener::RunStatus KafkaHistoListener::runStatus() {
  if (!m_decoder) {
    g_log.error("KafkaHistoListener::runStatus(): Kafka is not connected");
    throw Kernel::Exception::InternetError("Kafka is not connected");
  }

  return m_decoder->hasReachedEndOfRun() ? EndRun : Running;
}

/// @copydoc ILiveListener::runNumber
int KafkaHistoListener::runNumber() const {
  return (m_decoder ? m_decoder->runNumber() : -1);
}

} // namespace LiveData
} // namespace Mantid
