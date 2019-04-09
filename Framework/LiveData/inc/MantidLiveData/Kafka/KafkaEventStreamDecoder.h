// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_
#define MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_

#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataObjects/EventWorkspace.h"
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
class DLLExport KafkaEventStreamDecoder : public IKafkaStreamDecoder {
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
  ///@name Querying
  ///@{
  bool hasData() const noexcept override;
  bool hasReachedEndOfRun() noexcept override;
  ///@}

private:
  struct BufferGroup {
    std::vector<uint32_t> tof;
    std::vector<uint32_t> detID;
    Types::Core::DateAndTime pulseTime;
    DataObjects::EventWorkspace_sptr periodBuffer;
  };

  void captureImplExcept() override;
  /// Populate cache workspaces with data from messages
  std::vector<size_t> getSpectrumIndices(const uint32_t *detIds,
                                         size_t nEvents);
  std::vector<size_t>
  getSortedSpectraPermutation(const std::vector<size_t> &specindices);
  void eventDataFromMessage(const std::string &buffer, size_t &eventCount,
                            int64_t &pulseTimeRet);
  void populateEventDataWorkspace();

  /// Create the cache workspaces, LoadLiveData extracts data from these
  void initLocalCaches(const std::string &rawMsgBuffer,
                       const RunStartStruct &runStartData) override;

  void sampleDataFromMessage(const std::string &buffer) override;

  /// For LoadLiveData to extract the cached data
  API::Workspace_sptr extractDataImpl() override;

  /// Local event workspace buffers
  std::vector<DataObjects::EventWorkspace_sptr> m_localEvents;

  std::vector<BufferGroup> m_buffer;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_ */
