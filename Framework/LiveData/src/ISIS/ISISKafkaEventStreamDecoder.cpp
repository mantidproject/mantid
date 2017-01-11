#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidLiveData/Exception.h"

GCC_DIAG_OFF(conversion)
#include "private/Kafka/Schema/det_spec_mapping_schema_generated.h"
#include "private/Kafka/Schema/event_schema_generated.h"
GCC_DIAG_ON(conversion)

#include <boost/make_shared.hpp>

#include <cassert>
#include <condition_variable>
#include <functional>
#include <map>

namespace {
/// Logger
Mantid::Kernel::Logger g_log("ISISKafkaEventStreamDecoder");

std::string PROTON_CHARGE_PROPERTY = "proton_charge";
std::string RUN_NUMBER_PROPERTY = "run_number";
std::string RUN_START_PROPERTY = "run_start";

std::condition_variable cv;
bool extractWaiting = false;

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
                 const Mantid::Kernel::DateAndTime &time, T value) {
  if (mutableRunInfo.hasProperty(name)) {
    auto property = mutableRunInfo.getTimeSeriesProperty<T>(name);
    property->addValue(time, value);
  } else {
    auto property = new Mantid::Kernel::TimeSeriesProperty<T>(name);
    property->addValue(time, value);
    mutableRunInfo.addLogData(property);
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
void addSampleEnvLogs(
    const flatbuffers::Vector<flatbuffers::Offset<ISISStream::SEEvent>> &seData,
    flatbuffers::uoffset_t nSEEvents, Mantid::API::Run &mutableRunInfo) {
  for (decltype(nSEEvents) i = 0; i < nSEEvents; ++i) {
    auto seEvent = seData[i];
    auto name = seEvent->name()->str();

    // Convert time from seconds since start of run to an absolute datetime
    auto time = mutableRunInfo.startTime() + seEvent->time_offset();

    // If sample log with this name already exists then append to it
    // otherwise create a new log
    if (seEvent->value_type() == ISISStream::SEValue_IntValue) {
      auto value = static_cast<const ISISStream::IntValue *>(seEvent->value());
      appendToLog<int32_t>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == ISISStream::SEValue_LongValue) {
      auto value = static_cast<const ISISStream::LongValue *>(seEvent->value());
      appendToLog<int64_t>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == ISISStream::SEValue_DoubleValue) {
      auto value =
          static_cast<const ISISStream::DoubleValue *>(seEvent->value());
      appendToLog<double>(mutableRunInfo, name, time, value->value());
    } else if (seEvent->value_type() == ISISStream::SEValue_StringValue) {
      auto value =
          static_cast<const ISISStream::StringValue *>(seEvent->value());
      appendToLog<std::string>(mutableRunInfo, name, time,
                               value->value()->str());
    } else {
      g_log.warning() << "SEValue for log named '" << name
                      << "' was not of recognised type" << std::endl;
    }
  }
}
}

namespace Mantid {
namespace LiveData {
using DataObjects::TofEvent;
using Kernel::DateAndTime;

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
      m_localEvents(), m_specToIdx(), m_runStart(),
      m_runStream(broker.subscribe(runInfoTopic)),
      m_spDetStream(broker.subscribe(spDetTopic)), m_runNumber(-1), m_thread(),
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
bool ISISKafkaEventStreamDecoder::hasData() const noexcept {
  std::lock_guard<std::mutex> lock(m_mutex);
  return !m_localEvents.empty();
}

/**
 * Check for an exception thrown by the background thread and rethrow
 * it if necessary. If no error occurred swap the current internal buffer
 * for a fresh one and return the old buffer.
 * @return A pointer to the data collected since the last call to this
 * method
 */
API::Workspace_sptr ISISKafkaEventStreamDecoder::extractData() {
  if (m_exception) {
    throw * m_exception;
  }

  std::unique_lock<std::mutex> waitLock(m_waitMutex);
  extractWaiting = true;
  waitLock.unlock();
  cv.notify_one(); // TODO this line may not be required

  auto workspace_ptr = extractDataImpl();

  waitLock.lock();
  extractWaiting = false;
  waitLock.unlock();
  cv.notify_one();

  return workspace_ptr;
}

API::Workspace_sptr ISISKafkaEventStreamDecoder::extractDataImpl() {
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

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------
/**
 * Start decoding data from the streams into the internal buffers.
 * Implementation designed to be entry point for new thread of execution.
 * It catches all thrown exceptions.
 */
void ISISKafkaEventStreamDecoder::captureImpl() noexcept {
  m_capturing = true;
  try {
    captureImplExcept();
  } catch (std::exception &exc) {
    m_exception = boost::make_shared<std::runtime_error>(exc.what());
  } catch (...) {
    m_exception = boost::make_shared<std::runtime_error>(
        "ISISKafkaEventStreamDecoder: Unknown exception type caught.");
  }
  m_capturing = false;
}

/**
 * Exception-throwing variant of captureImpl(). Do not call this directly
 */
void ISISKafkaEventStreamDecoder::captureImplExcept() {
  g_log.debug("Event capture starting");
  initLocalCaches();

  m_interrupt = false;
  std::string buffer;
  while (!m_interrupt) {
    // Let another thread do something for a short while
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //TODO if extractData method is waiting for access then we wait for it to finish with the buffer workspace
    std::unique_lock<std::mutex> readyLock(m_waitMutex);
    //TODO is this if statement required?
    if (extractWaiting) {
      cv.wait(readyLock, [&] { return !extractWaiting; });
    }
    readyLock.unlock();

    // Pull in events
    m_eventStream->consumeMessage(&buffer);
    // No events, wait for some to come along...
    if (buffer.empty())
      continue;
    auto evtMsg = ISISStream::GetEventMessage(
        reinterpret_cast<const uint8_t *>(buffer.c_str()));
    if (evtMsg->message_type() == ISISStream::MessageTypes_FramePart) {
      auto frameData =
          static_cast<const ISISStream::FramePart *>(evtMsg->message());
      DateAndTime pulseTime =
          m_runStart + static_cast<double>(frameData->frame_time());
      const auto eventData = frameData->n_events();
      const auto &seData = *(frameData->se_events());
      const auto &tofData = *(eventData->tof());
      const auto &specData = *(eventData->spec());
      auto nevents = tofData.size();
      auto nSEEvents = seData.size();

      std::lock_guard<std::mutex> lock(m_mutex);
      auto &periodBuffer = *m_localEvents[frameData->period()];
      auto &mutableRunInfo = periodBuffer.mutableRun();
      mutableRunInfo.getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)
          ->addValue(pulseTime, frameData->proton_charge());
      for (decltype(nevents) i = 0; i < nevents; ++i) {
        auto &spectrum = periodBuffer.getSpectrum(m_specToIdx[specData[i]]);
        spectrum.addEventQuickly(TofEvent(tofData[i], pulseTime));
      }
      addSampleEnvLogs(seData, nSEEvents, mutableRunInfo);
    }
  }
  g_log.debug("Event capture finished");
}

/**
 * Pull information from the run & detector-spectrum stream and initialize
 * the internal EventWorkspace buffer + other cached information such as run
 * start. This includes loading the instrument.
 * By the end of this method the local event buffer is ready to accept
 * events
 */
void ISISKafkaEventStreamDecoder::initLocalCaches() {
  std::string rawMsgBuffer;

  // Load spectra-detector mapping from stream
  m_spDetStream->consumeMessage(&rawMsgBuffer);
  if (rawMsgBuffer.empty()) {
    throw std::runtime_error("ISISKafkaEventStreamDecoder::initLocalCaches() - "
                             "Empty message received from spectrum-detector "
                             "topic. Unable to continue");
  }
  auto spDetMsg = ISISStream::GetSpectraDetectorMapping(
      reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
  auto nspec = spDetMsg->spec()->size();
  auto nudet = spDetMsg->det()->size();
  if (nudet != nspec) {
    std::ostringstream os;
    os << "ISISKafkaEventStreamDecoder::initLocalEventBuffer() - Invalid "
          "spectra/detector mapping. Expected matched length arrays but "
          "found nspec="
       << nspec << ", ndet=" << nudet;
    throw std::runtime_error(os.str());
  }
  // Create buffer
  auto eventBuffer = createBufferWorkspace(
      static_cast<size_t>(spDetMsg->n_spectra()), spDetMsg->spec()->data(),
      spDetMsg->det()->data(), nudet);

  // Load run metadata
  m_runStream->consumeMessage(&rawMsgBuffer);
  if (rawMsgBuffer.empty()) {
    throw std::runtime_error("ISISKafkaEventStreamDecoder::initLocalCaches() - "
                             "Empty message received from run info "
                             "topic. Unable to continue");
  }
  auto runMsg = ISISStream::GetRunInfo(
      reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));

  // Load the instrument if possibly but continue if we can't
  auto instName = runMsg->inst_name();
  if (instName && instName->size() > 0)
    loadInstrument(instName->c_str(), eventBuffer);
  else
    g_log.warning(
        "Empty instrument name received. Continuing without instrument");

  auto &mutableRun = eventBuffer->mutableRun();
  // Run start. Cache locally for computing frame times
  time_t runStartTime = runMsg->start_time();
  char timeString[32];
  strftime(timeString, 32, "%Y-%m-%dT%H:%M:%S", localtime(&runStartTime));
  m_runStart.setFromISO8601(timeString, false);
  // Run number
  mutableRun.addProperty(RUN_START_PROPERTY, std::string(timeString));
  m_runNumber = runMsg->run_number();
  mutableRun.addProperty(RUN_NUMBER_PROPERTY, std::to_string(m_runNumber));
  // Create the proton charge property
  mutableRun.addProperty(
      new Kernel::TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY));

  // Cache spec->index mapping. We assume it is the same across all periods
  m_specToIdx = eventBuffer->getSpectrumToWorkspaceIndexMap();

  // Buffers for each period
  const size_t nperiods(static_cast<size_t>(runMsg->n_periods()));
  if (nperiods == 0) {
    throw std::runtime_error(
        "ISISKafkaEventStreamDecoder - Message has n_periods==0. This is "
        "an error by the data producer");
  }
  std::lock_guard<std::mutex> lock(m_mutex);
  m_localEvents.resize(nperiods);
  m_localEvents[0] = eventBuffer;
  for (size_t i = 1; i < nperiods; ++i) {
    // A clone should be cheap here as there are no events yet
    m_localEvents[i] = eventBuffer->clone();
  }
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
DataObjects::EventWorkspace_sptr
ISISKafkaEventStreamDecoder::createBufferWorkspace(const size_t nspectra,
                                                   const int32_t *spec,
                                                   const int32_t *udet,
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
DataObjects::EventWorkspace_sptr
ISISKafkaEventStreamDecoder::createBufferWorkspace(
    const DataObjects::EventWorkspace_sptr &parent) {
  auto buffer = boost::static_pointer_cast<DataObjects::EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", parent->getNumberHistograms(), 2, 1));
  // Copy meta data
  API::WorkspaceFactory::Instance().initializeFromParent(*parent, *buffer, false);
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
void ISISKafkaEventStreamDecoder::loadInstrument(
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
}

} // namespace Mantid
