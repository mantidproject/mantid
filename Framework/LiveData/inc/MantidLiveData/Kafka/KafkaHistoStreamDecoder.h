// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/Workspace2D.h"
#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"

namespace Mantid {
namespace LiveData {

/**
  High-level interface to ISIS Kafka histo system.

  A call to startCapture() starts the process of capturing the stream on a
  separate thread.

  Some further documentation is in docs/source/concepts/KafkaLiveStreams.rst
*/
class MANTID_LIVEDATA_DLL KafkaHistoStreamDecoder : public IKafkaStreamDecoder {
public:
  KafkaHistoStreamDecoder(std::shared_ptr<IKafkaBroker> broker, const std::string &histoTopic,
                          const std::string &runInfoTopic, const std::string &sampleEnvTopic,
                          const std::string &chopperTopic);
  ~KafkaHistoStreamDecoder() override;
  // Disable copies since multiple subscribers will cause problems
  KafkaHistoStreamDecoder(const KafkaHistoStreamDecoder &) = delete;
  KafkaHistoStreamDecoder &operator=(const KafkaHistoStreamDecoder &) = delete;

  // Allow moves though, since we only keep one copy still
  KafkaHistoStreamDecoder(KafkaHistoStreamDecoder &&) noexcept;

  bool hasData() const noexcept override;
  bool hasReachedEndOfRun() noexcept override { return !m_capturing; }

private:
  void captureImplExcept() override;

  /// Create the cache workspaces, LoadLiveData extracts data from these
  void initLocalCaches(const RunStartStruct &runStartData) override;

  void sampleDataFromMessage(const std::string &buffer) override;

  API::Workspace_sptr extractDataImpl() override;

private:
  std::string m_buffer;
  DataObjects::Workspace2D_sptr m_workspace;
};

} // namespace LiveData
} // namespace Mantid
