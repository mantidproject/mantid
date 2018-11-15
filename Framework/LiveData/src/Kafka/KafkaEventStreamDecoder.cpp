// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.tcc"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/ba57_run_info_generated.h"
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/ev42_events_generated.h"
#include "private/Schema/f142_logdata_generated.h"
#include "private/Schema/is84_isis_events_generated.h"
GNU_DIAG_ON("conversion")

using namespace Mantid::Types;
using namespace LogSchema;

namespace {
/// Logger
Mantid::Kernel::Logger g_log("KafkaEventStreamDecoder");

const std::string PROTON_CHARGE_PROPERTY = "proton_charge";
const std::string RUN_NUMBER_PROPERTY = "run_number";
const std::string RUN_START_PROPERTY = "run_start";

// File identifiers from flatbuffers schema
const std::string RUN_MESSAGE_ID = "ba57";
const std::string EVENT_MESSAGE_ID = "ev42";
const std::string SAMPLE_MESSAGE_ID = "f142";

/**
 * Append sample log data to existing log or create a new log if one with
 * specified name does not already exist
 *
 * @tparam T : Type of the log value
 * @param mutableRunInfo : Log manager containing the existing sample logs
 * @param name : Name of the sample log
 * @param time : Time at which the value was measured
 * @param value : Sample log measured value
 */
template <typename T>
void appendToLog(Mantid::API::Run &mutableRunInfo, const std::string &name,
                 const Core::DateAndTime &time, T value) {
  if (mutableRunInfo.hasProperty(name)) {
    auto property = mutableRunInfo.getTimeSeriesProperty<T>(name);
    property->addValue(time, value);
  } else {
    auto property = new Mantid::Kernel::TimeSeriesProperty<T>(name);
    property->addValue(time, value);
    mutableRunInfo.addLogData(property);
  }
}
} // namespace

namespace Mantid {
namespace LiveData {
using Types::Core::DateAndTime;
using Types::Event::TofEvent;

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------
/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param eventTopic The name of the topic streaming the event data
 * @param spDetTopic The name of the topic streaming the spectrum-detector
 * run mapping
 */
KafkaEventStreamDecoder::KafkaEventStreamDecoder(
    std::shared_ptr<IKafkaBroker> broker, const std::string &eventTopic,
    const std::string &runInfoTopic, const std::string &spDetTopic,
    const std::string &sampleEnvTopic)
    : IKafkaStreamDecoder(broker, eventTopic, runInfoTopic, spDetTopic,
                          sampleEnvTopic) {}

/**
 * Destructor.
 * Stops capturing from the stream
 */
KafkaEventStreamDecoder::~KafkaEventStreamDecoder() {}

/**
 * Check if there is data available to extract
 * @return True if data has been accumulated so that extractData()
 * can be called, false otherwise
 */
bool KafkaEventStreamDecoder::hasData() const noexcept {
  std::lock_guard<std::mutex> lock(m_mutex);
  return !m_localEvents.empty();
}

/**
 * Check if a message has indicated that end of run has been reached
 * @return  True if end of run has been reached
 */
bool KafkaEventStreamDecoder::hasReachedEndOfRun() noexcept {
  // Notify the decoder that MonitorLiveData knows it has reached end of run
  // and after giving it opportunity to interrupt, decoder can continue with
  // messages of the next run
  if (!m_extractedEndRunData || m_extractWaiting)
    return false;
  if (m_endRun) {
    std::lock_guard<std::mutex> runStatusLock(m_runStatusMutex);
    m_runStatusSeen = true;
    m_cvRunStatus.notify_one();
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------

API::Workspace_sptr KafkaEventStreamDecoder::extractDataImpl() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_localEvents.size() == 1) {
    auto temp = createBufferWorkspace<DataObjects::EventWorkspace>(
        "EventWorkspace", m_localEvents.front());
    std::swap(m_localEvents.front(), temp);
    return temp;
  } else if (m_localEvents.size() > 1) {
    auto group = boost::make_shared<API::WorkspaceGroup>();
    size_t index(0);
    for (auto &filledBuffer : m_localEvents) {
      auto temp = createBufferWorkspace<DataObjects::EventWorkspace>(
          "EventWorkspace", filledBuffer);
      std::swap(m_localEvents[index++], temp);
      group->addWorkspace(temp);
    }
    return group;
  } else {
    throw Exception::NotYet("Local buffers not initialized.");
  }
}

/**
 * Exception-throwing variant of captureImpl(). Do not call this directly
 */
void KafkaEventStreamDecoder::captureImplExcept() {
  g_log.debug("Event capture starting");

  // Load spectra-detector and runstart struct then initialise the cache
  std::string buffer;
  std::string runBuffer;
  int64_t offset;
  int32_t partition;
  std::string topicName;
  m_spDetStream->consumeMessage(&buffer, offset, partition, topicName);
  auto runStartStruct = getRunStartMessage(runBuffer);
  initLocalCaches(buffer, runStartStruct);

  m_interrupt = false; // Allow MonitorLiveData or user to interrupt
  m_endRun = false; // Indicates to MonitorLiveData that end of run is reached
  m_runStatusSeen = false; // Flag to ensure MonitorLiveData observes end of run
  // Flag to ensure LoadLiveData extracts data before start of next run
  m_extractedEndRunData = true;

  // Keep track of whether we've reached the end of a run
  std::unordered_map<std::string, std::vector<int64_t>> stopOffsets;
  std::unordered_map<std::string, std::vector<bool>> reachedEnd;
  bool checkOffsets = false;

  while (!m_interrupt) {
    if (m_endRun) {
      waitForRunEndObservation();
      continue;
    } else {
      waitForDataExtraction();
    }
    // Pull in events
    m_dataStream->consumeMessage(&buffer, offset, partition, topicName);
    // No events, wait for some to come along...
    if (buffer.empty()) {
      m_cbIterationEnd();
      continue;
    }

    if (checkOffsets) {
      checkRunEnd(topicName, checkOffsets, offset, partition, stopOffsets,
                  reachedEnd);
      if (offset > stopOffsets[topicName][static_cast<size_t>(partition)]) {
        // If the offset is beyond the end of the current run, then skip to
        // the next iteration and don't process the message
        m_cbIterationEnd();
        continue;
      }
    }

    // Check if we have an event message
    // Most will be event messages so we check for this type first
    if (flatbuffers::BufferHasIdentifier(
            reinterpret_cast<const uint8_t *>(buffer.c_str()),
            EVENT_MESSAGE_ID.c_str())) {
      eventDataFromMessage(buffer);
    }
    // Check if we have a sample environment log message
    else if (flatbuffers::BufferHasIdentifier(
                 reinterpret_cast<const uint8_t *>(buffer.c_str()),
                 SAMPLE_MESSAGE_ID.c_str())) {
      sampleDataFromMessage(buffer);
    }
    // Check if we have a runMessage
    else
      checkRunMessage(buffer, checkOffsets, stopOffsets, reachedEnd);
    m_cbIterationEnd();
  }
  g_log.debug("Event capture finished");
}

void KafkaEventStreamDecoder::eventDataFromMessage(const std::string &buffer) {
  auto eventMsg =
      GetEventMessage(reinterpret_cast<const uint8_t *>(buffer.c_str()));

  DateAndTime pulseTime = static_cast<int64_t>(eventMsg->pulse_time());
  const auto &tofData = *(eventMsg->time_of_flight());
  const auto &detData = *(eventMsg->detector_id());
  auto nEvents = tofData.size();

  DataObjects::EventWorkspace_sptr periodBuffer;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (eventMsg->facility_specific_data_type() == FacilityData_ISISData) {
    auto ISISMsg =
        static_cast<const ISISData *>(eventMsg->facility_specific_data());
    periodBuffer = m_localEvents[static_cast<size_t>(ISISMsg->period_number())];
    auto &mutableRunInfo = periodBuffer->mutableRun();
    mutableRunInfo.getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)
        ->addValue(pulseTime, ISISMsg->proton_charge());
  } else {
    periodBuffer = m_localEvents[0];
  }
  for (decltype(nEvents) i = 0; i < nEvents; ++i) {
    auto &spectrum = periodBuffer->getSpectrum(
        m_specToIdx[static_cast<int32_t>(detData[i])]);
    spectrum.addEventQuickly(TofEvent(static_cast<double>(tofData[i]) *
                                          1e-3, // nanoseconds to microseconds
                                      pulseTime));
  }
}

/**
 * Get sample environment log data from the flatbuffer and append it to the
 * workspace
 *
 * @param seData : flatbuffer offset of the sample environment log data
 * @param nSEEvents : number of sample environment log values in the flatbuffer
 * @param mutableRunInfo : Log manager containing the existing sample logs
 */
void KafkaEventStreamDecoder::sampleDataFromMessage(const std::string &buffer) {

  std::lock_guard<std::mutex> lock(m_mutex);
  // Add sample log values to every workspace for every period
  for (const auto &periodBuffer : m_localEvents) {
    auto &mutableRunInfo = periodBuffer->mutableRun();

    auto seEvent =
        GetLogData(reinterpret_cast<const uint8_t *>(buffer.c_str()));

    auto name = seEvent->source_name()->str();

    // Convert time from nanoseconds since 1 Jan 1970 to nanoseconds since 1 Jan
    // 1990 to create a Mantid timestamp
    const int64_t nanoseconds1970To1990 = 631152000000000000L;
    auto time = Core::DateAndTime(static_cast<int64_t>(seEvent->timestamp()) -
                                  nanoseconds1970To1990);

    // If sample log with this name already exists then append to it
    // otherwise create a new log
    if (seEvent->value_type() == Value_Int) {
      auto value = static_cast<const Int *>(seEvent->value());
      appendToLog<int32_t>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == Value_Long) {
      auto value = static_cast<const Long *>(seEvent->value());
      appendToLog<int64_t>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == Value_Double) {
      auto value = static_cast<const Double *>(seEvent->value());
      appendToLog<double>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == Value_Float) {
      auto value = static_cast<const Float *>(seEvent->value());
      appendToLog<double>(mutableRunInfo, name, time,
                          static_cast<double>(value->value()));
    } else {
      g_log.warning() << "Value for sample log named '" << name
                      << "' was not of recognised type" << std::endl;
    }
  }
}

/**
 * Pull information from the run & detector-spectrum stream and initialize
 * the internal EventWorkspace buffer + other cached information such as run
 * start. This includes loading the instrument.
 * By the end of this method the local event buffer is ready to accept
 * events
 */
void KafkaEventStreamDecoder::initLocalCaches(
    const std::string &rawMsgBuffer, const RunStartStruct &runStartData) {

  if (rawMsgBuffer.empty()) {
    throw std::runtime_error("KafkaEventStreamDecoder::initLocalCaches() - "
                             "Empty message received from spectrum-detector "
                             "topic. Unable to continue");
  }
  auto spDetMsg = GetSpectraDetectorMapping(
      reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
  auto nspec = static_cast<uint32_t>(spDetMsg->n_spectra());
  auto nudet = spDetMsg->detector_id()->size();
  if (nudet != nspec) {
    std::ostringstream os;
    os << "KafkaEventStreamDecoder::initLocalEventBuffer() - Invalid "
          "spectra/detector mapping. Expected matched length arrays but "
          "found nspec="
       << nspec << ", ndet=" << nudet;
    throw std::runtime_error(os.str());
  }

  m_runNumber = runStartData.runNumber;

  // Create buffer
  auto eventBuffer = createBufferWorkspace<DataObjects::EventWorkspace>(
      "EventWorkspace", static_cast<size_t>(spDetMsg->n_spectra()),
      spDetMsg->spectrum()->data(), spDetMsg->detector_id()->data(), nudet);

  // Load the instrument if possible but continue if we can't
  auto instName = runStartData.instrumentName;
  if (!instName.empty())
    loadInstrument<DataObjects::EventWorkspace>(instName, eventBuffer);
  else
    g_log.warning(
        "Empty instrument name received. Continuing without instrument");

  auto &mutableRun = eventBuffer->mutableRun();
  // Run start. Cache locally for computing frame times
  // Convert nanoseconds to seconds (and discard the extra precision)
  auto runStartTime = static_cast<time_t>(runStartData.startTime / 1000000000);
  m_runStart.set_from_time_t(runStartTime);
  auto timeString = m_runStart.toISO8601String();
  // Run number
  mutableRun.addProperty(RUN_START_PROPERTY, std::string(timeString));
  mutableRun.addProperty(RUN_NUMBER_PROPERTY,
                         std::to_string(runStartData.runNumber));
  // Create the proton charge property
  mutableRun.addProperty(
      new Kernel::TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY));

  // Cache spec->index mapping. We assume it is the same across all periods
  m_specToIdx = eventBuffer->getSpectrumToWorkspaceIndexMap();

  // Buffers for each period
  const size_t nperiods = runStartData.nPeriods;
  if (nperiods == 0) {
    throw std::runtime_error(
        "KafkaEventStreamDecoder - Message has n_periods==0. This is "
        "an error by the data producer");
  }
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_localEvents.resize(nperiods);
    m_localEvents[0] = eventBuffer;
    for (size_t i = 1; i < nperiods; ++i) {
      // A clone should be cheap here as there are no events yet
      m_localEvents[i] = eventBuffer->clone();
    }
  }

  // New caches so LoadLiveData's output workspace needs to be replaced
  m_dataReset = true;
}

} // namespace LiveData

} // namespace Mantid
