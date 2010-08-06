//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadCanSAS1D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AlgorithmFactory.h"

#include "Poco/Path.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/Node.h"
#include "Poco/DOM/Text.h"

#include <boost/lexical_cast.hpp>
//-----------------------------------------------------------------------

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;
using Poco::XML::Node;
using Poco::XML::Text;

using namespace Mantid::Kernel;

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadCanSAS1D)

/// constructor
LoadCanSAS1D::LoadCanSAS1D()
{}

/// destructor
LoadCanSAS1D::~LoadCanSAS1D()
{}

/// Overwrites Algorithm Init method.
void LoadCanSAS1D::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".xml"),
      "The name of the input  xml file to load");
  declareProperty(new API::WorkspaceProperty<API::Workspace>("OutputWorkspace", "",
      Kernel::Direction::Output), "The name of the Output workspace");

}

/// Overwrites Algorithm exec method
void LoadCanSAS1D::exec()
{
  std::string fileName = getPropertyValue("Filename");
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(fileName);
  } catch (...)
  {
    throw Kernel::Exception::FileError("Unable to parse File:", fileName);
  }
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes())
  {
    throw Kernel::Exception::NotFoundError("No root element in CanSAS1D XML file", fileName);
  }
  Element* sasEntryElem = pRootElem->getChildElement("SASentry");
  throwException(sasEntryElem, "SASentry", fileName);
  Element*titleElem = sasEntryElem->getChildElement("Title");
  throwException(titleElem, "Title", fileName);
  std::string wsTitle = titleElem->innerText();

  Element* sasDataElem = sasEntryElem->getChildElement("SASdata");
  throwException(sasDataElem, "SASdata", fileName);
  // getting number of Idata elements in the xml file
  NodeList* idataElemList = sasDataElem->getElementsByTagName("Idata");
  unsigned long idataCount = idataElemList->length();
  //no.of bins
  int nBins = idataCount;
  const int numSpectra = 1;
  // Create the output workspace
  DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra, nBins, nBins));
  ws->setTitle(wsTitle);
  ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("MomentumTransfer");
  ws->setYUnit("");
  API::Workspace_sptr workspace = boost::static_pointer_cast<API::Workspace>(ws);
  setProperty("OutputWorkspace", workspace);

  createRunNumLog(sasEntryElem, "Run", ws, fileName);

  //load workspace data
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  int vecindex = 0;
  //iterate through each Idata element  and get the values of "Q",
  //"I" and "Idev" text nodes and fill X,Y,E vectors
  for (unsigned long index = 0; index < idataCount; ++index)
  {
    Node* idataElem = idataElemList->item(index);
    Element* elem = dynamic_cast<Element*> (idataElem);
    if (elem)
    {
      //setting X vector
      std::string nodeVal;
      Element*qElem = elem->getChildElement("Q");
      throwException(qElem, "Q", fileName);
      nodeVal = qElem->innerText();
      std::stringstream x(nodeVal);
      double d;
      x >> d;
      X[vecindex] = d;

      //setting Y vector
      Element*iElem = elem->getChildElement("I");
      throwException(qElem, "I", fileName);
      nodeVal = iElem->innerText();
      std::stringstream y(nodeVal);
      y >> d;
      Y[vecindex] = d;

      //setting the error vector
      Element*idevElem = elem->getChildElement("Idev");
      throwException(qElem, "Idev", fileName);
      nodeVal = idevElem->innerText();
      std::stringstream e(nodeVal);
      e >> d;
      E[vecindex] = d;
      ++vecindex;
    }

  }
  idataElemList->release();

  Element * instrElem = sasEntryElem->getChildElement("SASinstrument");
  throwException(instrElem, "SASinstrument", fileName);
  std::string instname;
  Element*nameElem = instrElem->getChildElement("name");
  throwException(nameElem, "name", fileName);
  instname = nameElem->innerText();
  // run load instrument
  runLoadInstrument(instname, ws);

}

/** Run the sub-algorithm LoadInstrument (as for LoadRaw)
 * @param inst_name The name written in the Nexus file
 * @param localWorkspace The workspace to insert the instrument into
 */
void LoadCanSAS1D::runLoadInstrument(const std::string & inst_name,
    DataObjects::Workspace2D_sptr localWorkspace)
{
  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = Kernel::ConfigService::Instance().getString(
      "instrumentDefinition.directory");
  if (directoryName.empty())
  {
    // This is the assumed deployment directory for IDFs, where we need to be relative to the
    // directory of the executable, not the current working directory.
    directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
        "../Instrument").toString();
  }

  // For Nexus Mantid processed, Instrument XML file name is read from nexus 
  std::string instrumentID = inst_name;
  // force ID to upper case
  std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
  std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

  API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    loadInst->setPropertyValue("Filename", fullPathIDF);
    loadInst->setProperty<API::MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->execute();
  } catch (std::invalid_argument&)
  {
    g_log.information("Invalid argument to LoadInstrument sub-algorithm");
  } catch (std::runtime_error&)
  {
    g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
  }

}

/* This method throws not found error if a element is not found in the xml file
 * @param elem pointer to  element
 * @param name  element name
 * @param fileName xml file name
 */
void LoadCanSAS1D::throwException(Poco::XML::Element* elem, const std::string & name,
    const std::string& fileName)
{
  if (!elem)
  {
    throw Kernel::Exception::NotFoundError(name + " element not found in CanSAS1D XML file", fileName);
  }
}
/** Reads in an element from the xml file and stores it in the samples log
*  @param[in] sasEntry the data in loaded from the SASentry element
*  @param[in] elemName the name of the element that holds the run number, normally "Run"
*  @param[out] wSpace the log will be created in this workspace
*  @param[in] fileName name of the file from which the run number was read, only used for error reporting
*  @throw NotFoundError if there is no xml element with the name eleName
*/
void LoadCanSAS1D::createRunNumLog(const Poco::XML::Element * const sasEntry, const std::string& elemName, DataObjects::Workspace2D_sptr wSpace, const std::string& fileName)
{
  Element * runText = sasEntry->getChildElement(elemName);

  throwException(runText, elemName, fileName);

  API::Run &run = wSpace->mutableRun();
  run.addLogData(new PropertyWithValue<std::string>("run_number", runText->innerText()));
}

}
}
