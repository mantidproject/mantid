#ifndef MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_
#define MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_

#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace Mantid {
namespace LiveData {

/**
  High-level interface to Kafka event system. It requires
  3 topic names of the data streams.

  A call to capture() starts the process of capturing the stream on a separate
  thread.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport KafkaEventStreamDecoder {
public:
  using CallbackFn = std::function<void()>;

public:
  KafkaEventStreamDecoder(std::shared_ptr<IKafkaBroker> broker,
                          const std::string &eventTopic,
                          const std::string &runInfoTopic,
                          const std::string &spDetTopic,
                          const std::string &sampleEnvTopic);
  ~KafkaEventStreamDecoder();
  KafkaEventStreamDecoder(const KafkaEventStreamDecoder &) = delete;
  KafkaEventStreamDecoder &operator=(const KafkaEventStreamDecoder &) = delete;

public:
  ///@name Start/stop
  ///@{
  void startCapture(bool startNow = true);
  void stopCapture() noexcept;
  ///@}

  ///@name Querying
  ///@{
  bool isCapturing() const noexcept { return m_capturing; }
  bool hasData() const noexcept;
  int runNumber() const noexcept { return m_runNumber; }
  bool hasReachedEndOfRun() noexcept;
  bool dataReset();
  ///@}

  ///@name Callbacks
  ///@{
  void registerIterationEndCb(CallbackFn cb) {
    m_cbIterationEnd = std::move(cb);
  }
  void registerErrorCb(CallbackFn cb) { m_cbError = std::move(cb); }
  ///@}

  ///@name Modifying
  ///@{
  API::Workspace_sptr extractData();
  ///@}

private:
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
  void captureImplExcept();

  /// Create the cache workspaces, LoadLiveData extracts data from these
  void initLocalCaches(const std::string &rawMsgBuffer,
                       const RunStartStruct &runStartData);
  DataObjects::EventWorkspace_sptr createBufferWorkspace(size_t nspectra,
                                                         const int32_t *spec,
                                                         const int32_t *udet,
                                                         uint32_t length);
  DataObjects::EventWorkspace_sptr
  createBufferWorkspace(const DataObjects::EventWorkspace_sptr &parent);

  /// Load a named instrument into a workspace
  void loadInstrument(const std::string &name,
                      DataObjects::EventWorkspace_sptr workspace);

  /// Get an expected message from the run information topic
  int64_t getRunInfoMessage(std::string &rawMsgBuffer);

  /// Get an expected RunStart message
  RunStartStruct getRunStartMessage(std::string &rawMsgBuffer);

  /// Populate cache workspaces with data from messages
  void eventDataFromMessage(const std::string &buffer);
  void sampleDataFromMessage(const std::string &buffer);

  /// For LoadLiveData to extract the cached data
  API::Workspace_sptr extractDataImpl();

  /// Broker to use to subscribe to topics
  std::shared_ptr<IKafkaBroker> m_broker;
  /// Topic names
  const std::string m_eventTopic;
  const std::string m_runInfoTopic;
  const std::string m_spDetTopic;
  const std::string m_sampleEnvTopic;
  /// Flag indicating if user interruption has been requested
  std::atomic<bool> m_interrupt;
  /// Subscriber for the event stream
  std::unique_ptr<IKafkaStreamSubscriber> m_eventStream;
  /// Local event workspace buffers
  std::vector<DataObjects::EventWorkspace_sptr> m_localEvents;
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
  /// Subscribe to event stream at the time specified in a run start message
  void joinEventStreamAtTime(const RunStartStruct &runStartData);
  /// Convert a duration in nanoseconds to milliseconds
  int64_t nanosecondsToMilliseconds(uint64_t timeNanoseconds) const;
  /// Get a det-spec map message using the time specified in a run start message
  std::string getDetSpecMapForRun(const RunStartStruct &runStartStruct);
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_ */
