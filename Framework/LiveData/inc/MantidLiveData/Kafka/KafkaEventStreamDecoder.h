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
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"

#include <vector>

namespace Mantid {
namespace LiveData {
/**
  High-level interface to Kafka event system. It requires
  3 topic names of the data streams.

  A call to capture() starts the process of capturing the stream on a separate
  thread.
*/
class MANTID_LIVEDATA_DLL KafkaEventStreamDecoder : public IKafkaStreamDecoder {
public:
  struct BufferedPulse {
    Types::Core::DateAndTime pulseTime;
    int periodNumber;
  };

  struct BufferedEvent {
    size_t wsIdx;
    uint64_t tof;
    size_t pulseIndex;
  };

public:
  KafkaEventStreamDecoder(std::shared_ptr<IKafkaBroker> broker, const std::string &eventTopic,
                          const std::string &runInfoTopic, const std::string &sampleEnvTopic,
                          const std::string &chopperTopic, const std::string &monitorTopic,
                          const std::size_t bufferThreshold);
  ~KafkaEventStreamDecoder() override;
  KafkaEventStreamDecoder(const KafkaEventStreamDecoder &) = delete;
  KafkaEventStreamDecoder &operator=(const KafkaEventStreamDecoder &) = delete;

  KafkaEventStreamDecoder(KafkaEventStreamDecoder &&) noexcept;

public:
  ///@name Querying
  ///@{
  bool hasData() const noexcept override;
  bool hasReachedEndOfRun() noexcept override;
  ///@}

private:
  void captureImplExcept() override;

  void eventDataFromMessage(const std::string &buffer, size_t &eventCount, uint64_t &pulseTimeRet);

  void flushIntermediateBuffer();

  /// Create the cache workspaces, LoadLiveData extracts data from these
  void initLocalCaches(const RunStartStruct &runStartData) override;

  void sampleDataFromMessage(const std::string &buffer) override;

  /// For LoadLiveData to extract the cached data
  API::Workspace_sptr extractDataImpl() override;

  /// Local event workspace buffers
  std::vector<DataObjects::EventWorkspace_sptr> m_localEvents;

  /// Intermediate buffer for received events yet to be populated in
  /// m_localEvents
  std::vector<BufferedEvent> m_receivedEventBuffer;
  std::vector<BufferedPulse> m_receivedPulseBuffer;
  /// Mutex protecting intermediate buffers
  mutable std::mutex m_intermediateBufferMutex;
  /// The number of events above which the intermediate buffer will be flushed
  const std::size_t m_intermediateBufferFlushThreshold;
};

MANTID_LIVEDATA_DLL std::vector<size_t>
computeGroupBoundaries(const std::vector<KafkaEventStreamDecoder::BufferedEvent> &eventBuffer,
                       const size_t numberOfGroups);

} // namespace LiveData
} // namespace Mantid
