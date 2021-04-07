// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ILiveListener.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid {
namespace LiveData {
/** An implementation of ILiveListener for testing purposes that gives back a
   buffer
    with an identical number of events every time extractData is called.
 */
class TestGroupDataListener : public API::ILiveListener {
public:
  TestGroupDataListener();

  std::string name() const override { return "TestDataListener"; }
  bool supportsHistory() const override { return false; }
  bool buffersEvents() const override { return true; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  std::shared_ptr<API::Workspace> extractData() override;

  bool isConnected() override;
  bool dataReset() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

  void setSpectra(const std::vector<specnum_t> &) override;
  void setAlgorithm(const class Mantid::API::IAlgorithm &callingAlgorithm) override;

private:
  API::WorkspaceGroup_sptr m_buffer;

  void createWorkspace();
};

} // namespace LiveData
} // namespace Mantid
