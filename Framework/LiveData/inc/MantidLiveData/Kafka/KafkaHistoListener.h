// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LiveListener.h"
#include "MantidLiveData/DllConfig.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace Mantid {
namespace LiveData {
class KafkaHistoStreamDecoder;

/**
  Implementation of a live listener to consume messages which are in a histogram
  format from the Kafka system at ISIS. It currently parses the histogram data
  directly using flatbuffers so will need updating if the schema changes.
 */
class MANTID_LIVEDATA_DLL KafkaHistoListener : public API::LiveListener {
public:
  KafkaHistoListener();
  ~KafkaHistoListener() override = default;

  //----------------------------------------------------------------------
  // Static properties
  //----------------------------------------------------------------------
  /// The name of this listener
  std::string name() const override { return "KafkaHistoListener"; }
  /// Does this listener support requests for (recent) past data
  bool supportsHistory() const override { return true; }
  /// Does this listener buffer events (true) or histogram data (false)
  bool buffersEvents() const override { return false; }

  void setAlgorithm(const Mantid::API::IAlgorithm &callingAlgorithm) override;

  //----------------------------------------------------------------------
  // Actions
  //----------------------------------------------------------------------
  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  std::shared_ptr<API::Workspace> extractData() override;

  //----------------------------------------------------------------------
  // State flags
  //----------------------------------------------------------------------
  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

private:
  std::unique_ptr<KafkaHistoStreamDecoder> m_decoder = nullptr;
  std::string m_instrumentName;
};

} // namespace LiveData
} // namespace Mantid
