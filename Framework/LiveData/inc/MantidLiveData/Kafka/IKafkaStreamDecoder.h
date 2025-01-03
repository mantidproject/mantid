// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidLiveData/DllConfig.h"
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
class MANTID_LIVEDATA_DLL IKafkaStreamDecoder {
public:
  /**
   * Defines a thread-safe callback. A mutex is held
   * both during assignment of a new callback function
   * and during the call.
   */
  class Callback {
  public:
    using FnType = std::function<void()>;

    explicit Callback(const Callback::FnType &callback) : m_mutex(), m_callback() { setFunction(callback); }

    Callback(Callback &&other) noexcept {
      {
        // We must lock the other obj - not ourself
        std::lock_guard lck(other.m_mutex);
        m_callback = std::move(other.m_callback);
      }
    }

    inline void operator()() {
      std::lock_guard<std::mutex> lck(m_mutex);
      m_callback();
    }
    void setFunction(const Callback::FnType &callback) {
      std::lock_guard<std::mutex> lck(m_mutex);
      m_callback = callback;
    }

  private:
    std::mutex m_mutex;
    FnType m_callback;
  };

public:
  IKafkaStreamDecoder(std::shared_ptr<IKafkaBroker> broker, std::string streamTopic, std::string runInfoTopic,
                      std::string sampleEnvTopic, std::string chopperTopic, std::string monitorTopic);
  virtual ~IKafkaStreamDecoder();
  IKafkaStreamDecoder(const IKafkaStreamDecoder &) = delete;
  IKafkaStreamDecoder &operator=(const IKafkaStreamDecoder &) = delete;

  IKafkaStreamDecoder(IKafkaStreamDecoder &&) noexcept;

  ///@name Start/stop
  ///@{
  void startCapture(bool startNow = true);
  void stopCapture() noexcept;
  ///@}

  ///@name Querying
  ///@{
  bool isCapturing() const noexcept { return m_capturing; }
  virtual bool hasData() const noexcept = 0;
  std::string runId() const noexcept { return m_runId; }
  int runNumber() const noexcept;
  virtual bool hasReachedEndOfRun() noexcept = 0;
  bool dataReset();
  ///@}

  ///@name Callbacks
  ///@{
  virtual void registerIterationEndCb(const Callback::FnType &cb) { m_cbIterationEnd.setFunction(cb); }
  virtual void registerErrorCb(const Callback::FnType &cb) { m_cbError.setFunction(cb); }
  ///@}

  ///@name Modifying
  ///@{
  API::Workspace_sptr extractData();
  ///@}

protected:
  struct RunStartStruct {
    std::string instrumentName;
    std::string runId;
    uint64_t startTime;
    size_t nPeriods;
    std::string nexusStructure;
    int64_t runStartMsgOffset;

    // Detector-Spectrum mapping information
    bool detSpecMapSpecified = false;
    size_t numberOfSpectra = 0;
    std::vector<int32_t> spectrumNumbers;
    std::vector<int32_t> detectorIDs;
  };

  /// Main loop of listening for data messages and populating the cache
  /// workspaces
  void captureImpl() noexcept;
  virtual void captureImplExcept() = 0;

  /// Create the cache workspaces, LoadLiveData extracts data from these
  virtual void initLocalCaches(const RunStartStruct &runStartData) = 0;

  /// Get an expected message from the run information topic
  int64_t getRunInfoMessage(std::string &rawMsgBuffer);

  /// Get an expected RunStart message
  RunStartStruct getRunStartMessage(std::string &rawMsgBuffer);

  /// Populate cache workspaces with data from messages
  virtual void sampleDataFromMessage(const std::string &buffer) = 0;

  template <typename T = API::MatrixWorkspace> void writeChopperTimestampsToWorkspaceLogs(std::vector<T> workspaces);

  /// For LoadLiveData to extract the cached data
  virtual API::Workspace_sptr extractDataImpl() = 0;

  /// Broker to use to subscribe to topics
  std::shared_ptr<IKafkaBroker> m_broker;
  /// Topic names
  const std::string m_streamTopic;
  const std::string m_runInfoTopic;
  const std::string m_sampleEnvTopic;
  const std::string m_chopperTopic;
  const std::string m_monitorTopic;
  /// Flag indicating if user interruption has been requested
  std::atomic<bool> m_interrupt;
  /// Subscriber for the data stream
  std::unique_ptr<IKafkaStreamSubscriber> m_dataStream;
  /// Map from detId to workspace index
  std::function<size_t(uint64_t)> m_eventIdToWkspIdx;
  /// Start time of the run
  Types::Core::DateAndTime m_runStart;
  /// Subscriber for the run info stream
  std::unique_ptr<IKafkaStreamSubscriber> m_runStream;
  /// Subscriber for the chopper timestamp stream
  std::unique_ptr<IKafkaStreamSubscriber> m_chopperStream;
  /// Run number
  std::string m_runId;

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
  std::shared_ptr<std::runtime_error> m_exception;

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

  static std::map<int32_t, std::set<int32_t>> buildSpectrumToDetectorMap(const int32_t *spec, const int32_t *udet,
                                                                         uint32_t length);

  template <typename T>
  std::shared_ptr<T> createBufferWorkspace(const std::string &workspaceClassName, size_t nspectra, const int32_t *spec,
                                           const int32_t *udet, uint32_t length);
  template <typename T>
  std::shared_ptr<T> createBufferWorkspace(const std::string &workspaceClassName, const std::shared_ptr<T> &parent);

  template <typename T>
  bool loadInstrument(const std::string &name, std::shared_ptr<T> workspace, const std::string &jsonGeometry = "");

  void checkRunMessage(const std::string &buffer, bool &checkOffsets,
                       std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
                       std::unordered_map<std::string, std::vector<bool>> &reachedEnd);

  void checkRunEnd(const std::string &topicName, bool &checkOffsets, int64_t offset, int32_t partition,
                   std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
                   std::unordered_map<std::string, std::vector<bool>> &reachedEnd);

  /// Methods for checking if the end of a run was reached
  std::unordered_map<std::string, std::vector<int64_t>>
  getStopOffsets(std::unordered_map<std::string, std::vector<int64_t>> &stopOffsets,
                 std::unordered_map<std::string, std::vector<bool>> &reachedEnd, uint64_t stopTime) const;
  void checkIfAllStopOffsetsReached(const std::unordered_map<std::string, std::vector<bool>> &reachedEnd,
                                    bool &checkOffsets);

  /// Callbacks for unit tests
  Callback m_cbIterationEnd;
  Callback m_cbError;

  /// Waits until a run start message with higher run number is received
  bool waitForNewRunStartMessage(RunStartStruct &runStartStructOutput);
  /// Subscribe to data stream at the time specified in a run start message
  void joinStreamAtTime(const RunStartStruct &runStartData);
  /// Convert a duration in nanoseconds to milliseconds
  static int64_t nanosecondsToMilliseconds(uint64_t timeNanoseconds);

  static RunStartStruct extractRunStartDataFromMessage(const std::string &messageBuffer, int64_t offset);
};
} // namespace LiveData
} // namespace Mantid
