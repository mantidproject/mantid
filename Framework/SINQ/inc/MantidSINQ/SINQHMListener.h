// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * SINQHMListener.h
 *
 * This is a Mantid live data listener for the HTTP based
 * histogram memory  servers used at SINQ, PSI and ANSTO
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 *
 */

#pragma once

#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/LiveListener.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidSINQ/DllConfig.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPResponse.h>

class MANTID_SINQ_DLL SINQHMListener : public Mantid::API::LiveListener {
public:
  SINQHMListener();

  std::string name() const override { return "SINQHMListener"; }
  bool supportsHistory() const override { return false; }
  bool buffersEvents() const override { return false; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(Mantid::Types::Core::DateAndTime startTime = Mantid::Types::Core::DateAndTime()) override;
  std::shared_ptr<Mantid::API::Workspace> extractData() override;
  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override { return 0; }

  void setSpectra(const std::vector<Mantid::specnum_t> &specList) override;

private:
  Poco::Net::HTTPClientSession httpcon;
  Poco::Net::HTTPResponse response;
  bool connected;
  bool dimDirty;
  int rank;
  int dim[3]; // @SINQ we only do 3D HM's, change when more dimensions
  std::string hmhost;

  std::istream &httpRequest(const std::string &path);
  void loadDimensions();
  void doSpecialDim();
  void readHMData(const Mantid::API::IMDHistoWorkspace_sptr &ws);
  void recurseDim(int *data, const Mantid::API::IMDHistoWorkspace_sptr &ws, int currentDim, Mantid::coord_t *idx);
  int calculateCAddress(Mantid::coord_t *pos);

  ILiveListener::RunStatus oldStatus;
};
