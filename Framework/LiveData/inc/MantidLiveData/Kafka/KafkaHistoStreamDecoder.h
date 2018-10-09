// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_ISISKAFKAHISTOSTREAMDECODER_H_
#define MANTID_LIVEDATA_ISISKAFKAHISTOSTREAMDECODER_H_

#include "MantidDataObjects/Workspace2D.h"
#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace Mantid {
namespace LiveData {

/**
  High-level interface to ISIS Kafka histo system.

  A call to startCapture() starts the process of capturing the stream on a
  separate thread.
*/
class DLLExport KafkaHistoStreamDecoder {
public:
  KafkaHistoStreamDecoder(std::shared_ptr<IKafkaBroker> broker,
                          const std::string &histoTopic,
                          const std::string &instrumentName);
  ~KafkaHistoStreamDecoder();
  KafkaHistoStreamDecoder(const KafkaHistoStreamDecoder &) = delete;
  KafkaHistoStreamDecoder &operator=(const KafkaHistoStreamDecoder &) = delete;

public:
  ///@name Start/stop
  ///@{
  void startCapture(bool startNow = true);
  void stopCapture();
  ///@}

  ///@name Querying
  ///@{
  bool isCapturing() const { return m_capturing; }
  bool hasData() const;
  int runNumber() const { return 1; }
  bool hasReachedEndOfRun() { return !m_capturing; }
  ///@}

  ///@name Modifying
  ///@{
  API::Workspace_sptr extractData();
  ///@}

private:
  void captureImpl();
  void captureImplExcept();
  API::Workspace_sptr extractDataImpl();

  DataObjects::Workspace2D_sptr createBufferWorkspace();

  /// Broker to use to subscribe to topics
  std::shared_ptr<IKafkaBroker> m_broker;
  /// Topic name
  const std::string m_histoTopic;
  /// Instrument name
  const std::string m_instrumentName;
  /// Subscriber for the histo stream
  std::unique_ptr<IKafkaStreamSubscriber> m_histoStream;
  /// Workspace used as template for workspaces created when extracting
  DataObjects::Workspace2D_sptr m_workspace;
  /// Buffer for latest FlatBuffers message
  std::string m_buffer;

  /// Associated thread running the capture process
  std::thread m_thread;
  /// Mutex protecting histo buffers
  mutable std::mutex m_buffer_mutex;
  /// Flag indicating if user interruption has been requested
  std::atomic<bool> m_interrupt;
  /// Flag indicating that the decoder is capturing
  std::atomic<bool> m_capturing;
  /// Exception object indicating there was an error
  boost::shared_ptr<std::runtime_error> m_exception;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_ISISKAFKAHISTOSTREAMDECODER_H_ */
