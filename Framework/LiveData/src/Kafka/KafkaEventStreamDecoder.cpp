// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"

#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.hxx"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/ev42_events_generated.h"
#include "private/Schema/f142_logdata_generated.h"
#include "private/Schema/is84_isis_events_generated.h"
GNU_DIAG_ON("conversion")

#include <chrono>
#include <json/json.h>
#include <numeric>
#include <utility>

#include <tbb/parallel_sort.h>

using namespace Mantid::Types;
using Mantid::Kernel::ConfigService;

// Counters
size_t totalNumEventsSinceStart = 0;
size_t totalNumEventsBeforeLastTimeout = 0;
double totalPopulateWorkspaceDuration = 0;
double numPopulateWorkspaceCalls = 0;
double totalEventFromMessageDuration = 0;
double numEventFromMessageCalls = 0;

using namespace LogSchema;

namespace {
/// Logger
Mantid::Kernel::Logger g_log("KafkaEventStreamDecoder");

const std::string PROTON_CHARGE_PROPERTY = "proton_charge";
const std::string RUN_NUMBER_PROPERTY = "run_number";
const std::string RUN_START_PROPERTY = "run_start";

// File identifiers from flatbuffers schema
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
void appendToLog(Mantid::API::Run &mutableRunInfo, const std::string &name, const Core::DateAndTime &time, T value) {
  if (mutableRunInfo.hasProperty(name)) {
    auto property = mutableRunInfo.getTimeSeriesProperty<T>(name);
    property->addValue(time, value);
  } else {
    auto property = new Mantid::Kernel::TimeSeriesProperty<T>(name);
    property->addValue(time, value);
    mutableRunInfo.addLogData(property);
  }
}

void sortIntermediateEventBuffer(
    std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent> &eventBuffer,
    const std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedPulse> &pulseBuffer) {
  tbb::parallel_sort(eventBuffer.begin(), eventBuffer.end(),
                     [&](const Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent &lhs,
                         const Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent &rhs) -> bool {
                       const auto &lhsPulse = pulseBuffer[lhs.pulseIndex];
                       const auto &rhsPulse = pulseBuffer[rhs.pulseIndex];

                       /* If events are from different periods compare the period
                        * numbers, otherwise compare the workspace index */
                       return (lhsPulse.periodNumber != rhsPulse.periodNumber)
                                  ? lhsPulse.periodNumber < rhsPulse.periodNumber
                                  : lhs.wsIdx < rhs.wsIdx;
                     });
}
} // namespace

namespace Mantid::LiveData {
using Types::Core::DateAndTime;
using Types::Event::TofEvent;

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------
/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param eventTopic The name of the topic streaming the event data
 * run mapping
 */
KafkaEventStreamDecoder::KafkaEventStreamDecoder(std::shared_ptr<IKafkaBroker> broker, const std::string &eventTopic,
                                                 const std::string &runInfoTopic, const std::string &sampleEnvTopic,
                                                 const std::string &chopperTopic, const std::string &monitorTopic,
                                                 const std::size_t bufferThreshold)
    : IKafkaStreamDecoder(std::move(broker), eventTopic, runInfoTopic, sampleEnvTopic, chopperTopic, monitorTopic),
      m_intermediateBufferFlushThreshold(bufferThreshold) {
#ifndef _OPENMP
  g_log.warning() << "Multithreading is not available on your system. This "
                     "is likely to be an issue with high event counts.\n";
#endif
}

/**
 * Destructor.
 * Stops capturing from the stream
 */
KafkaEventStreamDecoder::~KafkaEventStreamDecoder() {
  /*
   * IKafkaStreamDecoder::stopCapture is called again here as capture must be
   * terminated before the KafkaEventStreamDecoder is destruced.
   *
   * Without this, a race condition occurs (over threads 1 and 2):
   *  - KafkaEventStreamDecoder::~KafkaEventStreamDecoder is called and members
   *    immediately destructed [1]
   *  - IKafkaStreamDecoder::~IKafkaStreamDecoder is called [1]
   *    - IKafkaStreamDecoder::m_interrupt set [on 1, visible to 2]
   *    - Capture loop iteration completes and calls
   *      KafkaEventStreamDecoder::flushIntermediateBuffer [2]
   *      (this function attempts to access already deleted members of
   *      KafkaEventStreamDecoder)
   *
   * By calling IKafkaStreamDecoder::stopCapture here it ensures that the
   * capture has fully completed before local state is deleted.
   */
  stopCapture();
}

KafkaEventStreamDecoder::KafkaEventStreamDecoder(KafkaEventStreamDecoder &&o) noexcept
    : IKafkaStreamDecoder(std::move(o)), m_intermediateBufferFlushThreshold(o.m_intermediateBufferFlushThreshold) {

  std::scoped_lock lck(m_intermediateBufferMutex, m_mutex);
  m_localEvents = std::move(o.m_localEvents);
  m_receivedEventBuffer = std::move(o.m_receivedEventBuffer);
  m_receivedPulseBuffer = std::move(o.m_receivedPulseBuffer);
}

/**
 * Check if there is data available to extract
 * @return True if data has been accumulated so that extractData()
 * can be called, false otherwise
 */
bool KafkaEventStreamDecoder::hasData() const noexcept {
  std::lock_guard<std::mutex> workspaceLock(m_mutex);
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
  std::lock_guard<std::mutex> workspaceLock(m_mutex);
  g_log.debug() << "Events since last timeout " << totalNumEventsSinceStart - totalNumEventsBeforeLastTimeout
                << std::endl;
  totalNumEventsBeforeLastTimeout = totalNumEventsSinceStart;

  if (m_localEvents.size() == 1) {
    auto temp = createBufferWorkspace<DataObjects::EventWorkspace>("EventWorkspace", m_localEvents.front());
    std::swap(m_localEvents.front(), temp);
    return temp;
  } else if (m_localEvents.size() > 1) {
    auto group = std::make_shared<API::WorkspaceGroup>();
    size_t index(0);
    for (auto &filledBuffer : m_localEvents) {
      auto temp = createBufferWorkspace<DataObjects::EventWorkspace>("EventWorkspace", filledBuffer);
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

  // Load runstart struct then initialise the cache
  std::string buffer;
  std::string runBuffer;
  int64_t offset;
  int32_t partition;
  std::string topicName;
  auto runStartStruct = getRunStartMessage(runBuffer);
  initLocalCaches(runStartStruct);

  m_interrupt = false;     // Allow MonitorLiveData or user to interrupt
  m_endRun = false;        // Indicates to MonitorLiveData that end of run is reached
  m_runStatusSeen = false; // Flag to ensure MonitorLiveData observes end of run
  // Flag to ensure LoadLiveData extracts data before start of next run
  m_extractedEndRunData = true;

  // Keep track of whether we've reached the end of a run
  std::unordered_map<std::string, std::vector<int64_t>> stopOffsets;
  std::unordered_map<std::string, std::vector<bool>> reachedEnd;
  bool checkOffsets = false;

  size_t nEvents = 0;
  size_t nMessages = 0;
  size_t totalMessages = 0;
  size_t eventsPerMessage = 0;
  size_t lastMessageEvents = 0;
  uint64_t lastPulseTime = 0;
  size_t messagesPerPulse = 0;
  size_t numMessagesForSinglePulse = 0;
  size_t pulseTimeCount = 0;
  auto globstart = std::chrono::system_clock::now();
  auto start = std::chrono::system_clock::now();

  while (!m_interrupt) {
    if (m_endRun) {
      /* Ensure the intermediate buffer is flushed so as to prevent
       * EventWorksapces containing events from other runs. */
      flushIntermediateBuffer();

      waitForRunEndObservation();
      continue;
    } else {
      waitForDataExtraction();
    }

    // Pull in events
    m_dataStream->consumeMessage(&buffer, offset, partition, topicName);
    // No events, wait for some to come along...
    if (buffer.empty()) {
      start = std::chrono::system_clock::now();
      globstart = std::chrono::system_clock::now();
      g_log.notice() << "Waiting to start..." << std::endl;
      m_cbIterationEnd();
      continue;
    }

    writeChopperTimestampsToWorkspaceLogs(m_localEvents);

    const auto end = std::chrono::system_clock::now();
    const std::chrono::duration<double> dur = end - start;
    if (dur.count() >= 60) {
      g_log.debug() << "Message count " << nMessages << '\n';
      const auto rate = static_cast<double>(nMessages) / dur.count();
      g_log.debug() << "Consuming " << rate << "Hz\n";
      nMessages = 0;
      g_log.debug() << eventsPerMessage << " events per message\n";
      const auto mpp = static_cast<double>(numMessagesForSinglePulse) / static_cast<double>(pulseTimeCount);
      g_log.debug() << mpp << " event messages per pulse\n";
      g_log.debug() << "Achievable pulse rate is " << rate / mpp << "Hz\n";
      g_log.debug() << "Average time taken to convert event messages "
                    << totalEventFromMessageDuration / numEventFromMessageCalls << " seconds\n";
      g_log.debug() << "Average time taken to populate workspace "
                    << totalPopulateWorkspaceDuration / numPopulateWorkspaceCalls << " seconds\n";
      start = std::chrono::system_clock::now();
    }

    if (checkOffsets) {
      checkRunEnd(topicName, checkOffsets, offset, partition, stopOffsets, reachedEnd);
      if (offset > stopOffsets[topicName][static_cast<size_t>(partition)]) {
        // If the offset is beyond the end of the current run, then skip to
        // the next iteration and don't process the message
        m_cbIterationEnd();
        continue;
      }
    }

    // Check if we have an event message
    // Most will be event messages so we check for this type first
    if (flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(buffer.c_str()), EVENT_MESSAGE_ID.c_str())) {
      uint64_t currentPulseTime(-1);
      eventDataFromMessage(buffer, nEvents, currentPulseTime);

      if (lastPulseTime == 0)
        lastPulseTime = currentPulseTime;
      else if (lastPulseTime != currentPulseTime) {
        ++pulseTimeCount;
        lastPulseTime = currentPulseTime;
        numMessagesForSinglePulse += messagesPerPulse;
        messagesPerPulse = 0;
      }

      ++messagesPerPulse;

      eventsPerMessage = nEvents - lastMessageEvents;
      lastMessageEvents = nEvents;

      /* If there are enough events in the receive buffer then empty it into
       * the EventWorkspace(s) */
      if (m_receivedEventBuffer.size() > m_intermediateBufferFlushThreshold) {
        flushIntermediateBuffer();
      }

      totalNumEventsSinceStart = nEvents;
      ++nMessages;
      ++totalMessages;
    }
    // Check if we have a sample environment log message
    else if (flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(buffer.c_str()),
                                              SAMPLE_MESSAGE_ID.c_str())) {
      sampleDataFromMessage(buffer);
    }
    // Check if we have a runMessage
    else
      checkRunMessage(buffer, checkOffsets, stopOffsets, reachedEnd);
    m_cbIterationEnd();
  }

  /* Flush any remaining events when capture is terminated */
  flushIntermediateBuffer();

  const auto globend = std::chrono::system_clock::now();
  const std::chrono::duration<double> dur = globend - globstart;
  g_log.debug() << "Consumed at a rate of " << static_cast<double>(totalMessages) / dur.count() << "Hz" << std::endl;
  g_log.debug("Event capture finished");
  totalNumEventsBeforeLastTimeout = 0;
  totalNumEventsSinceStart = 0;
  totalNumEventsSinceStart = 0;
  totalNumEventsBeforeLastTimeout = 0;
  totalPopulateWorkspaceDuration = 0;
  numPopulateWorkspaceCalls = 0;
  totalEventFromMessageDuration = 0;
  numEventFromMessageCalls = 0;
}

void KafkaEventStreamDecoder::eventDataFromMessage(const std::string &buffer, size_t &eventCount,
                                                   uint64_t &pulseTimeRet) {
  /* Parse message */
  const auto eventMsg = GetEventMessage(reinterpret_cast<const uint8_t *>(buffer.c_str()));

  /* Parse pulse time */
  pulseTimeRet = static_cast<uint64_t>(eventMsg->pulse_time());
  const DateAndTime pulseTime(pulseTimeRet);

  /* Get TOF and detector ID buffers */
  const auto &tofData = *(eventMsg->time_of_flight());
  const auto &detData = *(eventMsg->detector_id());
  /* Increment event count */
  const auto nEvents = tofData.size();
  eventCount += nEvents;

  /* Create buffered pulse */
  BufferedPulse pulse{pulseTime, 0};

  /* Perform facility specific operations */
  if (eventMsg->facility_specific_data_type() == FacilityData::ISISData) {
    std::lock_guard<std::mutex> workspaceLock(m_mutex);
    const auto ISISMsg = static_cast<const ISISData *>(eventMsg->facility_specific_data());
    pulse.periodNumber = static_cast<int>(ISISMsg->period_number());
    auto periodWs = m_localEvents[pulse.periodNumber];
    auto &mutableRunInfo = periodWs->mutableRun();
    mutableRunInfo.getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)->addValue(pulseTime, ISISMsg->proton_charge());
  }

  const auto starttime = std::chrono::system_clock::now();

  {
    std::lock_guard<std::mutex> bufferLock(m_intermediateBufferMutex);

    /* Store the buffered pulse */
    m_receivedPulseBuffer.emplace_back(pulse);
    const auto pulseIndex = m_receivedPulseBuffer.size() - 1;

    /* Ensure storage for newly received events */
    const auto oldBufferSize(m_receivedEventBuffer.size());
    m_receivedEventBuffer.reserve(oldBufferSize + nEvents);

    std::transform(detData.begin(), detData.end(), tofData.begin(), std::back_inserter(m_receivedEventBuffer),
                   [&](uint64_t detId, uint64_t tof) -> BufferedEvent {
                     const auto workspaceIndex = m_eventIdToWkspIdx(detId);
                     return {workspaceIndex, tof, pulseIndex};
                   });
  }

  const auto endTime = std::chrono::system_clock::now();
  const std::chrono::duration<double> dur = endTime - starttime;
  totalEventFromMessageDuration += dur.count();
  numEventFromMessageCalls += 1;
}

void KafkaEventStreamDecoder::flushIntermediateBuffer() {
  /* Do nothing if there are no buffered events */
  if (m_receivedEventBuffer.empty()) {
    return;
  }

  g_log.debug() << "Populating event workspace with " << m_receivedEventBuffer.size() << " events\n";

  const auto startTime = std::chrono::system_clock::now();

  std::lock_guard<std::mutex> bufferLock(m_intermediateBufferMutex);

  sortIntermediateEventBuffer(m_receivedEventBuffer, m_receivedPulseBuffer);

  /* Compute groups for parallel insertion */
  const auto numberOfGroups = PARALLEL_GET_MAX_THREADS;
  const auto groupBoundaries = computeGroupBoundaries(m_receivedEventBuffer, numberOfGroups);

  /* Insert events into EventWorkspace(s) */
  {
    std::lock_guard<std::mutex> workspaceLock(m_mutex);

    for (auto &ws : m_localEvents) {
      ws->invalidateCommonBinsFlag();
    }

    PARALLEL_FOR_NO_WSP_CHECK()
    for (auto group = 0; group < numberOfGroups; ++group) {
      for (auto idx = groupBoundaries[group]; idx < groupBoundaries[group + 1]; ++idx) {
        const auto &event = m_receivedEventBuffer[idx];
        const auto &pulse = m_receivedPulseBuffer[event.pulseIndex];

        auto *spectrum = m_localEvents[pulse.periodNumber]->getSpectrumUnsafe(event.wsIdx);

        // nanoseconds to microseconds
        spectrum->addEventQuickly(TofEvent(static_cast<double>(event.tof) * 1e-3, pulse.pulseTime));
      }
    }
  }

  /* Clear buffers */
  m_receivedPulseBuffer.clear();
  m_receivedEventBuffer.clear();

  const auto endTime = std::chrono::system_clock::now();
  const std::chrono::duration<double> dur = endTime - startTime;
  g_log.debug() << "Time to populate EventWorkspace: " << dur.count() << '\n';

  totalPopulateWorkspaceDuration += dur.count();
  numPopulateWorkspaceCalls += 1;
} // namespace LiveData

/**
 * Get sample environment log data from the flatbuffer and append it to the
 * workspace
 *
 * @param seData : flatbuffer offset of the sample environment log data
 * @param nSEEvents : number of sample environment log values in the flatbuffer
 * @param mutableRunInfo : Log manager containing the existing sample logs
 */
void KafkaEventStreamDecoder::sampleDataFromMessage(const std::string &buffer) {
  std::lock_guard<std::mutex> workspaceLock(m_mutex);
  // Add sample log values to every workspace for every period
  for (const auto &periodBuffer : m_localEvents) {
    auto &mutableRunInfo = periodBuffer->mutableRun();

    auto seEvent = GetLogData(reinterpret_cast<const uint8_t *>(buffer.c_str()));

    auto name = seEvent->source_name()->str();

    // Convert time from nanoseconds since 1 Jan 1970 to nanoseconds since 1 Jan
    // 1990 to create a Mantid timestamp
    const int64_t nanoseconds1970To1990 = 631152000000000000L;
    auto time = Core::DateAndTime(static_cast<int64_t>(seEvent->timestamp()) - nanoseconds1970To1990);

    // If sample log with this name already exists then append to it
    // otherwise create a new log
    if (seEvent->value_type() == Value::Int) {
      auto value = static_cast<const Int *>(seEvent->value());
      appendToLog<int32_t>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == Value::Long) {
      auto value = static_cast<const Long *>(seEvent->value());
      appendToLog<int64_t>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == Value::Double) {
      auto value = static_cast<const Double *>(seEvent->value());
      appendToLog<double>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == Value::Float) {
      auto value = static_cast<const Float *>(seEvent->value());
      appendToLog<double>(mutableRunInfo, name, time, static_cast<double>(value->value()));
    } else if (seEvent->value_type() == Value::Short) {
      auto value = static_cast<const Short *>(seEvent->value());
      appendToLog<int32_t>(mutableRunInfo, name, time, static_cast<int32_t>(value->value()));
    } else if (seEvent->value_type() == Value::String) {
      auto value = static_cast<const String *>(seEvent->value());
      appendToLog<std::string>(mutableRunInfo, name, time, static_cast<std::string>(value->value()->str()));
    } else if (seEvent->value_type() == Value::ArrayByte) {
      // do nothing for now.
    } else {
      g_log.warning() << "Value for sample log named '" << name << "' was not of recognised type. The value type is "
                      << EnumNameValue(seEvent->value_type()) << std::endl;
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
void KafkaEventStreamDecoder::initLocalCaches(const RunStartStruct &runStartData) {
  m_runId = runStartData.runId;

  const auto jsonGeometry = runStartData.nexusStructure;
  const auto instName = runStartData.instrumentName;

  DataObjects::EventWorkspace_sptr eventBuffer;
  if (!runStartData.detSpecMapSpecified) {
    /* Load the instrument to get the number of spectra :c */
    auto ws = API::WorkspaceFactory::Instance().create("EventWorkspace", 1, 2, 1);
    loadInstrument<API::MatrixWorkspace>(instName, ws, jsonGeometry);
    const auto nspec = ws->getInstrument()->getNumberDetectors();

    // Create buffer
    eventBuffer = std::static_pointer_cast<DataObjects::EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", nspec, 2, 1));
    eventBuffer->setInstrument(ws->getInstrument());
    /* Need a mapping with spectra numbers starting at zero */
    eventBuffer->rebuildSpectraMapping(true, 0);
    eventBuffer->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    eventBuffer->setYUnit("Counts");

  } else {
    // Create buffer
    eventBuffer = createBufferWorkspace<DataObjects::EventWorkspace>(
        "EventWorkspace", runStartData.numberOfSpectra, runStartData.spectrumNumbers.data(),
        runStartData.detectorIDs.data(), static_cast<uint32_t>(runStartData.detectorIDs.size()));
  }

  // Set mapping function
  if (ConfigService::Instance().getInstrument(instName).facility().name() == "ISIS" ||
      runStartData.detSpecMapSpecified) {
    specnum_t idToIdxOffset(0);
    auto specToIdx = eventBuffer->getSpectrumToWorkspaceIndexVector(idToIdxOffset);
    m_eventIdToWkspIdx = [specToIdx = std::move(specToIdx), idToIdxOffset](uint64_t specNum) {
      return specToIdx[specNum + idToIdxOffset];
    };
  } else {
    detid_t idToIdxOffset(0);
    auto detIdToIdx = eventBuffer->getDetectorIDToWorkspaceIndexVector(idToIdxOffset);
    m_eventIdToWkspIdx = [detIdToIdx = std::move(detIdToIdx), idToIdxOffset](uint64_t detId) {
      return detIdToIdx[detId + idToIdxOffset];
    };
  }

  // Load the instrument if possible but continue if we can't
  if (!loadInstrument<DataObjects::EventWorkspace>(instName, eventBuffer, jsonGeometry))
    g_log.warning("Instrument could not be loaded. Continuing without instrument");

  auto &mutableRun = eventBuffer->mutableRun();
  // Run start. Cache locally for computing frame times
  // Convert nanoseconds to seconds (and discard the extra precision)
  auto runStartTime = static_cast<time_t>(runStartData.startTime / 1000000000);
  m_runStart.set_from_time_t(runStartTime);
  auto timeString = m_runStart.toISO8601String();
  // Run number
  mutableRun.addProperty(RUN_START_PROPERTY, std::string(timeString));
  mutableRun.addProperty(RUN_NUMBER_PROPERTY, runStartData.runId);
  // Create the proton charge property
  mutableRun.addProperty(new Kernel::TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY));

  // Buffers for each period
  size_t nperiods = runStartData.nPeriods;
  if (nperiods == 0) {
    g_log.warning("KafkaEventStreamDecoder - Stream reports 0 periods. This is "
                  "an error by the data producer. Number of periods being set to 1.");
    nperiods = 1;
  }
  {
    std::lock_guard<std::mutex> workspaceLock(m_mutex);
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

std::vector<size_t>
computeGroupBoundaries(const std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent> &eventBuffer,
                       const size_t numberOfGroups) {
  std::vector<size_t> groupBoundaries(numberOfGroups + 1, eventBuffer.size());

  /* First group always starts at beginning of buffer */
  groupBoundaries[0] = 0;

  const auto eventsPerGroup = std::max<size_t>(1, eventBuffer.size() / numberOfGroups);

  /* Iterate over groups */
  for (size_t group = 1; group < numberOfGroups; ++group) {
    /* Calculate a reasonable end boundary for the group */
    groupBoundaries[group] = std::min(groupBoundaries[group - 1] + eventsPerGroup - 1, eventBuffer.size());

    /* If we have already gotten through all events then exit early, leaving
     * some threads without events. */
    if (groupBoundaries[group] == eventBuffer.size()) {
      break;
    }

    /* Advance the end boundary of the group until all events for a given
     * workspace index fall within a single group */
    while (groupBoundaries[group] + 1 < eventBuffer.size() &&
           (eventBuffer[groupBoundaries[group]].wsIdx == eventBuffer[groupBoundaries[group] + 1].wsIdx)) {
      ++groupBoundaries[group];
    }

    /* Increment group end boundary (such that group is defined by [lower,
     * upper) boundaries) */
    ++groupBoundaries[group];

    /* If we have already gotten through all events then exit early, leaving
     * some threads without events. */
    if (groupBoundaries[group] >= eventBuffer.size()) {
      break;
    }
  }

  return groupBoundaries;
}

} // namespace Mantid::LiveData
