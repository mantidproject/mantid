//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadCanSAS1D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/LoadAlgorithmFactory.h"

#include <Poco/Path.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

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

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadCanSAS1D)

//register the algorithm into loadalgorithm factory
DECLARE_LOADALGORITHM(LoadCanSAS1D)

/// constructor
LoadCanSAS1D::LoadCanSAS1D() : m_groupNumber(0)
{}

/// destructor
LoadCanSAS1D::~LoadCanSAS1D()
{}

/// Overwrites Algorithm Init method.
void LoadCanSAS1D::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".xml"),
      "The name of the input  xml file to load");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
      Kernel::Direction::Output), "The name of the Output workspace");
}

/** Overwrites Algorithm exec method
* @throw FileError if the file isn't valid xml
* @throw NotFoundError if any expected elements couldn't be read
* @throw NotImplementedError if any SASentry doesn't contain exactly one run
*/
void LoadCanSAS1D::exec()
{
  const std::string fileName = getPropertyValue("Filename");
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(fileName);
  } catch (...)
  {
    throw Exception::FileError("Unable to parse File:", fileName);
  }
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes())
  {
    throw Kernel::Exception::NotFoundError("No root element in CanSAS1D XML file", fileName);
  }
  // there can be multiple <SASentry> elements, each one contains a period which will go into a workspace group if there are more than one of them
  NodeList* entryList = pRootElem->getElementsByTagName("SASentry");
  unsigned int numEntries = entryList->length();
  Workspace_sptr outputWork;
  std::string runName;
  switch(numEntries)
  {
    case 0:
      Exception::NotFoundError("No <SASentry>s were found in the file", fileName);
    case 1:
      //the value of the string runName is unused in this case
      outputWork = loadEntry(entryList->item(0), runName);
      break;
    default:
      WorkspaceGroup_sptr group(new WorkspaceGroup);
      for (unsigned int i = 0; i < numEntries; ++i)
      {
        std::string runName;
        MatrixWorkspace_sptr newWork = loadEntry(entryList->item(i), runName);
        appendDataToOutput(newWork, runName, group);
      }
      outputWork = group;
  }
  entryList->release();
  pDoc->release();
  setProperty("OutputWorkspace", outputWork);
}
/** Load an individual "<SASentry>" element into a new workspace
* @param[in] workspaceData points to a "<SASentry>" element
* @param[out] runName the name this workspace should take
* @return dataWS this workspace will be filled with data
* @throw NotFoundError if any expected elements couldn't be read
* @throw NotImplementedError if the entry doesn't contain exactly one run
*/
MatrixWorkspace_sptr LoadCanSAS1D::loadEntry(Poco::XML::Node * const workspaceData, std::string & runName)
{
  Element *workspaceElem = dynamic_cast<Element*>(workspaceData);
  check(workspaceElem, "<SASentry>");
  runName = workspaceElem->getAttribute("name");

  NodeList* runs = workspaceElem->getElementsByTagName("Run");
  if ( runs->length() != 1 )
  {
    throw Exception::NotImplementedError("<SASentry>s containing multiple runs, or no runs, are not currently supported");
  }

  Element* sasDataElem = workspaceElem->getChildElement("SASdata");
  check(sasDataElem, "<SASdata>");
  // getting number of Idata elements in the xml file
  NodeList* idataElemList = sasDataElem->getElementsByTagName("Idata");
  unsigned int nBins = idataElemList->length();

  MatrixWorkspace_sptr dataWS =
    WorkspaceFactory::Instance().create("Workspace2D", 1, nBins, nBins);

  createLogs(workspaceElem, dataWS);

  Element *titleElem = workspaceElem->getChildElement("Title");
  check(titleElem, "<Title>");
  dataWS->setTitle(titleElem->innerText());
  dataWS->isDistribution(true);
  dataWS->setYUnit("");
  
  //load workspace data
  MantidVec& X = dataWS->dataX(0);
  MantidVec& Y = dataWS->dataY(0);
  MantidVec& E = dataWS->dataE(0);
  int vecindex = 0;
  //iterate through each Idata element  and get the values of "Q",
  //"I" and "Idev" text nodes and fill X,Y,E vectors
  for (unsigned long index = 0; index < nBins; ++index)
  {
    Node* idataElem = idataElemList->item(index);
    Element* elem = dynamic_cast<Element*> (idataElem);
    if (elem)
    {
      //setting X vector
      std::string nodeVal;
      Element*qElem = elem->getChildElement("Q");
      check(qElem, "Q");
      nodeVal = qElem->innerText();
      std::stringstream x(nodeVal);
      double d;
      x >> d;
      X[vecindex] = d;
      
      //setting Y vector
      Element*iElem = elem->getChildElement("I");
      check(qElem, "I");
      nodeVal = iElem->innerText();
      std::stringstream y(nodeVal);
      y >> d;
      Y[vecindex] = d;
      
      //setting the error vector
      Element*idevElem = elem->getChildElement("Idev");
      check(qElem, "Idev");
      nodeVal = idevElem->innerText();
      std::stringstream e(nodeVal);
      e >> d;
      E[vecindex] = d;
      ++vecindex;
    }
  }

  Element * instrElem = workspaceElem->getChildElement("SASinstrument");
  check(instrElem, "SASinstrument");
  std::string instname;
  Element*nameElem = instrElem->getChildElement("name");
  check(nameElem, "name");
  instname = nameElem->innerText();
  // run load instrument
  runLoadInstrument(instname, dataWS);

  idataElemList->release();
  return dataWS;
}
/* This method throws not found error if a element is not found in the xml file
 * @param[in] toCheck pointer to  element
 * @param[in] name element name
*  @throw NotFoundError if the pointer is NULL
 */
void LoadCanSAS1D::check(const Poco::XML::Element* const toCheck, const std::string & name) const
{
  if( !toCheck )
  {
    std::string fileName = getPropertyValue("Filename");
    throw Kernel::Exception::NotFoundError("<"+name+"> element not found in CanSAS1D XML file", fileName);
  }
}
/** Appends the first workspace to the second workspace. The second workspace will became a group unless
*  it was initially empty, in that situation it becames a copy of the first workspace
* @param[in] newWork the new data to add
* @param[in] newWorkName the name that the new workspace will take
* @param[out] container the data will be added to this group
* @throw ExistsError if a workspace with this name had already been added
*/
void LoadCanSAS1D::appendDataToOutput(API::MatrixWorkspace_sptr newWork, const std::string & newWorkName, API::WorkspaceGroup_sptr container)
{  
  //the name of the property, like the workspace name must be different for each workspace. Add "_run" at the end to stop problems with names like "outputworkspace"
  std::string propName = newWorkName+"_run";
  
  //the following code registers the workspace with the AnalysisDataService and with the workspace group, I'm taking this oone trust I don't know why it's done this way sorry, Steve
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(propName, newWorkName,
                                                         Direction::Output));
  container->add(newWorkName);
  setProperty(propName, newWork);
}
/** Run the sub-algorithm LoadInstrument (as for LoadRaw)
 * @param inst_name :: The name written in the Nexus file
 * @param localWorkspace :: The workspace to insert the instrument into
 */
void LoadCanSAS1D::runLoadInstrument(const std::string & inst_name,
    API::MatrixWorkspace_sptr localWorkspace)
{

  API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    loadInst->setPropertyValue("InstrumentName", inst_name);
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
/** Loads data into the run log
*  @param[in] sasEntry the entry corresponding to the passed workspace
*  @param[in] wSpace the log will be created in this workspace
*/
void LoadCanSAS1D::createLogs(const Poco::XML::Element * const sasEntry, API::MatrixWorkspace_sptr wSpace) const
{
  API::Run &run = wSpace->mutableRun();
  Element * runText = sasEntry->getChildElement("Run");
  check(runText, "Run");
  run.addLogData(new PropertyWithValue<std::string>(
                                      "run_number", runText->innerText()));

  Element * process = sasEntry->getChildElement("SASprocess");
  if (process)
  {
    NodeList* terms = process->getElementsByTagName("term");
    for ( unsigned int i = 0; i < terms->length(); ++i )
    {
      Node* term = terms->item(i);
      Element* elem = dynamic_cast<Element*>(term);
      if (elem)
      {
        const std::string termName = elem->getAttribute("name");
        if ( termName == "user_file" )
        {
          std::string file = elem->innerText();
          run.addLogData(new PropertyWithValue<std::string>("UserFile", file));
          break;
        }
      }
    }
  }
}


/**This method does a quick file check by checking the no.of bytes read nread params and header buffer
 *  @param filePath- path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadCanSAS1D::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
{
  std::string extn=extension(filePath);
  bool bspice2d(false);
  (!extn.compare("xml"))?bspice2d=true:bspice2d=false;

  const char* xml_header="<?xml version=";
  if ( ((unsigned)nread >= strlen(xml_header)) && 
    !strncmp((char*)header.full_hdr, xml_header, strlen(xml_header)) )
  {
  }
  return(bspice2d?true:false);
}

/**checks the file by opening it and reading few lines 
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file 
 */
int LoadCanSAS1D::fileCheck(const std::string& filePath)
{      
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(filePath);
  } 
  catch (...)
  {
    throw Kernel::Exception::FileError("Unable to parse File:", filePath);
  }
  int confidence(0);
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if(pRootElem)
  {
    if(pRootElem->tagName().compare("SASroot") == 0)
    {
      confidence = 80;
    }
  }
  pDoc->release();
  return confidence;

}



}
}
