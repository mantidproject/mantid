//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadCanSAS1D2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidDataObjects/Workspace2D.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>

#include <boost/lexical_cast.hpp>
//-----------------------------------------------------------------------

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;
using Poco::XML::Node;
using Poco::XML::Text;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadCanSAS1D2)

/// constructor
LoadCanSAS1D2::LoadCanSAS1D2() : LoadCanSAS1D() {}

/// destructor
LoadCanSAS1D2::~LoadCanSAS1D2() {}

/// Overwrites Algorithm Init method.
void LoadCanSAS1D2::init() {
  LoadCanSAS1D::init();
  declareProperty(
      new PropertyWithValue<bool>("LoadTransmission", false, Direction::Input),
      "Load the transmission related data from the file if it is present "
      "(optional, default False).");
}

void LoadCanSAS1D2::exec() {
  LoadCanSAS1D::exec();
  bool loadTrans = getProperty("LoadTransmission");
  if (!loadTrans)
    return; // all done. It is not to load the transmission, nor check if it
            // exists.
  if (trans_gp.size() == 0 && trans_can_gp.size() == 0) {
    return; // all done, not transmission inside
  }

  std::string out_wsname = this->getProperty("OutputWorkspace");
  processTransmission(trans_gp, "sample", out_wsname);
  processTransmission(trans_can_gp, "can", out_wsname);
}

/**
   Process the Transmission workspaces in order to output them to Mantid.
   @param trans_gp: A vector with the pointers to the workspaces related to the
   transmission
   @param name: Name of the transmission. Only two names are allowed:
   sample/can.
   @param output_name: The name of the OutputWorkspace, in order to create the
   workspaces with similar names.
 */
void
LoadCanSAS1D2::processTransmission(std::vector<MatrixWorkspace_sptr> &trans_gp,
                                   const std::string &name,
                                   const std::string &output_name) {

  std::string trans_wsname =
      std::string(output_name).append("_trans_").append(name);
  const std::string fileName = getPropertyValue("Filename");

  std::string propertyWS;
  if (name == "sample")
    propertyWS = "TransmissionWorkspace";
  else
    propertyWS = "TransmissionCanWorkspace";
  const std::string doc = "The transmission workspace";

  if (trans_gp.size() == 1) {
    MatrixWorkspace_sptr WS = trans_gp[0];
    WS->mutableRun().addProperty("Filename", fileName);
    declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                        propertyWS, trans_wsname, Direction::Output),
                    doc);

    setProperty(propertyWS, WS);
  } else if (trans_gp.size() > 1) {
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    for (unsigned int i = 0; i < trans_gp.size(); i++) {
      MatrixWorkspace_sptr newWork = trans_gp[i];

      newWork->mutableRun().addProperty("Filename", fileName);
      std::stringstream pname;
      std::stringstream name;
      pname << propertyWS << i;
      name << trans_wsname << i;
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                          pname.str(), name.str(), Direction::Output),
                      doc);
      setProperty(pname.str(), newWork);
      group->addWorkspace(newWork);
    }
    std::string pname = std::string(propertyWS).append("GP");
    declareProperty(new WorkspaceProperty<WorkspaceGroup>(pname, trans_wsname,
                                                          Direction::Output),
                    doc);
    setProperty(pname, group);
  }
}

/** Load an individual "<SASentry>" element into a new workspace. It extends the
*LoadCanSAS1D
* in the direction of loading the SAStransmission_spectrum as well. (which was
*introduced in version 1.1)
*
* @param[in] workspaceData points to a "<SASentry>" element
* @param[out] runName the name this workspace should take
* @return dataWS this workspace will be filled with data
* @throw NotFoundError if any expected elements couldn't be read
* @throw NotImplementedError if the entry doesn't contain exactly one run
*/
MatrixWorkspace_sptr
LoadCanSAS1D2::loadEntry(Poco::XML::Node *const workspaceData,
                         std::string &runName) {
  MatrixWorkspace_sptr main_out =
      LoadCanSAS1D::loadEntry(workspaceData, runName);
  bool loadTrans = getProperty("LoadTransmission");
  if (!loadTrans)
    return main_out; // all done. It is not to load the transmission, nor check
                     // if it exists.

  Element *workspaceElem = dynamic_cast<Element *>(workspaceData);
  //  check(workspaceElem, "<SASentry>"); // already done at
  //  LoadCanSAS1D::loadEntry
  Poco::AutoPtr<NodeList> sasTransList =
      workspaceElem->getElementsByTagName("SAStransmission_spectrum");
  if (!sasTransList->length()) {
    g_log.warning() << "There is no transmission data for this file "
                    << getPropertyValue("Filename") << std::endl;
    return main_out;
  }

  for (unsigned short trans_index = 0; trans_index < sasTransList->length();
       trans_index++) {
    // foreach SAStransmission_spectrum
    Node *idataElem = sasTransList->item(trans_index);
    Element *sasTrasElem = dynamic_cast<Element *>(idataElem);
    if (!sasTrasElem)
      continue;
    std::vector<API::MatrixWorkspace_sptr> &group =
        (sasTrasElem->getAttribute("name") == "sample") ? trans_gp
                                                        : trans_can_gp;

    // getting number of Tdata elements in the xml file
    Poco::AutoPtr<NodeList> tdataElemList =
        sasTrasElem->getElementsByTagName("Tdata");
    size_t nBins = tdataElemList->length();

    MatrixWorkspace_sptr dataWS =
        WorkspaceFactory::Instance().create("Workspace2D", 1, nBins, nBins);

    createLogs(workspaceElem, dataWS);

    std::string title = main_out->getTitle();
    title += ":trans";
    title += sasTrasElem->getAttribute("name");
    dataWS->setTitle(title);
    dataWS->isDistribution(true);
    dataWS->setYUnit("");

    // load workspace data
    MantidVec &X = dataWS->dataX(0);
    MantidVec &Y = dataWS->dataY(0);
    MantidVec &E = dataWS->dataE(0);
    int vecindex = 0;
    // iterate through each Tdata element  and get the values of "Lambda",
    //"T" and "Tdev" text nodes and fill X,Y,E vectors
    for (unsigned long index = 0; index < nBins; ++index) {
      Node *idataElem = tdataElemList->item(index);
      Element *elem = dynamic_cast<Element *>(idataElem);
      if (elem) {
        // setting X vector
        std::string nodeVal;
        Element *qElem = elem->getChildElement("Lambda");
        check(qElem, "Lambda");
        nodeVal = qElem->innerText();
        std::stringstream x(nodeVal);
        double d;
        x >> d;
        X[vecindex] = d;

        // setting Y vector
        Element *iElem = elem->getChildElement("T");
        check(qElem, "T");
        nodeVal = iElem->innerText();
        std::stringstream y(nodeVal);
        y >> d;
        Y[vecindex] = d;

        // setting the error vector
        Element *idevElem = elem->getChildElement("Tdev");
        check(qElem, "Tdev");
        nodeVal = idevElem->innerText();
        std::stringstream e(nodeVal);
        e >> d;
        E[vecindex] = d;
        ++vecindex;
      }
    }

    runLoadInstrument(main_out->getInstrument()->getName(), dataWS);
    dataWS->getAxis(0)->setUnit("Wavelength");

    // add to group
    group.push_back(dataWS);
  }
  return main_out;
}
}
}
