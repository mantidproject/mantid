/*
 * SINQHMListener.cpp
 *
 *  Created on: Nov 14, 2012
 *      Author: mark.koennecke@psi.ch
 */
#include "MantidSINQ/SINQHMListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPBasicCredentials.h>
#include <Poco/StreamCopier.h>
#include <iostream>
#include <sstream>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <unistd.h>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Poco::Net;
using namespace Poco::XML;

DECLARE_LISTENER(SINQHMListener)


SINQHMListener::SINQHMListener() :ILiveListener(), httpcon(), response()
{
	connected = false;
	dimDirty = true;
}

SINQHMListener::~SINQHMListener()
{

}

bool SINQHMListener::connect(const Poco::Net::SocketAddress& address)
{
    std::string host = address.toString();
    unsigned int i = host.find(':');
    if ( i != std::string::npos )
    {
      host.erase( i );
    }
    httpcon.setHost(host);
    httpcon.setPort(address.port());
    httpcon.setKeepAlive(true);
    connected = true;
	return true;
}

bool SINQHMListener::isConnected()
{
	return connected;
}

ILiveListener::RunStatus SINQHMListener::runStatus()
{
  std::istream& istr = httpRequest("/admin/textstatus.egi");
  std::stringstream oss;
  Poco::StreamCopier::copyStream(istr,oss);

  std::map<std::string,std::string> daq;
  std::string line, key, value;
  std::istringstream daqdata(oss.str());
  while(std::getline(daqdata,line)){
	  std::istringstream l(line);
	  std::getline(l,key,':');
	  std::getline(l,value,'\n');
	  boost::algorithm::trim(key);
	  boost::algorithm::trim(value);
	  daq[key] = value;
  }

  hmhost = daq["HM-Host"];

  /**
   * Not only set the RunStatus but also set the dimDirty flag when
   * changing from NoRun to Running. The HM may have been reconfigured...
   */
  std::istringstream i(daq["DAQ"]);
  int status;
  i >> status;
  if(status == 1) {
	  if(oldStatus == NoRun){
		  dimDirty = true;
	  }
	  oldStatus = Running;
	  return Running;
  } else if (status == 0) {
	  oldStatus = NoRun;
	  return NoRun;
  } else {
	  throw  std::runtime_error("Invalid DAQ status code " + daq["DAQ"] + "detected");
  }

}

boost::shared_ptr<Workspace> SINQHMListener::extractData()
{
	static char *dimNames[] = {"x","y","z","t"};

	if(dimDirty){
		runStatus(); //make sure that hmhost is initialized
		loadDimensions();
	}

    std::vector<MDHistoDimension_sptr> dimensions;
    for(int i = 0; i < rank; i++){
    	dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(dimNames[i], dimNames[i], "", .0, double(dim[i]), dim[i])));
    }
    MDHistoWorkspace_sptr ws (new MDHistoWorkspace(dimensions));
    ws->setTo(.0,.0,.0);

    readHMData(ws);

    return ws;
}

void SINQHMListener::setSpectra(const std::vector<Mantid::specid_t>& /*specList*/)
{
	/**
	 * Nothing to do: we always go for the full data.
	 * SINQHM would do subsampling but this cannot easily
	 * be expressed as a spectra map.
	 */
}

void SINQHMListener::start(Mantid::Kernel::DateAndTime /*startTime */)
{
	// Nothing to do here
}

void SINQHMListener::loadDimensions()
{
	  std::istream& istr = httpRequest("/sinqhm.xml");
	  std::stringstream oss;
	  Poco::StreamCopier::copyStream(istr,oss);
	  //std::cout << oss.str() << std::endl;

	  DOMParser xmlParser;
	  Document *doc;
	  try{
		  doc = xmlParser.parseString(oss.str());
	  } catch (...)
	  {
	    throw std::runtime_error("Unable to parse sinqhm.xml");
	  }
	  Element *root = doc->documentElement();
	  NodeList *bankList = root->getElementsByTagName("bank");
	  /**
	   * TODO: There may be multiple banks but I only
	   *       look at the first
	   */
	  Element *bank = dynamic_cast<Element*>(bankList->item(0));
	  std::string rankt = bank->getAttribute("rank");
	  rank = atoi(rankt.c_str());

	  NodeList *axisList = bank->getElementsByTagName("axis");
	  for(unsigned int i = 0; i < axisList->length(); i++){
		  Element *axis = dynamic_cast<Element*>(axisList->item(i));
		  std::string sdim = axis->getAttribute("length");
		  dim[i] = atoi(sdim.c_str());
	  }

//	  std::cout << "Dims after parsing sinqhm.xml: rank = " << rank << " dims =";
//	  for(int i = 0; i < rank; i++){
//		  std::cout << dim[i] << ", ";
//	  }
//	  std::cout << std::endl;

	  doSpecialDim();

//	  std::cout << "Dims after doSpecialDim: rank = " << rank << " dims =";
//	  for(int i = 0; i < rank; i++){
//		  std::cout << dim[i] << ", ";
//	  }
//	  std::cout << std::endl;

}

std::istream& SINQHMListener::httpRequest(std::string path)
{
	std::cout << path << std::endl;

	HTTPRequest req(HTTPRequest::HTTP_GET,path, HTTPMessage::HTTP_1_1);
    req.setKeepAlive(true);
	HTTPBasicCredentials cred("spy","007");
	cred.authenticate(req);
	httpcon.sendRequest(req);
	std::istream& istr = httpcon.receiveResponse(response);
	if(response.getStatus() != HTTPResponse::HTTP_OK){
		throw  std::runtime_error("Failed to get " + path + " with reason " + response.getReason());
	}
	return istr;
}

/**
 * This makes some special dimension adaptions for HMs at SINQ. Especially it helps
 * at SANS where the really 2D HM is treated as a very long 1D. If you use this code
 * at a facility which is not SINQ, nuke the code. And takes the biggest nuke you can find....
 */
void SINQHMListener::doSpecialDim()
{
	if(hmhost == "sanshm" && rank == 1){
		rank = 2;
		dim[0] = 128;
		dim[1] = 128;
	}
}
int SINQHMListener::calculateCAddress(coord_t *pos)
{
	int result, mult;
	int i, j;

	result = (int)pos[rank - 1];
	for(i = 0; i < rank -1; i++){
		mult = 1;
		for(j = rank -1; j > i; j--){
			mult *= dim[j];
		}
		if((int)pos[i] < dim[i] && (int)pos[i] > 0){
			result += mult*(int)pos[i];
		}
	}
	return result;
}
void SINQHMListener::recurseDim(int *data, IMDHistoWorkspace_sptr ws, int currentDim, coord_t *idx)
{
	if(currentDim == rank){
		int Cindex = calculateCAddress(idx);
		int val = data[Cindex];
		MDHistoWorkspace_sptr mdws = boost::dynamic_pointer_cast<MDHistoWorkspace>(ws);
		unsigned int F77index = mdws->getLinearIndexAtCoord(idx);
		mdws->setSignalAt(F77index,signal_t(val));
		mdws->setErrorSquaredAt(F77index,signal_t(val));
	} else {
		for(int i = 0; i < dim[currentDim]; i++){
			idx[currentDim] = i;
			recurseDim(data,ws,currentDim+1, idx);
		}
	}
}

void SINQHMListener::readHMData(IMDHistoWorkspace_sptr ws)
{
	int val, *data = NULL, length = 1;
	coord_t *idx;
	long dataSum = 0;

	for(int i = 0; i < rank; i++){
		length *= dim[i];
	}
	char pathBuffer[132];
	snprintf(pathBuffer,sizeof(pathBuffer),"/admin/readhmdata.egi?bank=0&start=0&end=%d", length);
	std::istream& istr = httpRequest(std::string(pathBuffer));

	//std::cout << "Content-length " << response.getContentLength() << ", Content-type: " << response.getContentType() << std::endl;

	data = (int *)malloc(length*sizeof(int));
	if(data == NULL){
		throw std::runtime_error("Out of memory reading HM data");
	}
	istr.read((char *)data,length*sizeof(int));
	if(!istr.good()){
		std::cout << "Encountered Problem before reading all SINQHM data" << std::endl;
	}
	for(int i = 0; i < length ; i++){
		data[i] = ntohl(data[i]);
		dataSum += data[i];
	}
    //std::cout << "DataSum = " << dataSum  << std::endl;

	/**
	 * recurseDim also takes care of converting from C to F77 storage order. Because
	 * Mantid MD arrays are in F77 storage order. Only the holy cucumber knows why....
	 */
	idx = (coord_t*)malloc(rank*sizeof(coord_t));
	recurseDim(data, ws, 0, idx);

	free(data);
	free(idx);
//	httpcon.reset();
//	httpcon.abort();
}
