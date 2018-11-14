// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.tcc"
#include "MantidKernel/Logger.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/ba57_run_info_generated.h"
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/f142_logdata_generated.h"
GNU_DIAG_ON("conversion")

using namespace Mantid::Types;
using namespace LogSchema;

namespace {
/// Logger
Mantid::Kernel::Logger g_log("IKafkaStreamDecoder");

// File identifiers from flatbuffers schema
const std::string RUN_MESSAGE_ID = "ba57";

const std::chrono::seconds MAX_LATENCY(1);
} // namespace

namespace Mantid {
namespace LiveData {
// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------
/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param streamTopic The name of the topic streaming the stream data
 * @param runInfoTopic The name of the topic streaming the run information
 * @param spDetTopic The name of the topic streaming the spectrum-detector
 * @param sampleEnvTopic The name of the topic stream sample environment
 * information. run mapping
 */
IKafkaStreamDecoder::IKafkaStreamDecoder(std::shared_ptr<IKafkaBroker> broker,
                                         const std::string &streamTopic,
                                         const std::string &runInfoTopic,
                                         const std::string &spDetTopic,
                                         const std::string &sampleEnvTopic)
    : m_broker(broker), m_streamTopic(streamTopic),
      m_runInfoTopic(runInfoTopic), m_spDetTopic(spDetTopic),
      m_sampleEnvTopic(sampleEnvTopic), m_interrupt(false), m_specToIdx(),
      m_runStart(), m_runNumber(-1), m_thread(), m_capturing(false),
      m_exception(), m_extractWaiting(false), m_cbIterationEnd([] {}),
      m_cbError([] {}) {}

/**
 * Destructor.
 * Stops capturing from the stream
 */
IKafkaStreamDecoder::~IKafkaStreamDecoder() { stopCapture(); }

/**
 * Start capturing from the stream on a separate thread. This is a non-blocking
 * call and will return after the thread has started
 */
void IKafkaStreamDecoder::startCapture(bool startNow) {

  // If we are not starting now, then we want to start at the start of the run
  if (!startNow) {
    // Get last two messages in run topic to ensure we get a runStart message
    m_runStream =
        m_broker->subscribe({m_runInfoTopic}, SubscribeAtOption::LASTTWO);
    std::string rawMsgBuffer;
    auto runStartData = getRunStartMessage(rawMsgBuffer);
    joinStreamAtTime(runStartData);
  } else {
    m_dataStream =
        m_broker->subscribe({m_streamTopic, m_runInfoTopic, m_sampleEnvTopic},
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
bool IKafkaStreamDecoder::dataReset() {
  bool result = (m_dataReset == true); // copy from atomic bool
  m_dataReset = false;                 // reset to false
  return result;
}

void IKafkaStreamDecoder::joinStreamAtTime(
    const IKafkaStreamDecoder::RunStartStruct &runStartData) {
  auto runStartTime = runStartData.startTime;
  int64_t startTimeMilliseconds = nanosecondsToMilliseconds(runStartTime);
  m_dataStream =
      m_broker->subscribe({m_streamTopic, m_runInfoTopic, m_sampleEnvTopic},
                          startTimeMilliseconds, SubscribeAtOption::TIME);
  // make sure we listen to the run start topic starting from the run start
  // message we already got the start time from
  m_dataStream->seek(m_runInfoTopic, 0, runStartData.runStartMsgOffset);
}

int64_t
IKafkaStreamDecoder::nanosecondsToMilliseconds(uint64_t timeNanoseconds) const {
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
  };
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

  m_extractWaiting = true;
  m_cv.notify_one();

  auto workspace_ptr = extractDataImpl();

  m_extractWaiting = false;
  m_cv.notify_one();

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
    m_exception = boost::make_shared<std::runtime_error>(exc.what());
  } catch (...) {
    m_cbError();
    m_exception = boost::make_shared<std::runtime_error>(
        "IKafkaStreamDecoder: Unknown exception type caught.");
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
IKafkaStreamDecoder::getStopOffsets(
    std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
    std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
    uint64_t stopTime) const {
  reachedEnd.clear();
  stopOffsets.clear();
  // Wait for max latency so that we don't miss any late messages
  std::this_thread::sleep_for(MAX_LATENCY);
  stopOffsets = m_dataStream->getOffsetsForTimestamp(
      static_cast<int64_t>(stopTime / 1000000));
  // /1000000 to convert nanosecond precision from message to millisecond
  // precision which Kafka offset query supports

  auto currentOffsets = m_dataStream->getCurrentOffsets();

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
void IKafkaStreamDecoder::waitForDataExtraction() {
  {
    std::unique_lock<std::mutex> readyLock(m_waitMutex);
    m_cv.wait(readyLock, [&] { return !m_extractWaiting; });
  }
}

void IKafkaStreamDecoder::waitForRunEndObservation() {
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
  joinStreamAtTime(runStartStruct);
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
std::string IKafkaStreamDecoder::getDetSpecMapForRun(
    const IKafkaStreamDecoder::RunStartStruct &runStartStruct) {
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
bool IKafkaStreamDecoder::waitForNewRunStartMessage(
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
        IKafkaStreamDecoder::RunStartStruct runStartStruct = {
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

IKafkaStreamDecoder::RunStartStruct
IKafkaStreamDecoder::getRunStartMessage(std::string &rawMsgBuffer) {
  auto offset = getRunInfoMessage(rawMsgBuffer);
  auto runMsg =
      GetRunInfo(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
  if (runMsg->info_type_type() != InfoTypes_RunStart) {
    // We want a runStart message, try the next one
    offset = getRunInfoMessage(rawMsgBuffer);
    runMsg =
        GetRunInfo(reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
    if (runMsg->info_type_type() != InfoTypes_RunStart) {
      throw std::runtime_error("IKafkaStreamDecoder::initLocalCaches() - "
                               "Could not find a run start message"
                               "in the run info topic. Unable to continue");
    }
  }
  auto runStartData = static_cast<const RunStart *>(runMsg->info_type());
  IKafkaStreamDecoder::RunStartStruct runStart = {
      runStartData->instrument_name()->str(), runStartData->run_number(),
      runStartData->start_time(),
      static_cast<size_t>(runStartData->n_periods()), offset};
  return runStart;
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
  if (!flatbuffers::BufferHasIdentifier(
          reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()),
          RUN_MESSAGE_ID.c_str())) {
    throw std::runtime_error("IKafkaStreamDecoder::getRunInfoMessage() - "
                             "Received unexpected message type from run info "
                             "topic. Unable to continue");
  }
  return offset;
}

std::map<int32_t, std::set<int32_t>>
IKafkaStreamDecoder::buildSpectrumToDetectorMap(const int32_t *spec,
                                                const int32_t *udet,
                                                uint32_t length) {
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

void IKafkaStreamDecoder::checkRunMessage(
    const std::string &buffer, bool &checkOffsets,
    std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
    std::unordered_map<std::string, std::vector<bool>> &reachedEnd) {
  if (flatbuffers::BufferHasIdentifier(
          reinterpret_cast<const uint8_t *>(buffer.c_str()),
          RUN_MESSAGE_ID.c_str())) {
    auto runMsg = GetRunInfo(reinterpret_cast<const uint8_t *>(buffer.c_str()));
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
}

void IKafkaStreamDecoder::checkRunEnd(
    const std::string &topicName, bool &checkOffsets, const int64_t offset,
    const int32_t partition,
    std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
    std::unordered_map<std::string, std::vector<bool>> &reachedEnd) {
  if (reachedEnd.count(topicName) &&
      offset >= stopOffsets[topicName][static_cast<size_t>(partition)]) {

    reachedEnd[topicName][static_cast<size_t>(partition)] = true;

    if (offset == stopOffsets[topicName][static_cast<size_t>(partition)]) {
      g_log.debug() << "Reached end-of-run in " << topicName << " topic."
                    << std::endl;
      g_log.debug() << "topic: " << topicName << " offset: " << offset
                    << " stopOffset: "
                    << stopOffsets[topicName][static_cast<size_t>(partition)]
                    << std::endl;
    }
    checkIfAllStopOffsetsReached(reachedEnd, checkOffsets);
  }
}

} // namespace LiveData

} // namespace Mantid
