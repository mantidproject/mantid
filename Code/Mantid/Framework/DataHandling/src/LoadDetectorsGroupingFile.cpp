/*WIKI*



This algorithm loads an XML file containing grouping information to
a GroupingWorkspace.


*WIKI*/
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"

#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(LoadDetectorsGroupingFile)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadDetectorsGroupingFile::LoadDetectorsGroupingFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadDetectorsGroupingFile::~LoadDetectorsGroupingFile()
  {
  }
  
  /// Sets documentation strings for this algorithm
  void LoadDetectorsGroupingFile::initDocs(){
    this->setWikiSummary("Loads an XML file to generate GroupingWorkspace.");
    this->setOptionalMessage("");
  }

  /// Initialise the properties
  void LoadDetectorsGroupingFile::init(){
    declareProperty(new FileProperty("InputFile", "", FileProperty::Load, ".xml"),
        "XML file for masking");
    declareProperty(new WorkspaceProperty<DataObjects::GroupingWorkspace>("OutputWorkspace", "", Direction::Output),
        "Output Masking Workspace");

    return;
  }


  /// Run the algorithm
  void LoadDetectorsGroupingFile::exec(){

    // 1. Parse XML File
    std::string xmlfilename = getProperty("InputFile");
    this->initializeXMLParser(xmlfilename);
    this->parseXML();

    // 2. Load Instrument and create output Mask workspace
    this->intializeGroupingWorkspace();
    setProperty("OutputWorkspace", mGroupWS);

    // 3. Translate and set geometry
    this->setByComponents();
    this->setByDetectors();

    // 4. Apply
    /*
    this->processGroupingDetectors(maskdetids, maskdetidpairsL, maskdetidpairsU);
    */

    return;
  }

  /*
   * Convert Componenet -> Detector IDs -> Workspace Indices -> set group ID
   */
  void LoadDetectorsGroupingFile::setByComponents(){

    // 1. Prepare
    Geometry::Instrument_const_sptr minstrument = mGroupWS->getInstrument();
    detid2index_map* indexmap = mGroupWS->getDetectorIDToWorkspaceIndexMap(true);

    // 2. Set
    for (std::map<int, std::vector<std::string> >::iterator it =  mGroupComponentsMap.begin();
        it != mGroupComponentsMap.end(); ++it)
    {
      g_log.notice() << "Group ID = " << it->first << std::endl;

      for (size_t i = 0; i < it->second.size(); i ++){

        // a) get component
        Geometry::IComponent_const_sptr component = minstrument->getComponentByName(it->second[i]);
        g_log.debug() << "Component Name = " << it->second[i] << "  Component ID = " << component->getComponentID() << std::endl;

        // b) component -> component assembly --> children (more than detectors)
        boost::shared_ptr<const Geometry::ICompAssembly> asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);
        std::vector<Geometry::IComponent_const_sptr> children;
        asmb->getChildren(children, true);

        g_log.debug() << "Number of Children = " << children.size() << std::endl;

        for (size_t ic = 0; ic < children.size(); ic++){
          // c) convert component to detector
          Geometry::IComponent_const_sptr child = children[ic];
          Geometry::IDetector_const_sptr det = boost::dynamic_pointer_cast<const Geometry::IDetector>(child);

          if (det){
            // Component is DETECTOR:
            int32_t detid = det->getID();
            detid2index_map::iterator itx = indexmap->find(detid);
            if (itx != indexmap->end()){
              size_t wsindex = itx->second;
              mGroupWS->dataY(wsindex)[0] = it->first;
            } else {
              g_log.error() << "Pixel w/ ID = " << detid << " Cannot Be Located" << std::endl;
            }
          } // ENDIF Detector

        } // ENDFOR (children of component)
       } // ENDFOR (component)

    } // ENDFOR GroupID

    // 3. Clear
    delete indexmap;

    return;
  }


  /* Set workspace->group ID map by detectors (range)
   *
   */
  void LoadDetectorsGroupingFile::setByDetectors(){

    // 1. Prepare
    Geometry::Instrument_const_sptr minstrument = mGroupWS->getInstrument();
    detid2index_map* indexmap = mGroupWS->getDetectorIDToWorkspaceIndexMap(true);

    // 2. Set GroupingWorkspace
    for (std::map<int, std::vector<detid_t> >::iterator it =  mGroupDetectorsMap.begin();
        it != mGroupDetectorsMap.end(); ++it)
    {
      g_log.notice() << "Group ID = " << it->first << std::endl;

      for (size_t i = 0; i < it->second.size()/2; i ++){
        g_log.notice() << "Detector From = " << it->second[2*i] << ", " << it->second[2*i+1] << std::endl;

        for (detid_t detid = it->second[2*i]; detid <= it->second[2*i+1]; detid ++)
        {
          detid2index_map::iterator itx = indexmap->find(detid);
          if (itx != indexmap->end()){
            size_t wsindex = itx->second;
            mGroupWS->dataY(wsindex)[0] = it->first;
          } else {
            g_log.error() << "Pixel w/ ID = " << detid << " Cannot Be Located" << std::endl;
          }
        } // ENDFOR detid (in range)
      } // ENDFOR each range
    } // ENDFOR each group ID

    // 3. Clear
    delete indexmap;

    return;
  }


  /* Initialize a GroupingWorkspace
   *
   */
  void LoadDetectorsGroupingFile::intializeGroupingWorkspace(){

    g_log.notice() << "Instrument Name = " << mInstrumentName << std::endl;

    // 1. Create Instrument
    Algorithm_sptr childAlg = this->createSubAlgorithm("LoadInstrument");
    MatrixWorkspace_sptr tempWS(new DataObjects::Workspace2D());
    childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
    childAlg->setPropertyValue("InstrumentName", mInstrumentName);
    childAlg->setProperty("RewriteSpectraMap", false);
    childAlg->executeAsSubAlg();
    Geometry::Instrument_const_sptr minstrument = tempWS->getInstrument();

    // 2. Create GroupingWorkspace
    mGroupWS = DataObjects::GroupingWorkspace_sptr(new DataObjects::GroupingWorkspace(minstrument));

    return;
  }

  /*
   * Initalize Poco XML Parser
   */
  void LoadDetectorsGroupingFile::initializeXMLParser(const std::string & filename)
  {
    // const std::string instName
    std::cout << "Load File " << filename << std::endl;
    const std::string xmlText = Kernel::Strings::loadFile(filename);
    std::cout << "Successfully Load XML File " << std::endl;

    // Set up the DOM parser and parse xml file
    Poco::XML::DOMParser pParser;
    try
    {
      pDoc = pParser.parseString(xmlText);
    }
    catch(Poco::Exception& exc)
    {
      throw Kernel::Exception::FileError(exc.displayText() + ". Unable to parse File:", filename);
    }
    catch(...)
    {
      throw Kernel::Exception::FileError("Unable to parse File:" , filename);
    }
    // Get pointer to root element
    pRootElem = pDoc->documentElement();
    if ( !pRootElem->hasChildNodes() )
    {
      g_log.error("XML file: " + filename + "contains no root element.");
      throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument file", filename);
    }
  }

  /*
   * Parse XML file
   */
  void LoadDetectorsGroupingFile::parseXML()
  {
    // 0. Check
    if (!pDoc)
      throw std::runtime_error("Call LoadDetectorsGroupingFile::initialize() before parseXML.");

    // 1. Parse and create a structure
    /*
    NodeList* pNL_type = pRootElem->getElementsByTagName("type");
    g_log.information() << "Node Size = " << pNL_type->length() << std::endl;
    */

    Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
    Poco::XML::Node* pNode = it.nextNode();

    int curgroupid = -1;

    /* While loop to go over all nodes!
     *
     */
    while (pNode){

      const Poco::XML::XMLString value = pNode->innerText();

      if (pNode->nodeName().compare("detector-grouping") == 0){

        // Node "detector-grouping" (first level). Instrument Name
        bool foundname;
        mInstrumentName = getAttributeValueByName(pNode, "instrument", foundname);

        /*
        Poco::XML::NamedNodeMap* att = pNode->attributes();
        bool foundname = false;
        for (size_t i = 0; i < att->length(); i ++){
          Poco::XML::Node* cNode = att->item(i);
          if (cNode->localName().compare("instrument") == 0){
            mInstrumentName = cNode->getNodeValue();
            foundname = true;
            break;
          }
        } // ENDFOR
        */

        if (!foundname){
          g_log.error() << "Cannot find instrument name in node detector-masking!  Quit!" << std::endl;
          throw std::invalid_argument("Cannot find instrument name defined in XML");
        }

      } // "detector-masking"

      else if (pNode->nodeName().compare("group") == 0){

        // Group Node:  get ID, set map
        // a) Find group ID
        bool foundid;
        std::string idstr = getAttributeValueByName(pNode, "ID", foundid);
        if (!foundid){
          g_log.error() << "Node " << pNode->localName() << " has not attribute ID!  Quit!" << std::endl;
          throw std::invalid_argument("Cannot find ID defined for group in XML");
        }

        curgroupid = atoi(idstr.c_str());

        // b) Set in map
        std::map<int, std::vector<std::string> >::iterator itc = mGroupComponentsMap.find(curgroupid);
        if (itc != mGroupComponentsMap.end()){
          // Error! Duplicate Groupd ID defined in XML
          g_log.error() << "Map (group ID, components) has group ID " << curgroupid << " already.  Duplicate Group ID error!" << std::endl;
        } else {
          // Set map
          std::vector<std::string> tempcomponents;
          std::vector<detid_t> tempdetids;
          mGroupComponentsMap[curgroupid] = tempcomponents;
          mGroupDetectorsMap[curgroupid] = tempdetids;
        }

      } // "group"

      else if (pNode->nodeName().compare("component") == 0){

        // Node "component" = value
        std::map<int, std::vector<std::string> >::iterator it = mGroupComponentsMap.find(curgroupid);
        if (it == mGroupComponentsMap.end()){
          g_log.error() << "XML File (component) heirachial error!" << "  Inner Text = " << pNode->innerText() << std::endl;
        } else {
          it->second.push_back(value);
        }

      } // Componenet

      else if (pNode->nodeName().compare("detids") == 0){

        // Node "detids"
        std::map<int, std::vector<detid_t> >::iterator it = mGroupDetectorsMap.find(curgroupid);
        if (it == mGroupDetectorsMap.end()){
          g_log.error() << "XML File (detids) heirachial error!" << "  Inner Text = " << pNode->innerText() << std::endl;
        } else {
          parseDetectorIDs(value, it->second);
        }

      } // "detids"

      // Next Node!
      pNode = it.nextNode();

    } // ENDWHILE

    return;
  }

  /*
   * Get attribute's value by name from a Node
   */
  std::string LoadDetectorsGroupingFile::getAttributeValueByName(Poco::XML::Node* pNode, std::string attributename,
      bool& found){
    // 1. Init
    Poco::XML::NamedNodeMap* att = pNode->attributes();
    found = false;
    std::string value = "";

    // 2. Loop to find
    for (size_t i = 0; i < att->length(); i ++){
      Poco::XML::Node* cNode = att->item(i);
      if (cNode->localName().compare(attributename) == 0){
        value = cNode->getNodeValue();
        found = true;
        break;
      }
    } // ENDFOR

    return value;
  }

  /*
   * Parse a,b-c,d,... string to a vector
   */
  void LoadDetectorsGroupingFile::parseDetectorIDs(std::string inputstring, std::vector<detid_t>& detids){

    // 1. Parse range out
    std::vector<int32_t> singles;
    std::vector<int32_t> pairs;
    parseRangeText(inputstring, singles, pairs);

    // 2. Store single detectors... ..., detx, detx, ...
    for (size_t i = 0; i < singles.size(); i ++){
      detids.push_back(singles[i]);
      detids.push_back(singles[i]);
    }

    // 3. Store detectors pairs
    for (size_t i = 0; i < pairs.size()/2; i ++){
      detids.push_back(pairs[2*i]);
      detids.push_back(pairs[2*i+1]);
    }

    return;
  }

  /*
   * Parse index range text to singles and pairs
   * Example: 3,4,9-10,33
   */
  void LoadDetectorsGroupingFile::parseRangeText(std::string inputstr, std::vector<int32_t>& singles, std::vector<int32_t>& pairs){

    // 1. Split ','
    std::vector<std::string> rawstrings;
    boost::split(rawstrings, inputstr, boost::is_any_of(","), boost::token_compress_on);

    // 2. Filter
    std::vector<std::string> strsingles;
    std::vector<std::string> strpairs;
    for (size_t i = 0; i < rawstrings.size(); i ++){
      // a) Find '-':
      bool containto = false;
      const char* tempchs = rawstrings[i].c_str();
      for (size_t j = 0; j < rawstrings[i].size(); j ++)
        if (tempchs[j] == '-'){
          containto = true;
          break;
        }
      // b) Rebin
      if (containto)
        strpairs.push_back(rawstrings[i]);
      else
        strsingles.push_back(rawstrings[i]);
    } // ENDFOR i

    // 3. Treat singles
    for (size_t i = 0; i < strsingles.size(); i ++){
      int32_t itemp = atoi(strsingles[i].c_str());
      singles.push_back(itemp);
    }

    // 4. Treat pairs
    for (size_t i = 0; i < strpairs.size(); i ++){
      // a) split and check
      std::vector<std::string> ptemp;
      boost::split(ptemp, strpairs[i], boost::is_any_of("-"), boost::token_compress_on);
      if (ptemp.size() != 2){
        g_log.error() << "Range string " << strpairs[i] << " has a wrong format!" << std::endl;
        throw std::invalid_argument("Wrong format");
      }

      // b) parse
      int32_t intstart = atoi(ptemp[0].c_str());
      int32_t intend = atoi(ptemp[1].c_str());
      if (intstart >= intend){
        g_log.error() << "Range string " << strpairs[i] << " has a reversed order" << std::endl;
        throw std::invalid_argument("Wrong format");
      }
      pairs.push_back(intstart);
      pairs.push_back(intend);
    }

    return;
  }



} // namespace Mantid
} // namespace DataHandling
