#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/ba57_run_info_generated.h"
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/ev42_events_generated.h"
#include "private/Schema/f142_logdata_generated.h"
#include "private/Schema/is84_isis_events_generated.h"
GNU_DIAG_ON("conversion")

using namespace Mantid::Types;

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

const std::chrono::seconds MAX_LATENCY(1);

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
    : m_broker(broker), m_eventTopic(eventTopic), m_runInfoTopic(runInfoTopic),
      m_spDetTopic(spDetTopic), m_sampleEnvTopic(sampleEnvTopic),
      m_interrupt(false), m_localEvents(), m_specToIdx(), m_runStart(),
      m_runNumber(-1), m_thread(), m_capturing(false), m_exception(),
      m_extractWaiting(false), m_cbIterationEnd([] {}), m_cbError([] {}) {}

/**
 * Destructor.
 * Stops capturing from the stream
 */
KafkaEventStreamDecoder::~KafkaEventStreamDecoder() { stopCapture(); }

/**
 * Start capturing from the stream on a separate thread. This is a non-blocking
 * call and will return after the thread has started
 */
void KafkaEventStreamDecoder::startCapture(bool startNow) {

  // If we are not starting now, then we want to start at the start of the run
  if (!startNow) {
    // Get last two messages in run topic to ensure we get a runStart message
    m_runStream =
        m_broker->subscribe({m_runInfoTopic}, SubscribeAtOption::LASTTWO);
    std::string rawMsgBuffer;
    auto runStartData = getRunStartMessage(rawMsgBuffer);
    joinEventStreamAtTime(runStartData);
  } else {
    m_eventStream =
        m_broker->subscribe({m_eventTopic, m_runInfoTopic, m_sampleEnvTopic},
                            SubscribeAtOption::LATEST);
  }

  // Get last two messages in run topic to ensure we get a runStart message
  m_runStream =
      m_broker->subscribe({m_runInfoTopic}, SubscribeAtOption::LASTTWO);
  m_spDetStream =
      m_broker->subscribe({m_spDetTopic}, SubscribeAtOption::LASTONE);

  m_thread = std::thread([this]() { this->captureImpl(); });
  m_thread.detach();
}

/** Indicate if the next data to be extracted should replace LoadLiveData's
 * output workspace,
 *  for example the first data of a new run
 */
bool KafkaEventStreamDecoder::dataReset() {
  bool result = (m_dataReset == true); // copy from atomic bool
  m_dataReset = false;                 // reset to false
  return result;
}

void KafkaEventStreamDecoder::joinEventStreamAtTime(
    const KafkaEventStreamDecoder::RunStartStruct &runStartData) {
  auto runStartTime = runStartData.startTime;
  int64_t startTimeMilliseconds = nanosecondsToMilliseconds(runStartTime);
  m_eventStream =
      m_broker->subscribe({m_eventTopic, m_runInfoTopic, m_sampleEnvTopic},
                          startTimeMilliseconds, SubscribeAtOption::TIME);
  // make sure we listen to the run start topic starting from the run start
  // message we already got the start time from
  m_eventStream->seek(m_runInfoTopic, 0, runStartData.runStartMsgOffset);
}

int64_t KafkaEventStreamDecoder::nanosecondsToMilliseconds(
    uint64_t timeNanoseconds) const {
  return static_cast<int64_t>(timeNanoseconds / 1000000);
}

/**
 * Stop capturing from the stream. This is a blocking call until the capturing
 * function has completed
 */
void KafkaEventStreamDecoder::stopCapture() noexcept {
  // This will interrupt the "event" loop
  m_interrupt = true;
  // Wait until the function has completed. The background thread
  // will exit automatically
  while (m_capturing) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  };
}

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

/**
 * Check for an exception thrown by the background thread and rethrow
 * it if necessary. If no error occurred swap the current internal buffer
 * for a fresh one and return the old buffer.
 * @return A pointer to the data collected since the last call to this
 * method
 */
API::Workspace_sptr KafkaEventStreamDecoder::extractData() {
  if (m_exception) {
    throw std::runtime_error(*m_exception);
  }

  m_extractWaiting = true;
  m_cv.notify_one();

  auto workspace_ptr = extractDataImpl();

  m_extractWaiting = false;
  m_cv.notify_one();

  return workspace_ptr;
}

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------

API::Workspace_sptr KafkaEventStreamDecoder::extractDataImpl() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_localEvents.size() == 1) {
    auto temp = createBufferWorkspace(m_localEvents.front());
    std::swap(m_localEvents.front(), temp);
    return temp;
  } else if (m_localEvents.size() > 1) {
    auto group = boost::make_shared<API::WorkspaceGroup>();
    size_t index(0);
    for (auto &filledBuffer : m_localEvents) {
      auto temp = createBufferWorkspace(filledBuffer);
      std::swap(m_localEvents[index++], temp);
      group->addWorkspace(temp);
    }
    return group;
  } else {
    throw Exception::NotYet("Local buffers not initialized.");
  }
}

/**
 * Start decoding data from the streams into the internal buffers.
 * Implementation designed to be entry point for new thread of execution.
 * It catches all thrown exceptions.
 */
void KafkaEventStreamDecoder::captureImpl() noexcept {
  m_capturing = true;
  try {
    captureImplExcept();
  } catch (std::exception &exc) {
    m_cbError();
    m_exception = boost::make_shared<std::runtime_error>(exc.what());
  } catch (...) {
    m_cbError();
    m_exception = boost::make_shared<std::runtime_error>(
        "KafkaEventStreamDecoder: Unknown exception type caught.");
  }
  m_capturing = false;
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
    m_eventStream->consumeMessage(&buffer, offset, partition, topicName);
    // No events, wait for some to come along...
    if (buffer.empty()) {
      m_cbIterationEnd();
      continue;
    }

    if (checkOffsets) {
      if (reachedEnd.count(topicName) &&
          offset >= stopOffsets[topicName][static_cast<size_t>(partition)]) {

        reachedEnd[topicName][static_cast<size_t>(partition)] = true;

        if (offset == stopOffsets[topicName][static_cast<size_t>(partition)]) {
          g_log.debug() << "Reached end-of-run in " << topicName << " topic."
                        << std::endl;
          g_log.debug()
              << "topic: " << topicName << " offset: " << offset
              << " stopOffset: "
              << stopOffsets[topicName][static_cast<size_t>(partition)]
              << std::endl;
        }
        checkIfAllStopOffsetsReached(reachedEnd, checkOffsets);

        if (offset > stopOffsets[topicName][static_cast<size_t>(partition)]) {
          // If the offset is beyond the end of the current run, then skip to
          // the next iteration and don't process the message
          m_cbIterationEnd();
          continue;
        }
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
    else if (flatbuffers::BufferHasIdentifier(
                 reinterpret_cast<const uint8_t *>(buffer.c_str()),
                 RUN_MESSAGE_ID.c_str())) {
      auto runMsg =
          GetRunInfo(reinterpret_cast<const uint8_t *>(buffer.c_str()));
      if (!checkOffsets && runMsg->info_type_type() == InfoTypes_RunStop) {
        auto runStopMsg = static_cast<const RunStop *>(runMsg->info_type());
        auto stopTime = runStopMsg->stop_time();
        g_log.debug() << "Received an end-of-run message with stop time = "
                      << stopTime << std::endl;
        stopOffsets = getStopOffsets(stopOffsets, reachedEnd, stopTime);
        checkOffsets = true;
        checkIfAllStopOffsetsReached(reachedEnd, checkOffsets);
      }
    }
    m_cbIterationEnd();
  }
  g_log.debug("Event capture finished");
}

/**
 * Check if we've reached the stop offset on every partition of every topic
 *
 * @param reachedEnd : Bool for each topic and partition to mark when stop
 * offset reached
 */
void KafkaEventStreamDecoder::checkIfAllStopOffsetsReached(
    const std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
    bool &checkOffsets) {

  if (std::all_of(reachedEnd.cbegin(), reachedEnd.cend(),
                  [](std::pair<std::string, std::vector<bool>> kv) {
                    return std::all_of(
                        kv.second.cbegin(), kv.second.cend(),
                        [](bool partitionEnd) { return partitionEnd; });
                  }) ||
      reachedEnd.empty()) {
    m_endRun = true;
    // If we've reached the end of a run then set m_extractWaiting to true
    // so that we wait until the buffer is emptied before continuing.
    // Otherwise we can end up with data from two different runs in the
    // same buffer workspace which is problematic if the user wanted the
    // "Stop" or "Rename" run transition option.
    m_extractedEndRunData = false;
    checkOffsets = false;
    g_log.notice("Reached end of run in data streams.");
  }
}

std::unordered_map<std::string, std::vector<int64_t>>
KafkaEventStreamDecoder::getStopOffsets(
    std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
    std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
    uint64_t stopTime) const {
  reachedEnd.clear();
  stopOffsets.clear();
  // Wait for max latency so that we don't miss any late messages
  std::this_thread::sleep_for(MAX_LATENCY);
  stopOffsets = m_eventStream->getOffsetsForTimestamp(
      static_cast<int64_t>(stopTime / 1000000));
  // /1000000 to convert nanosecond precision from message to millisecond
  // precision which Kafka offset query supports

  auto currentOffsets = m_eventStream->getCurrentOffsets();

  // Set reachedEnd to false for each topic and partition
  for (auto &topicOffsets : stopOffsets) {
    auto topicName = topicOffsets.first;
    // Ignore the runInfo topic
    if (topicName.substr(topicName.length() -
                         KafkaTopicSubscriber::RUN_TOPIC_SUFFIX.length()) !=
        KafkaTopicSubscriber::RUN_TOPIC_SUFFIX) {
      g_log.debug() << "TOPIC: " << topicName
                    << " PARTITIONS: " << topicOffsets.second.size()
                    << std::endl;
      reachedEnd.insert(
          {topicName, std::vector<bool>(topicOffsets.second.size(), false)});

      auto &partitionOffsets = topicOffsets.second;
      for (uint32_t partitionNumber = 0;
           partitionNumber < partitionOffsets.size(); partitionNumber++) {
        auto offset = partitionOffsets[partitionNumber];
        // If the stop offset is negative then there are no messages for us
        // to collect on this topic, so mark reachedEnd as true already
        reachedEnd[topicName][partitionNumber] = offset < 0;
        // If the stop offset has already been reached then mark reachedEnd as
        // true
        if (currentOffsets[topicName][partitionNumber] >= offset)
          reachedEnd[topicName][partitionNumber] = true;
      }
    }
  }
  return stopOffsets;
}

/**
 * If extractData method is waiting for access to the buffer workspace
 * then we wait for it to finish
 */
void KafkaEventStreamDecoder::waitForDataExtraction() {
  {
    std::unique_lock<std::mutex> readyLock(m_waitMutex);
    m_cv.wait(readyLock, [&] { return !m_extractWaiting; });
  }
}

void KafkaEventStreamDecoder::waitForRunEndObservation() {
  m_extractWaiting = true;
  // Mark extractedEndRunData true before waiting on the extraction to ensure
  // an immediate request for run status after extracting the data will return
  // the correct value - avoids race condition in MonitorLiveData and tests
  m_extractedEndRunData = true;
  waitForDataExtraction();

  // Wait until MonitorLiveData has seen that end of run was
  // reached before setting m_endRun back to false and continuing
  std::unique_lock<std::mutex> runStatusLock(m_runStatusMutex);
  m_cvRunStatus.wait(runStatusLock, [&] { return m_runStatusSeen; });
  m_endRun = false;
  m_runStatusSeen = false;
  runStatusLock.unlock();

  // Set to zero until we have the new run number, MonitorLiveData will
  // queries before each time it extracts data until it gets non-zero
  m_runNumber = 0;

  // Get new run message now so that new run number is available for
  // MonitorLiveData as early as possible
  RunStartStruct runStartStruct;
  if (waitForNewRunStartMessage(runStartStruct))
    return;

  // Give time for MonitorLiveData to act on runStatus information
  // and trigger m_interrupt for next loop iteration if user requested
  // LiveData algorithm to stop at the end of the run
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  if (m_interrupt)
    return;

  // Rejoin event stream at start of new run
  joinEventStreamAtTime(runStartStruct);
  std::string detSpecMapMsgBuffer = getDetSpecMapForRun(runStartStruct);
  initLocalCaches(detSpecMapMsgBuffer, runStartStruct);
}

/**
 * Try to find a detector-spectrum map message published after the
 * current run start time
 *
 * @param runStartStruct details of the current run
 * @return received detector-spectrum map message buffer
 */
std::string KafkaEventStreamDecoder::getDetSpecMapForRun(
    const KafkaEventStreamDecoder::RunStartStruct &runStartStruct) {
  std::string rawMsgBuffer;
  int64_t offset;
  int32_t partition;
  std::string topicName;
  m_spDetStream = m_broker->subscribe(
      {m_spDetTopic}, nanosecondsToMilliseconds(runStartStruct.startTime),
      SubscribeAtOption::TIME);
  m_spDetStream->consumeMessage(&rawMsgBuffer, offset, partition, topicName);
  if (rawMsgBuffer.empty()) {
    std::runtime_error(
        "No detector-spectrum map message found for run number " +
        std::to_string(runStartStruct.runNumber));
  }
  return rawMsgBuffer;
}

/**
 * Wait for a run start message until we get one with a higher run number
 * than the current run or the algorithm is interrupted
 *
 * @param runStartStructOutput details of the new run
 * @return true if interrupted, false if got a new run start message
 */
bool KafkaEventStreamDecoder::waitForNewRunStartMessage(
    RunStartStruct &runStartStructOutput) {
  while (!m_interrupt) {
    std::string runMsgBuffer;

    int64_t offset;
    int32_t partition;
    std::string topicName;
    m_runStream->consumeMessage(&runMsgBuffer, offset, partition, topicName);
    if (runMsgBuffer.empty()) {
      continue; // no message available, try again
    } else {
      auto runMsg =
          GetRunInfo(reinterpret_cast<const uint8_t *>(runMsgBuffer.c_str()));
      if (runMsg->info_type_type() == InfoTypes_RunStart) {
        // We got a run start message, deserialise it
        auto runStartData = static_cast<const RunStart *>(runMsg->info_type());
        KafkaEventStreamDecoder::RunStartStruct runStartStruct = {
            runStartData->instrument_name()->str(), runStartData->run_number(),
            runStartData->start_time(),
            static_cast<size_t>(runStartData->n_periods()), offset};
        if (runStartStruct.runNumber > m_runNumber) {
          runStartStructOutput = runStartStruct;
          m_runNumber = runStartStruct.runNumber;
          return false; // not interrupted
        }
      } else {
        continue; // received message wasn't a RunStart message, try again
      }
    }
  }
  return true; // interrupted
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

KafkaEventStreamDecoder::RunStartStruct
KafkaEventStreamDecoder::getRunStartMessage(std::string &rawMsgBuffer) {
  auto offset = getRunInfoMessage(rawMsgBuffer);
  auto runMsg =
      GetRunInfo(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
  if (runMsg->info_type_type() != InfoTypes_RunStart) {
    // We want a runStart message, try the next one
    offset = getRunInfoMessage(rawMsgBuffer);
    runMsg =
        GetRunInfo(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
    if (runMsg->info_type_type() != InfoTypes_RunStart) {
      throw std::runtime_error("KafkaEventStreamDecoder::initLocalCaches() - "
                               "Could not find a run start message"
                               "in the run info topic. Unable to continue");
    }
  }
  auto runStartData = static_cast<const RunStart *>(runMsg->info_type());
  KafkaEventStreamDecoder::RunStartStruct runStart = {
      runStartData->instrument_name()->str(), runStartData->run_number(),
      runStartData->start_time(),
      static_cast<size_t>(runStartData->n_periods()), offset};
  return runStart;
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
  auto nspec = spDetMsg->spectrum()->size();
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
  auto eventBuffer = createBufferWorkspace(
      static_cast<size_t>(spDetMsg->n_spectra()), spDetMsg->spectrum()->data(),
      spDetMsg->detector_id()->data(), nudet);

  // Load the instrument if possible but continue if we can't
  auto instName = runStartData.instrumentName;
  if (!instName.empty())
    loadInstrument(instName, eventBuffer);
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

/**
 * Try to get a runInfo message from Kafka, throw error if it fails
 * @param rawMsgBuffer : string to use as message buffer
 */
int64_t KafkaEventStreamDecoder::getRunInfoMessage(std::string &rawMsgBuffer) {
  int64_t offset;
  int32_t partition;
  std::string topicName;
  m_runStream->consumeMessage(&rawMsgBuffer, offset, partition, topicName);
  if (rawMsgBuffer.empty()) {
    throw std::runtime_error("KafkaEventStreamDecoder::getRunInfoMessage() - "
                             "Empty message received from run info "
                             "topic. Unable to continue");
  }
  if (!flatbuffers::BufferHasIdentifier(
          reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()),
          RUN_MESSAGE_ID.c_str())) {
    throw std::runtime_error("KafkaEventStreamDecoder::getRunInfoMessage() - "
                             "Received unexpected message type from run info "
                             "topic. Unable to continue");
  }
  return offset;
}

/**
 * Create a buffer workspace of the correct size based on the values given.
 * @param nspectra The number of unique spectrum numbers
 * @param spec An array of length ndet specifying the spectrum number of each
 * detector
 * @param udet An array of length ndet specifying the detector ID of each
 * detector
 * @param length The length of the spec/udet arrays
 * @return A new workspace of the appropriate size
 */
DataObjects::EventWorkspace_sptr KafkaEventStreamDecoder::createBufferWorkspace(
    const size_t nspectra, const int32_t *spec, const int32_t *udet,
    const uint32_t length) {
  // Order is important here
  std::map<int32_t, std::set<int32_t>> spdetMap;
  for (uint32_t i = 0; i < length; ++i) {
    auto specNo = spec[i];
    auto detId = udet[i];
    auto search = spdetMap.find(specNo);
    if (search != spdetMap.end()) {
      search->second.insert(detId);
    } else {
      spdetMap.insert({specNo, {detId}});
    }
  }
  assert(nspectra == spdetMap.size());

  // Create event workspace
  auto eventBuffer = boost::static_pointer_cast<DataObjects::EventWorkspace>(
      API::WorkspaceFactory::Instance().create("EventWorkspace", nspectra, 2,
                                               1));
  // Set the units
  eventBuffer->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("TOF");
  eventBuffer->setYUnit("Counts");
  // Setup spectra-detector mapping.
  size_t wsIdx(0);
  for (const auto &spIter : spdetMap) {
    auto &spectrum = eventBuffer->getSpectrum(wsIdx);
    spectrum.setSpectrumNo(spIter.first);
    spectrum.addDetectorIDs(spIter.second);
    ++wsIdx;
  }
  return eventBuffer;
}

/**
 * Create new buffer workspace from an existing copy
 * @param parent A pointer to an existing workspace
 */
DataObjects::EventWorkspace_sptr KafkaEventStreamDecoder::createBufferWorkspace(
    const DataObjects::EventWorkspace_sptr &parent) {
  auto buffer = boost::static_pointer_cast<DataObjects::EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", parent->getNumberHistograms(), 2, 1));
  // Copy meta data
  API::WorkspaceFactory::Instance().initializeFromParent(*parent, *buffer,
                                                         false);
  // Clear out the old logs, except for the most recent entry
  buffer->mutableRun().clearOutdatedTimeSeriesLogValues();
  return buffer;
}

/**
 * Run LoadInstrument for the given instrument name. If it cannot succeed it
 * does nothing to the internal workspace
 * @param name Name of an instrument to load
 * @param workspace A pointer to the workspace receiving the instrument
 */
void KafkaEventStreamDecoder::loadInstrument(
    const std::string &name, DataObjects::EventWorkspace_sptr workspace) {
  if (name.empty()) {
    g_log.warning("Empty instrument name found");
    return;
  }
  try {
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
    // Do not put the workspace in the ADS
    alg->setChild(true);
    alg->initialize();
    alg->setPropertyValue("InstrumentName", name);
    alg->setProperty("Workspace", workspace);
    alg->setProperty("RewriteSpectraMap", Kernel::OptionalBool(false));
    alg->execute();
  } catch (std::exception &exc) {
    g_log.warning() << "Error loading instrument '" << name
                    << "': " << exc.what() << "\n";
  }
}
} // namespace LiveData

} // namespace Mantid
