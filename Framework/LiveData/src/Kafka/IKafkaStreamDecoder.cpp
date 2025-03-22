// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.hxx"
#include "MantidAPI/Run.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/6s4t_run_stop_generated.h"
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/f142_logdata_generated.h"
#include "private/Schema/pl72_run_start_generated.h"
GNU_DIAG_ON("conversion")

#include <json/json.h>

#include <utility>

using namespace Mantid::Types;

namespace {
/// Logger
Mantid::Kernel::Logger g_log("IKafkaStreamDecoder");

// File identifiers from flatbuffers schema
const std::string RUN_START_MESSAGE_ID = "pl72";
const std::string RUN_STOP_MESSAGE_ID = "6s4t";

const std::chrono::seconds MAX_LATENCY(1);
} // namespace

namespace Mantid::LiveData {
// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------
/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param streamTopic The name of the topic streaming the stream data
 * @param runInfoTopic The name of the topic streaming the run information
 * @param sampleEnvTopic The name of the topic stream sample environment
 * information. run mapping
 */
IKafkaStreamDecoder::IKafkaStreamDecoder(std::shared_ptr<IKafkaBroker> broker, std::string streamTopic,
                                         std::string runInfoTopic, std::string sampleEnvTopic, std::string chopperTopic,
                                         std::string monitorTopic)
    : m_broker(std::move(broker)), m_streamTopic(std::move(streamTopic)), m_runInfoTopic(std::move(runInfoTopic)),
      m_sampleEnvTopic(std::move(sampleEnvTopic)), m_chopperTopic(std::move(chopperTopic)),
      m_monitorTopic(std::move(monitorTopic)), m_interrupt(false), m_eventIdToWkspIdx(), m_runStart(), m_runId(""),
      m_thread(), m_capturing(false), m_exception(), m_extractWaiting(false), m_cbIterationEnd([] {}),
      m_cbError([] {}) {}

IKafkaStreamDecoder::IKafkaStreamDecoder(IKafkaStreamDecoder &&o) noexcept
    : m_broker(std::move(o.m_broker)), m_streamTopic(o.m_streamTopic), m_runInfoTopic(o.m_runInfoTopic),
      m_sampleEnvTopic(o.m_sampleEnvTopic), m_chopperTopic(o.m_chopperTopic), m_monitorTopic(o.m_monitorTopic),
      m_interrupt(o.m_interrupt.load()), m_eventIdToWkspIdx(std::move(o.m_eventIdToWkspIdx)), m_runStart(o.m_runStart),
      m_runId(std::move(o.m_runId)), m_thread(std::move(o.m_thread)), m_capturing(o.m_capturing.load()),
      m_exception(std::move(o.m_exception)), m_cbIterationEnd(std::move(o.m_cbIterationEnd)),
      m_cbError(std::move(o.m_cbError)) {
  std::scoped_lock lck(m_runStatusMutex, m_waitMutex, m_mutex);
  m_runStatusSeen = o.m_runStatusSeen;
  m_extractWaiting = o.m_extractWaiting.load();
}

/**
 * Destructor.
 * Stops capturing from the stream
 */
IKafkaStreamDecoder::~IKafkaStreamDecoder() { stopCapture(); }

/**
 * Start capturing from the stream on a separate thread. This is a
 * non-blocking call and will return after the thread has started
 */
void IKafkaStreamDecoder::startCapture(bool startNow) {
  // If we are not starting now, then we want to start at the start of the run
  if (!startNow) {
    // Get last two messages in run topic to ensure we get a runStart message
    m_runStream = m_broker->subscribe({m_runInfoTopic}, SubscribeAtOption::LASTTWO);
    std::string rawMsgBuffer;
    auto runStartData = getRunStartMessage(rawMsgBuffer);
    joinStreamAtTime(runStartData);
  } else {
    m_dataStream = m_broker->subscribe({m_streamTopic, m_monitorTopic, m_runInfoTopic, m_sampleEnvTopic},
                                       SubscribeAtOption::LATEST);
  }

  try {
    if (!m_chopperTopic.empty())
      m_chopperStream = m_broker->subscribe({m_chopperTopic}, SubscribeAtOption::LATEST);
  } catch (std::exception &) {
    g_log.notice() << "Could not subscribe to topic " + m_chopperTopic +
                          ". This topic does not exist. No chopper information "
                          "will be written to the logs."
                   << std::endl;
  }

  // Get last two messages in run topic to ensure we get a runStart message
  m_runStream = m_broker->subscribe({m_runInfoTopic}, SubscribeAtOption::LASTTWO);

  m_thread = std::thread([this]() { this->captureImpl(); });
  m_thread.detach();
}

/** Indicate if the next data to be extracted should replace LoadLiveData's
 * output workspace,
 *  for example the first data of a new run
 */
bool IKafkaStreamDecoder::dataReset() {
  bool result = (m_dataReset == true); // copy from atomic bool
  m_dataReset = false;                 // reset to false
  return result;
}

void IKafkaStreamDecoder::joinStreamAtTime(const IKafkaStreamDecoder::RunStartStruct &runStartData) {
  auto runStartTime = runStartData.startTime;
  int64_t startTimeMilliseconds = nanosecondsToMilliseconds(runStartTime);
  m_dataStream = m_broker->subscribe({m_streamTopic, m_runInfoTopic, m_sampleEnvTopic}, startTimeMilliseconds,
                                     SubscribeAtOption::TIME);
  // make sure we listen to the run start topic starting from the run start
  // message we already got the start time from
  m_dataStream->seek(m_runInfoTopic, 0, runStartData.runStartMsgOffset);
}

int64_t IKafkaStreamDecoder::nanosecondsToMilliseconds(uint64_t timeNanoseconds) {
  return static_cast<int64_t>(timeNanoseconds / 1000000);
}

/**
 * Stop capturing from the stream. This is a blocking call until the capturing
 * function has completed
 */
void IKafkaStreamDecoder::stopCapture() noexcept {
  // This will interrupt the "event" loop
  m_interrupt = true;
  // Wait until the function has completed. The background thread
  // will exit automatically
  while (m_capturing) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

/**
 * Check if a message has indicated that end of run has been reached
 * @return  True if end of run has been reached
 */
bool IKafkaStreamDecoder::hasReachedEndOfRun() noexcept {
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
API::Workspace_sptr IKafkaStreamDecoder::extractData() {
  if (m_exception) {
    throw std::runtime_error(*m_exception);
  }

  {
    std::lock_guard lck(m_waitMutex);
    m_extractWaiting = true;
    m_cv.notify_one();
  }

  auto workspace_ptr = extractDataImpl();

  {
    std::lock_guard lck(m_waitMutex);
    m_extractWaiting = false;
    m_cv.notify_one();
  }

  return workspace_ptr;
}

/**
 * Start decoding data from the streams into the internal buffers.
 * Implementation designed to be entry point for new thread of execution.
 * It catches all thrown exceptions.
 */
void IKafkaStreamDecoder::captureImpl() noexcept {
  m_capturing = true;
  try {
    captureImplExcept();
  } catch (std::exception &exc) {
    m_cbError();
    m_exception = std::make_shared<std::runtime_error>(exc.what());
  } catch (...) {
    m_cbError();
    m_exception = std::make_shared<std::runtime_error>("IKafkaStreamDecoder: Unknown exception type caught.");
  }
  m_capturing = false;
}

/**
 * Check if we've reached the stop offset on every partition of every topic
 *
 * @param reachedEnd : Bool for each topic and partition to mark when stop
 * offset reached
 */
void IKafkaStreamDecoder::checkIfAllStopOffsetsReached(
    const std::unordered_map<std::string, std::vector<bool>> &reachedEnd, bool &checkOffsets) {
  if (std::all_of(reachedEnd.cbegin(), reachedEnd.cend(),
                  [](const std::pair<std::string, std::vector<bool>> &kv) {
                    return std::all_of(kv.second.cbegin(), kv.second.cend(),
                                       [](bool partitionEnd) { return partitionEnd; });
                  }) ||
      reachedEnd.empty()) {
    m_endRun = true;
    // If we've reached the end of a run then set m_extractWaiting to true
    // so that we wait until the buffer is emptied before continuing.
    // Otherwise we can end up with data from two different runs in the
    // same buffer workspace which is problematic if the user wanted the
    // "Stop" or "Rename" run transition option.
    {
      std::lock_guard lck(m_waitMutex);
      m_extractedEndRunData = false;
    }
    checkOffsets = false;
    g_log.notice("Reached end of run in data streams.");
  }
}

std::unordered_map<std::string, std::vector<int64_t>>
IKafkaStreamDecoder::getStopOffsets(std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
                                    std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
                                    uint64_t stopTime) const {
  reachedEnd.clear();
  stopOffsets.clear();
  // Wait for max latency so that we don't miss any late messages
  std::this_thread::sleep_for(MAX_LATENCY);
  stopOffsets = m_dataStream->getOffsetsForTimestamp(static_cast<int64_t>(stopTime / 1000000));
  // /1000000 to convert nanosecond precision from message to millisecond
  // precision which Kafka offset query supports

  auto currentOffsets = m_dataStream->getCurrentOffsets();

  // Set reachedEnd to false for each topic and partition
  for (auto &topicOffsets : stopOffsets) {
    auto topicName = topicOffsets.first;
    // Ignore the runInfo topic
    if (topicName.substr(topicName.length() - KafkaTopicSubscriber::RUN_TOPIC_SUFFIX.length()) !=
        KafkaTopicSubscriber::RUN_TOPIC_SUFFIX) {
      g_log.debug() << "TOPIC: " << topicName << " PARTITIONS: " << topicOffsets.second.size() << std::endl;
      reachedEnd.insert({topicName, std::vector<bool>(topicOffsets.second.size(), false)});

      auto &partitionOffsets = topicOffsets.second;
      for (uint32_t partitionNumber = 0; partitionNumber < partitionOffsets.size(); partitionNumber++) {
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
void IKafkaStreamDecoder::waitForDataExtraction() {
  std::unique_lock<std::mutex> readyLock(m_waitMutex);
  if (m_extractWaiting) { // Might have extracted whilst we were grabbing
                          // mutex
    m_cv.wait(readyLock, [&] { return !m_extractWaiting; });
  }
}

void IKafkaStreamDecoder::waitForRunEndObservation() {
  {
    std::lock_guard lck(m_waitMutex);
    m_extractWaiting = true;
  }
  // Mark extractedEndRunData true before waiting on the extraction to ensure
  // an immediate request for run status after extracting the data will return
  // the correct value - avoids race condition in MonitorLiveData and tests
  m_extractedEndRunData = true;
  m_cbIterationEnd();
  waitForDataExtraction();

  // Wait until MonitorLiveData has seen that end of run was
  // reached before setting m_endRun back to false and continuing
  std::unique_lock<std::mutex> runStatusLock(m_runStatusMutex);
  m_cvRunStatus.wait(runStatusLock, [&] { return m_runStatusSeen; });
  m_endRun = false;
  m_runStatusSeen = false;
  runStatusLock.unlock();

  // Set to zero until we have the new run id, MonitorLiveData will
  // queries before each time it extracts data until it gets non-zero
  m_runId = "0";

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
  joinStreamAtTime(runStartStruct);
  initLocalCaches(runStartStruct);
}

/**
 * Wait for a run start message until we get one with a higher run number
 * than the current run or the algorithm is interrupted
 *
 * @param runStartStructOutput details of the new run
 * @return true if interrupted, false if got a new run start message
 */
bool IKafkaStreamDecoder::waitForNewRunStartMessage(RunStartStruct &runStartStructOutput) {
  while (!m_interrupt) {
    std::string runMsgBuffer;
    int64_t offset;
    int32_t partition;
    std::string topicName;
    m_runStream->consumeMessage(&runMsgBuffer, offset, partition, topicName);
    if (runMsgBuffer.empty() ||
        !flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(runMsgBuffer.c_str()),
                                          RUN_START_MESSAGE_ID.c_str())) {
      continue; // no start message available, try again
    } else {
      auto runStartStruct = extractRunStartDataFromMessage(runMsgBuffer, offset);
      if (runStartStruct.runId != m_runId) {
        runStartStructOutput = runStartStruct;
        m_runId = runStartStruct.runId;
        return false; // not interrupted
      }
    }
  }
  return true; // interrupted
}

IKafkaStreamDecoder::RunStartStruct
IKafkaStreamDecoder::extractRunStartDataFromMessage(const std::string &messageBuffer, const int64_t offset) {
  auto runStartData = GetRunStart(reinterpret_cast<const uint8_t *>(messageBuffer.c_str()));
  bool detSpecMapSpecified = false;
  size_t numberOfSpectra = 0;
  std::vector<int32_t> spectrumNumbers;
  std::vector<int32_t> detectorIDs;
  if (runStartData->detector_spectrum_map() != nullptr && runStartData->detector_spectrum_map()->n_spectra() != 0) {
    detSpecMapSpecified = true;
    auto spDetMsg = runStartData->detector_spectrum_map();
    numberOfSpectra = static_cast<size_t>(spDetMsg->n_spectra());
    auto numberOfDetectors = spDetMsg->detector_id()->size();
    if (numberOfDetectors != numberOfSpectra) {
      std::ostringstream os;
      os << "IKafkaStreamDecoder::waitForNewRunStartMessage() - Invalid "
            "spectra/detector mapping. Expected matched length arrays but "
            "found numberOfSpectra="
         << numberOfSpectra << ", numberOfDetectors=" << numberOfDetectors;
      throw std::runtime_error(os.str());
    }
    spectrumNumbers.assign(spDetMsg->spectrum()->data(), spDetMsg->spectrum()->data() + spDetMsg->spectrum()->size());
    detectorIDs.assign(spDetMsg->detector_id()->data(),
                       spDetMsg->detector_id()->data() + spDetMsg->detector_id()->size());
  }
  return {runStartData->instrument_name()->str(),
          runStartData->run_name()->str(),
          runStartData->start_time(),
          static_cast<size_t>(runStartData->n_periods()),
          runStartData->nexus_structure()->str(),
          offset,
          detSpecMapSpecified,
          numberOfSpectra,
          spectrumNumbers,
          detectorIDs};
}

IKafkaStreamDecoder::RunStartStruct IKafkaStreamDecoder::getRunStartMessage(std::string &rawMsgBuffer) {
  auto offset = getRunInfoMessage(rawMsgBuffer);
  // If the first message is not a run start message then get another message
  if (!flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()),
                                        RUN_START_MESSAGE_ID.c_str())) {
    offset = getRunInfoMessage(rawMsgBuffer);

    // If the second message is not a run start then give up
    if (!flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()),
                                          RUN_START_MESSAGE_ID.c_str())) {
      throw std::runtime_error("IKafkaStreamDecoder::getRunStartMessage() - "
                               "Didn't find a run start message in the run info "
                               "topic. Unable to continue");
    }
  }
  return extractRunStartDataFromMessage(rawMsgBuffer, offset);
}

/**
 * Try to get a runInfo message from Kafka, throw error if it fails
 * @param rawMsgBuffer : string to use as message buffer
 */
int64_t IKafkaStreamDecoder::getRunInfoMessage(std::string &rawMsgBuffer) {
  int64_t offset;
  int32_t partition;
  std::string topicName;
  m_runStream->consumeMessage(&rawMsgBuffer, offset, partition, topicName);
  if (rawMsgBuffer.empty()) {
    throw std::runtime_error("IKafkaStreamDecoder::getRunInfoMessage() - "
                             "Empty message received from run info "
                             "topic. Unable to continue");
  }
  if (!flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()),
                                        RUN_START_MESSAGE_ID.c_str()) &&
      !flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()),
                                        RUN_STOP_MESSAGE_ID.c_str())) {
    g_log.error() << "Message with flatbuffer ID of " << rawMsgBuffer.substr(4, 8) << " in run info topic.\n";
    throw std::runtime_error("IKafkaStreamDecoder::getRunInfoMessage() - "
                             "Received unexpected message type from run info "
                             "topic. Unable to continue");
  }
  return offset;
}

std::map<int32_t, std::set<int32_t>>
IKafkaStreamDecoder::buildSpectrumToDetectorMap(const int32_t *spec, const int32_t *udet, uint32_t length) {
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

  return spdetMap;
}

void IKafkaStreamDecoder::checkRunMessage(const std::string &buffer, bool &checkOffsets,
                                          std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
                                          std::unordered_map<std::string, std::vector<bool>> &reachedEnd) {
  if (flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(buffer.c_str()),
                                       RUN_STOP_MESSAGE_ID.c_str())) {
    auto runStopMsg = GetRunStop(reinterpret_cast<const uint8_t *>(buffer.c_str()));
    if (!checkOffsets) {
      auto stopTime = runStopMsg->stop_time();
      g_log.debug() << "Received an end-of-run message with stop time = " << stopTime << std::endl;
      stopOffsets = getStopOffsets(stopOffsets, reachedEnd, stopTime);
      checkOffsets = true;
      checkIfAllStopOffsetsReached(reachedEnd, checkOffsets);
    }
  }
}

void IKafkaStreamDecoder::checkRunEnd(const std::string &topicName, bool &checkOffsets, const int64_t offset,
                                      const int32_t partition,
                                      std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
                                      std::unordered_map<std::string, std::vector<bool>> &reachedEnd) {
  if (reachedEnd.count(topicName) && offset >= stopOffsets[topicName][static_cast<size_t>(partition)]) {

    reachedEnd[topicName][static_cast<size_t>(partition)] = true;

    if (offset == stopOffsets[topicName][static_cast<size_t>(partition)]) {
      g_log.debug() << "Reached end-of-run in " << topicName << " topic." << std::endl;
      g_log.debug() << "topic: " << topicName << " offset: " << offset
                    << " stopOffset: " << stopOffsets[topicName][static_cast<size_t>(partition)] << std::endl;
    }
    checkIfAllStopOffsetsReached(reachedEnd, checkOffsets);
  }
}

int IKafkaStreamDecoder::runNumber() const noexcept {
  // If the run ID is empty or if any non-digit char was found in the string
  if (m_runId.empty() || (std::find_if_not(m_runId.cbegin(), m_runId.cend(),
                                           [](const unsigned char c) { return std::isdigit(c); }) != m_runId.end()))
    return -1;

  return std::atoi(m_runId.c_str());
}
} // namespace Mantid::LiveData
