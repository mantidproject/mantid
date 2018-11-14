// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_IKAFKASTREAMDECODER_H_
#define MANTID_LIVEDATA_IKAFKASTREAMDECODER_H_

#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Mantid {
namespace LiveData {

/**
Kafka stream decoder interface. Handles (implements) all thread synchronization
functionality for accessing the data stream and processing data.
*/
class DLLExport IKafkaStreamDecoder {
public:
  using CallbackFn = std::function<void()>;

public:
  IKafkaStreamDecoder(std::shared_ptr<IKafkaBroker> broker,
                      const std::string &streamTopic,
                      const std::string &runInfoTopic,
                      const std::string &spDetTopic,
                      const std::string &sampleEnvTopic);
  virtual ~IKafkaStreamDecoder();
  IKafkaStreamDecoder(const IKafkaStreamDecoder &) = delete;
  IKafkaStreamDecoder &operator=(const IKafkaStreamDecoder &) = delete;

public:
  ///@name Start/stop
  ///@{
  void startCapture(bool startNow = true);
  void stopCapture() noexcept;
  ///@}

  ///@name Querying
  ///@{
  bool isCapturing() const noexcept { return m_capturing; }
  virtual bool hasData() const noexcept = 0;
  int runNumber() const noexcept { return m_runNumber; }
  virtual bool hasReachedEndOfRun() noexcept = 0;
  bool dataReset();
  ///@}

  ///@name Callbacks
  ///@{
  virtual void registerIterationEndCb(CallbackFn cb) {
    m_cbIterationEnd = std::move(cb);
  }
  virtual void registerErrorCb(CallbackFn cb) { m_cbError = std::move(cb); }
  ///@}

  ///@name Modifying
  ///@{
  API::Workspace_sptr extractData();
  ///@}

protected:
  struct RunStartStruct {
    std::string instrumentName;
    int runNumber;
    uint64_t startTime;
    size_t nPeriods;
    int64_t runStartMsgOffset;
  };

  /// Main loop of listening for data messages and populating the cache
  /// workspaces
  void captureImpl() noexcept;
  virtual void captureImplExcept() = 0;

  /// Create the cache workspaces, LoadLiveData extracts data from these
  virtual void initLocalCaches(const std::string &rawMsgBuffer,
                               const RunStartStruct &runStartData) = 0;

  /// Get an expected message from the run information topic
  int64_t getRunInfoMessage(std::string &rawMsgBuffer);

  /// Get an expected RunStart message
  RunStartStruct getRunStartMessage(std::string &rawMsgBuffer);

  /// Populate cache workspaces with data from messages
  virtual void sampleDataFromMessage(const std::string &buffer) = 0;

  /// For LoadLiveData to extract the cached data
  virtual API::Workspace_sptr extractDataImpl() = 0;

  /// Broker to use to subscribe to topics
  std::shared_ptr<IKafkaBroker> m_broker;
  /// Topic names
  const std::string m_streamTopic;
  const std::string m_runInfoTopic;
  const std::string m_spDetTopic;
  const std::string m_sampleEnvTopic;
  /// Flag indicating if user interruption has been requested
  std::atomic<bool> m_interrupt;
  /// Subscriber for the data stream
  std::unique_ptr<IKafkaStreamSubscriber> m_dataStream;
  /// Mapping of spectrum number to workspace index.
  spec2index_map m_specToIdx;
  /// Start time of the run
  Types::Core::DateAndTime m_runStart;
  /// Subscriber for the run info stream
  std::unique_ptr<IKafkaStreamSubscriber> m_runStream;
  /// Subscriber for the run info stream
  std::unique_ptr<IKafkaStreamSubscriber> m_spDetStream;
  /// Run number
  int m_runNumber;

  /// Associated thread running the capture process
  std::thread m_thread;
  /// Mutex protecting event buffers
  mutable std::mutex m_mutex;
  /// Mutex protecting the wait flag
  mutable std::mutex m_waitMutex;
  /// Mutex protecting the runStatusSeen flag
  mutable std::mutex m_runStatusMutex;
  /// Flag indicating that the decoder is capturing
  std::atomic<bool> m_capturing;
  /// Exception object indicating there was an error
  boost::shared_ptr<std::runtime_error> m_exception;

  /// For notifying other threads of changes to conditions (the following bools)
  std::condition_variable m_cv;
  std::condition_variable m_cvRunStatus;
  /// Indicate that decoder has reached the last message in a run
  std::atomic<bool> m_endRun;
  /// Indicate that LoadLiveData is waiting for access to the buffer workspace
  std::atomic<bool> m_extractWaiting;
  /// Indicate that MonitorLiveData has seen the runStatus since it was set to
  /// EndRun
  bool m_runStatusSeen;
  std::atomic<bool> m_extractedEndRunData;
  /// Indicate if the next data to be extracted should replace LoadLiveData's
  /// output workspace
  std::atomic<bool> m_dataReset;

  void waitForDataExtraction();
  void waitForRunEndObservation();

  std::map<int32_t, std::set<int32_t>>
  buildSpectrumToDetectorMap(const size_t nspectra, const int32_t *spec,
                             const int32_t *udet, uint32_t length);

  template <typename T>
  boost::shared_ptr<T>
  createBufferWorkspace(const std::string &workspaceClassName, size_t nspectra,
                        const int32_t *spec, const int32_t *udet,
                        uint32_t length);
  template <typename T>
  boost::shared_ptr<T>
  createBufferWorkspace(const std::string &workspaceClassName,
                        const boost::shared_ptr<T> &parent);

  template <typename T>
  void loadInstrument(const std::string &name, boost::shared_ptr<T> workspace);

  void checkRunMessage(
      const std::string &buffer, bool &checkOffsets,
      std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
      std::unordered_map<std::string, std::vector<bool>> &reachedEnd);

  void checkRunEnd(
      const std::string &topicName, bool &checkOffsets, const int64_t offset,
      const int32_t partition,
      std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
      std::unordered_map<std::string, std::vector<bool>> &reachedEnd);

  /// Methods for checking if the end of a run was reached
  std::unordered_map<std::string, std::vector<int64_t>> getStopOffsets(
      std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
      std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
      uint64_t stopTime) const;
  void checkIfAllStopOffsetsReached(
      const std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
      bool &checkOffsets);

  /// Callbacks for unit tests
  CallbackFn m_cbIterationEnd;
  CallbackFn m_cbError;

  /// Waits until a run start message with higher run number is received
  bool waitForNewRunStartMessage(RunStartStruct &runStartStructOutput);
  /// Subscribe to data stream at the time specified in a run start message
  void joinStreamAtTime(const RunStartStruct &runStartData);
  /// Convert a duration in nanoseconds to milliseconds
  int64_t nanosecondsToMilliseconds(uint64_t timeNanoseconds) const;
  /// Get a det-spec map message using the time specified in a run start message
  std::string getDetSpecMapForRun(const RunStartStruct &runStartStruct);
};

/**
 * Create a buffer workspace of the correct size based on the values given.
 * @param workspaceClassName the name of the workspace class to be created e.g
 * Workspace2D or EventWorkspace
 * @param nspectra The number of unique spectrum numbers
 * @param spec An array of length ndet specifying the spectrum number of each
 * detector
 * @param udet An array of length ndet specifying the detector ID of each
 * detector
 * @param length The length of the spec/udet arrays
 * @return A new workspace of the appropriate size
 */
template <typename T>
boost::shared_ptr<T> IKafkaStreamDecoder::createBufferWorkspace(
    const std::string &workspaceClassName, size_t nspectra, const int32_t *spec,
    const int32_t *udet, uint32_t length) {
  // Get spectra to detector mapping
  auto spdetMap = buildSpectrumToDetectorMap(nspectra, spec, udet, length);

  // Create histo workspace
  auto buffer =
      boost::static_pointer_cast<T>(API::WorkspaceFactory::Instance().create(
          workspaceClassName, nspectra, 2, 1));

  // Set the units
  buffer->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  buffer->setYUnit("Counts");
  // Setup spectra-detector mapping.
  size_t wsIdx(0);
  for (const auto &spIter : spdetMap) {
    auto &spectrum = buffer->getSpectrum(wsIdx);
    spectrum.setSpectrumNo(spIter.first);
    spectrum.addDetectorIDs(spIter.second);
    ++wsIdx;
  }
  return buffer;
}

/**
 * Create new buffer workspace from an existing copy
 * @param workspaceClassName the name of the workspace class to be created e.g
 * Workspace2D or EventWorkspace
 * @param parent A reference to an existing workspace
 */
template <typename T>
boost::shared_ptr<T> IKafkaStreamDecoder::createBufferWorkspace(
    const std::string &workspaceClassName, const boost::shared_ptr<T> &parent) {
  auto buffer =
      boost::static_pointer_cast<T>(API::WorkspaceFactory::Instance().create(
          workspaceClassName, parent->getNumberHistograms(), 2, 1));
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
template <typename T>
void IKafkaStreamDecoder::loadInstrument(const std::string &name,
                                         boost::shared_ptr<T> workspace) {
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
#endif // MANTID_LIVEDATA_IKAFKASTREAMDECODER_H_
