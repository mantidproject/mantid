/*
 * SINQHMListener.h
 *
 * This is a Mantid live data listener for the HTTP based
 * histogram memory  servers used at SINQ, PSI and ANSTO
 *
 *  Created on: Nov 14, 2012
 *      Author: Mark.Koennecke@psi.ch
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
