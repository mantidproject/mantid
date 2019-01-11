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
  void captureImplExcept() override;

  /// Create the cache workspaces, LoadLiveData extracts data from these
  void initLocalCaches(const std::string &rawMsgBuffer,
                       const RunStartStruct &runStartData) override;

  /// Populate cache workspaces with data from messages
  void eventDataFromMessage(const std::string &buffer);

  void sampleDataFromMessage(const std::string &buffer) override;

  /// For LoadLiveData to extract the cached data
  API::Workspace_sptr extractDataImpl() override;

  /// Local event workspace buffers
  std::vector<DataObjects::EventWorkspace_sptr> m_localEvents;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODER_H_ */
