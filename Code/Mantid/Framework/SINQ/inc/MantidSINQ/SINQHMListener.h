/*
 * SINQHMListener.h
 *
 * This is a Mantid live data listener for the HTTP based
 * histogram memory  servers used at SINQ, PSI and ANSTO
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 * 
 * Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

 * This file is part of Mantid.

 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

#ifndef SINQHMLISTENER_H_
#define SINQHMLISTENER_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPClientSession.h>

class MANTID_SINQ_DLL SINQHMListener : public Mantid::API::ILiveListener
{
public:
  SINQHMListener();
  ~SINQHMListener();

  std::string name() const {return "SINQHMListener";}
  bool supportsHistory() const {return false;}
  bool buffersEvents() const {return false;}

  bool connect(const Poco::Net::SocketAddress& address);
  void start(Mantid::Kernel::DateAndTime startTime = Mantid::Kernel::DateAndTime());
  boost::shared_ptr<Mantid::API::Workspace> extractData();
  bool isConnected();
  ILiveListener::RunStatus runStatus();
  int runNumber() const { return 0; }

  void setSpectra(const std::vector<Mantid::specid_t>& specList);

private:
  Poco::Net::HTTPClientSession httpcon;
  Poco::Net::HTTPResponse response;
  bool connected;
  bool dimDirty;
  int rank;
  int dim[3]; // @SINQ we only do 3D HM's, change when more dimensions
  std::string hmhost;

  std::istream& httpRequest(std::string path);
  void loadDimensions();
  void doSpecialDim();
  void readHMData(Mantid::API::IMDHistoWorkspace_sptr ws);
  void recurseDim(int *data, Mantid::API::IMDHistoWorkspace_sptr ws, int currentDim, Mantid::coord_t *idx);
  int calculateCAddress(Mantid::coord_t *pos);

  ILiveListener::RunStatus oldStatus;
};

#endif /* SINQHMLISTENER_H_ */
