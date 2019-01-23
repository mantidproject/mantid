// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/KafkaHistoListener.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/KafkaBroker.h"
#include "MantidLiveData/Kafka/KafkaHistoStreamDecoder.h"
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

void KafkaHistoListener::setAlgorithm(
    const Mantid::API::IAlgorithm &callingAlgorithm) {
  this->updatePropertyValues(callingAlgorithm);
  // Get the instrument name from StartLiveData so we can sub to correct topics
  if (callingAlgorithm.existsProperty("Instrument")) {
    m_instrumentName = callingAlgorithm.getPropertyValue("Instrument");
  } else {
    g_log.error("KafkaEventListener requires Instrument property to be set in "
                "calling algorithm");
  }
}

/// @copydoc ILiveListener::connect
bool KafkaHistoListener::connect(const Poco::Net::SocketAddress &address) {
  if (m_instrumentName.empty()) {
    g_log.error(
        "KafkaHistoListener::connect requires a non-empty instrument name");
  }

  try {
    const std::string histoTopic(m_instrumentName +
                                 KafkaTopicSubscriber::HISTO_TOPIC_SUFFIX),
        runInfoTopic(m_instrumentName + KafkaTopicSubscriber::RUN_TOPIC_SUFFIX),
        spDetInfoTopic(m_instrumentName +
                       KafkaTopicSubscriber::DET_SPEC_TOPIC_SUFFIX),
        sampleEnvTopic(m_instrumentName +
                       KafkaTopicSubscriber::SAMPLE_ENV_TOPIC_SUFFIX);

    m_decoder = Kernel::make_unique<KafkaHistoStreamDecoder>(
        std::make_shared<KafkaBroker>(address.toString()), histoTopic,
        runInfoTopic, spDetInfoTopic, sampleEnvTopic);
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
                       "from arbitrary time."
                    << std::endl;
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
    g_log.warning("KafkaHistoListener::runStatus(): Kafka is not connected");
    return NoRun;
  }

  return m_decoder->hasReachedEndOfRun() ? EndRun : Running;
}

/// @copydoc ILiveListener::runNumber
int KafkaHistoListener::runNumber() const {
  return (m_decoder ? m_decoder->runNumber() : -1);
}

} // namespace LiveData
} // namespace Mantid
