// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_KAFKAEVENTLISTENER_H_
#define MANTID_LIVEDATA_KAFKAEVENTLISTENER_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LiveListener.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace Mantid {

namespace API {
class IAlgorithm;
}

namespace LiveData {
class KafkaEventStreamDecoder;

/**
  Implementation of a live listener to consume messages from the Kafka system
  at ISIS. It currently parses the events directly using flatbuffers so will
  need updating if the schema changes.
 */
class DLLExport KafkaEventListener : public API::LiveListener {
public:
  KafkaEventListener();
  ~KafkaEventListener() override = default;

  //----------------------------------------------------------------------
  // Static properties
  //----------------------------------------------------------------------

  /// The name of this listener
  std::string name() const override { return "KafkaEventListener"; }
  /// Does this listener support requests for (recent) past data
  bool supportsHistory() const override { return true; }
  /// Does this listener buffer events (true) or histogram data (false)
  bool buffersEvents() const override { return true; }

  //----------------------------------------------------------------------
  // Actions
  //----------------------------------------------------------------------

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(
      Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  boost::shared_ptr<API::Workspace> extractData() override;
  void setAlgorithm(const Mantid::API::IAlgorithm &callingAlgorithm) override;

  //----------------------------------------------------------------------
  // State flags
  //----------------------------------------------------------------------
  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

  bool dataReset() override;

private:
  std::unique_ptr<KafkaEventStreamDecoder> m_decoder = nullptr;
  std::string m_instrumentName;
};

} // namespace LiveData
} // namespace Mantid

#endif /*MANTID_LIVEDATA_KAFKAEVENTLISTENER_H_*/
