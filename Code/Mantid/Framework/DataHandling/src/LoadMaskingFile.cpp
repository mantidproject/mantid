/*WIKI*



This algorithm loads masking file to a SpecialWorkspace2D/MaskWorkspace.

The format can be
* XML
* ... ...






*WIKI*/

#include "MantidDataHandling/LoadMaskingFile.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
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

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(LoadMaskingFile)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadMaskingFile::LoadMaskingFile()
  {
    // mMaskWS = NULL;
    // mInstrumentName = "";
    pDoc = NULL;
    pRootElem = NULL;

    return;
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadMaskingFile::~LoadMaskingFile()
  {
    // Auto-generated destructor stub
  }
  
  /// Sets documentation strings for this algorithm
  void LoadMaskingFile::initDocs(){
    this->setWikiSummary("Loads an XML file or calibration file to generate MaskWorkspace.");
    this->setOptionalMessage("");
  }

  /// Initialise the properties
  void LoadMaskingFile::init(){

    // 1. Setup
    std::vector<std::string> instrumentnames;
    instrumentnames.push_back("VULCAN");
    instrumentnames.push_back("POWGEN");
    instrumentnames.push_back("NOMAD");

    // 2. Declare property
    declareProperty("Instrument", "POWGEN", new ListValidator(instrumentnames),
        "Instrument to mask.  If InstrumentName is given, algorithm will take InstrumentName. ");
    declareProperty("InstrumentName", "", "Name of instrument to mask.");
    declareProperty(new FileProperty("InputFile", "", FileProperty::Load, ".xml"),
        "XML file for masking. ");
    // declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "Masking", Direction::Output),
    declareProperty(new WorkspaceProperty<DataObjects::SpecialWorkspace2D>("OutputWorkspace", "Masking", Direction::Output),
        "Output Masking Workspace");

    return;
  }


  /// Run the algorithm
  void LoadMaskingFile::exec(){

    // 1. Load Instrument and create output Mask workspace
    const std::string instrumentname = getProperty("Instrument");
    mInstrumentName = instrumentname;
    std::string anothername = getProperty("InstrumentName");
    if (anothername.size() > 0)
      mInstrumentName = anothername;

    this->intializeMaskWorkspace();
    setProperty("OutputWorkspace",mMaskWS);

    // 2. Parse XML File
    std::string xmlfilename = getProperty("InputFile");
    this->initializeXMLParser(xmlfilename);
    this->parseXML();

    // 3. Translate and set geometry
    g_log.information() << "To Mask: " << std::endl;
    std::vector<int32_t> maskdetids;
    std::vector<int32_t> maskdetidpairsL;
    std::vector<int32_t> maskdetidpairsU;

    // this->bankToDetectors(mask_bankid_single, maskdetids, maskdetidpairsL, maskdetidpairsU); use generalized componentToDetectors()
    this->componentToDetectors(mask_bankid_single, maskdetids);
    this->spectrumToDetectors(mask_specid_single, mask_specid_pair_low, mask_specid_pair_up, maskdetids, maskdetidpairsL, maskdetidpairsU);
    this->detectorToDetectors(mask_detid_single, mask_detid_pair_low, mask_detid_pair_up, maskdetids, maskdetidpairsL, maskdetidpairsU);

    g_log.information() << "To UnMask: " << std::endl;
    std::vector<int32_t> unmaskdetids;
    std::vector<int32_t> unmaskdetidpairsL;
    std::vector<int32_t> unmaskdetidpairsU;

    this->bankToDetectors(unmask_bankid_single, unmaskdetids, unmaskdetidpairsL, unmaskdetidpairsU);
    this->spectrumToDetectors(unmask_specid_single, unmask_specid_pair_low, unmask_specid_pair_up, unmaskdetids, unmaskdetidpairsL, unmaskdetidpairsU);
    this->detectorToDetectors(unmask_detid_single, unmask_detid_pair_low, unmask_detid_pair_up, unmaskdetids, unmaskdetidpairsL, unmaskdetidpairsU);

    // 4. Apply

    this->initDetectors();
    this->processMaskOnDetectors(true, maskdetids, maskdetidpairsL, maskdetidpairsU);
    this->processMaskOnDetectors(false, unmaskdetids, unmaskdetidpairsL, unmaskdetidpairsU);

    return;
  }

  void LoadMaskingFile::initDetectors(){

    // 1. Initialize
    if (mDefaultToUse){
      // Default is to use all detectors
      for (size_t i = 0; i < mMaskWS->getNumberHistograms(); i ++){
        mMaskWS->dataY(i)[0] = 1;
      }
    } else {
      // Default not to use any detectors
      for (size_t i = 0; i < mMaskWS->getNumberHistograms(); i ++){
        mMaskWS->dataY(i)[0] = 0;
      }
    }

    return;
  }

  /*
   *  Mask detectors or Unmask detectors
   *  params:
   *  @ tomask:  true to mask, false to unmask
   */
  void LoadMaskingFile::processMaskOnDetectors(bool tomask, std::vector<int32_t> singledetids, std::vector<int32_t> pairdetids_low,
      std::vector<int32_t> pairdetids_up){

    // 1. Get index map
    detid2index_map* indexmap = mMaskWS->getDetectorIDToWorkspaceIndexMap(true);

    // 2. Mask
    g_log.debug() << "Mask = " << tomask <<  "  Final Single IDs Size = " << singledetids.size() << std::endl;

    for (size_t i = 0; i < singledetids.size(); i ++){
      detid_t detid = singledetids[i];
      detid2index_map::iterator it;
      it = indexmap->find(detid);
      if (it != indexmap->end()){
        size_t index = it->second;
        if (tomask)
          mMaskWS->dataY(index)[0] = 0;
        else
          mMaskWS->dataY(index)[0] = 1;
      } else {
        g_log.error() << "Pixel w/ ID = " << detid << " Cannot Be Located" << std::endl;
      }
    }

    // 3. Mask pairs
    for (size_t i = 0; i < pairdetids_low.size(); i ++){
      g_log.error() << "To Be Implemented Soon For Pair (" << pairdetids_low[i] << ", " << pairdetids_up[i] << "!" << std::endl;
    }

    // 4. Clear
    delete indexmap;

    return;
  }


  /*
   * Convert a component to detectors.  It is a generalized version of bankToDetectors()
   * @params
   *  -
   */
  void LoadMaskingFile::componentToDetectors(std::vector<std::string> componentnames,
      std::vector<int32_t>& detectors){

    Geometry::Instrument_const_sptr minstrument = mMaskWS->getInstrument();

    for (size_t i = 0; i < componentnames.size(); i ++){

      g_log.debug() << "Component name = " << componentnames[i] << std::endl;

      // a) get componenet
      Geometry::IComponent_const_sptr component = minstrument->getComponentByName(componentnames[i]);
      g_log.debug() << "Component ID = " << component->getComponentID() << std::endl;

      // b) component -> component assembly --> children (more than detectors)
      boost::shared_ptr<const Geometry::ICompAssembly> asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);
      std::vector<Geometry::IComponent_const_sptr> children;
      asmb->getChildren(children, true);

      g_log.debug() << "Number of Children = " << children.size() << std::endl;

      int32_t numdets = 0;
      int32_t id_min = 1000000;
      int32_t  id_max = 0;

      for (size_t ic = 0; ic < children.size(); ic++){
        // c) convert component to detector
        Geometry::IComponent_const_sptr child = children[ic];
        Geometry::IDetector_const_sptr det = boost::dynamic_pointer_cast<const Geometry::IDetector>(child);

        if (det){
          int32_t detid = det->getID();
          detectors.push_back(detid);
          numdets ++;
          if (detid < id_min)
            id_min = detid;
          if (detid > id_max)
            id_max = detid;
        }
      }

      g_log.debug() << "Number of Detectors in Children = " << numdets << "  Range = " << id_min << ", " << id_max << std::endl;
    } // for component

    return;
  }

  /*
   * Convert bank to detectors
   */
  void LoadMaskingFile::bankToDetectors(std::vector<std::string> singlebanks,
      std::vector<int32_t>& detectors,
      std::vector<int32_t>& detectorpairslow, std::vector<int32_t>& detectorpairsup){

    for (size_t i = 0; i < singlebanks.size(); i ++){
      g_log.information() << "Bank: " << singlebanks[i] << std::endl;
    }

    Geometry::Instrument_const_sptr minstrument = mMaskWS->getInstrument();

    for (size_t ib = 0; ib < singlebanks.size(); ib++){

        std::vector<Geometry::IDetector_const_sptr> idetectors;

        minstrument->getDetectorsInBank(idetectors, singlebanks[ib]);
        g_log.debug() << "Bank: " << singlebanks[ib] << " has " << idetectors.size() << " detectors" << std::endl;

        // a) get information
        size_t numdets = idetectors.size();
        detid_t detid_first = idetectors[0]->getID();
        detid_t detid_last  = idetectors[idetectors.size()-1]->getID();

        // b) set detectors
        if (detid_first+int32_t(numdets) == detid_last+1 && false){
          // TODO This save-time method is not used at this stage
          g_log.information() << "Using Range of Detectors" << std::endl;

          detectorpairslow.push_back(detid_first);
          detectorpairsup.push_back(detid_last);

        } else {
          g_log.debug() << "Apply 1 by 1  " << "DetID: " << detid_first << ", " << detid_last << std::endl;

          for (size_t i = 0; i < idetectors.size(); i ++){
            Geometry::IDetector_const_sptr det = idetectors[i];
            int32_t detid = det->getID();
            detectors.push_back(detid);
          }

        } // if-else
    } // ENDFOR

    return;
  }

  /*
   * Convert spectrum to detectors
   */
  void LoadMaskingFile::spectrumToDetectors(std::vector<int32_t> singles, std::vector<int32_t> pairslow, std::vector<int32_t> pairsup,
      std::vector<int32_t>& detectors,
      std::vector<int32_t>& detectorpairslow, std::vector<int32_t>& detectorpairsup){

    UNUSED_ARG(detectors)
    UNUSED_ARG(detectorpairslow)
    UNUSED_ARG(detectorpairsup)

    if (singles.size() == 0 && pairslow.size() == 0)
      return;

    g_log.error() << "SpectrumID in XML File (ids) Is Not Supported!  Spectrum IDs" << std::endl;

    for (size_t i = 0; i < singles.size(); i ++){
      g_log.information() << "Not Taking Into Account: Spectrum " << singles[i] << std::endl;
    }
    for (size_t i = 0; i < pairslow.size(); i ++){
      g_log.information() << "Not Taking Into Account: Spectrum " << pairslow[i] << "  To " << pairsup[i] << std::endl;
    }

  }

  /*
   * Convert spectrum to detectors
   */
  void LoadMaskingFile::detectorToDetectors(std::vector<int32_t> singles, std::vector<int32_t> pairslow, std::vector<int32_t> pairsup,
      std::vector<int32_t>& detectors,
      std::vector<int32_t>& detectorpairslow, std::vector<int32_t>& detectorpairsup){
    UNUSED_ARG(detectorpairslow)
    UNUSED_ARG(detectorpairsup)

    /*
    for (size_t i = 0; i < singles.size(); i ++){
      g_log.information() << "Detector " << singles[i] << std::endl;
    }
    for (size_t i = 0; i < pairslow.size(); i ++){
      g_log.information() << "Detector " << pairslow[i] << "  To " << pairsup[i] << std::endl;
    }
    */
    for (size_t i = 0; i < singles.size(); i ++){
      detectors.push_back(singles[i]);
    }
    for (size_t i = 0; i < pairslow.size(); i ++){
      for (int32_t j = 0; j < pairsup[i]-pairslow[i]+1; j ++){
        int32_t detid = pairslow[i]+j;
        detectors.push_back(detid);
      }
      /*
      detectorpairslow.push_back(pairslow[i]);
      detectorpairsup.push_back(pairsup[i]);
      */
    }

    return;
  }

  /*
   * Initalize Poco XML Parser
   */
  void LoadMaskingFile::initializeXMLParser(const std::string & filename)
  {
    // const std::string instName
    std::cout << "Load File " << filename << std::endl;
    const std::string xmlText = Kernel::Strings::loadFile(filename);
    std::cout << "Successfully Load XML File " << std::endl;

    // Set up the DOM parser and parse xml file
    DOMParser pParser;
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
  void LoadMaskingFile::parseXML()
  {
    // 0. Check
    if (!pDoc)
      throw std::runtime_error("Call LoadMaskingFile::initialize() before parseXML.");

    // 1. Parse and create a structure
    NodeList* pNL_type = pRootElem->getElementsByTagName("type");
    g_log.information() << "Node Size = " << pNL_type->length() << std::endl;

    Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
    Poco::XML::Node* pNode = it.nextNode();

    bool tomask = true;
    bool ingroup = false;
    while (pNode){

      const Poco::XML::XMLString value = pNode->innerText();

      if (pNode->nodeName().compare("group") == 0){
        // Node "group"
        ingroup = true;
        // get type
        Poco::XML::NamedNodeMap* att = pNode->attributes();
        Poco::XML::Node* cNode = att->item(0);
        if (cNode->getNodeValue().compare("mask") == 0 || cNode->getNodeValue().compare("notuse") == 0){
          tomask = true;
        } else if (cNode->getNodeValue().compare("unmask") == 0 || cNode->getNodeValue().compare("use") == 0){
          tomask = false;
        } else {
          g_log.error() << "Type (" << cNode->localName() << ") = " << cNode->getNodeValue() << " is not supported!" << std::endl;
        }
        g_log.information() << "Node Group:  child Node Name = " << cNode->localName() << ": " << cNode->getNodeValue() << std::endl;

      } else if (pNode->nodeName().compare("component") == 0){
        // Node "component"
        if (ingroup){
          this->parseComponent(value, tomask);
        } else {
          g_log.error() << "XML File heirachial (component) error!" << std::endl;
        }
        // g_log.information() << "Component: " << value << std::endl;

      } else if (pNode->nodeName().compare("ids") == 0){
        // Node "ids"
        if (ingroup){
          this->parseSpectrumIDs(value, tomask);
        } else {
          g_log.error() << "XML File (ids) heirachial error!" << "  Inner Text = " << pNode->innerText() << std::endl;
        }
        // g_log.information() << "detids: " << value << std::endl;

      } else if (pNode->nodeName().compare("detids") == 0){
        // Node "detids"
        if (ingroup){
          this->parseDetectorIDs(value, tomask);
        } else {
          g_log.error() << "XML File (detids) heirachial error!" << std::endl;
        }

      } else if (pNode->nodeName().compare("detector-masking") == 0){
        // Node "detector-masking".  Check default value
        Poco::XML::NamedNodeMap* att = pNode->attributes();
        if (att->length() > 0){
          Poco::XML::Node* cNode = att->item(0);
          if (cNode->localName().compare("default") == 0){
            if (cNode->getNodeValue().compare("use") == 0){
              mDefaultToUse = true;
            } else {
              mDefaultToUse = false;
            }
          }
        } // if - att-length
      }

      pNode = it.nextNode();
    } // ENDWHILE

    return;
  }

  /*
   * Parse bank IDs (string name)
   * Sample:  bank2
   * params:
   * @valutext:  must be bank name
   */
  void LoadMaskingFile::parseComponent(std::string valuetext, bool tomask){

    // 1. Parse bank out
    /*
    std::vector<std::string> values;
    this->splitString(valuetext, values, "bank");
    if (values.size() <= 1){
      g_log.error() << "Bank information format error!" << std::endl;
      return;
    }
    */

    if (tomask){
      mask_bankid_single.push_back(valuetext);
    } else {
      unmask_bankid_single.push_back(valuetext);
    }

    /*
    for (size_t i = 0; i < singles.size(); i ++){
      g_log.information() << "Bank " << singles[i] << std::endl;
    }
    for (size_t i = 0; i < pairs.size()/2; i ++){
      g_log.information() << "Bank " << pairs[2*i] << "  To " << pairs[2*i+1] << std::endl;
    }
    */

    return;
  }

  /*
   * Parse input string for spectrum ID
   */
  void LoadMaskingFile::parseSpectrumIDs(std::string inputstr, bool tomask){

    g_log.error() << "SpectrumID in XML File (ids) Is Not Supported!  Spectrum IDs: " << inputstr << std::endl;

    // 1. Parse range out
    std::vector<int32_t> singles;
    std::vector<int32_t> pairs;
    this->parseRangeText(inputstr, singles, pairs);

    // 2. Set to data storage
    if (tomask){
      for (size_t i = 0; i < singles.size(); i ++){
        mask_specid_single.push_back(singles[i]);
      }
      for (size_t i = 0; i < pairs.size()/2; i ++){
        mask_specid_pair_low.push_back(pairs[2*i]);
        mask_specid_pair_up.push_back(pairs[2*i+1]);
      }
    } else {
      for (size_t i = 0; i < singles.size(); i ++){
        unmask_specid_single.push_back(singles[i]);
      }
      for (size_t i = 0; i < pairs.size()/2; i ++){
        unmask_specid_pair_low.push_back(pairs[2*i]);
        unmask_specid_pair_up.push_back(pairs[2*i+1]);
      }
    }

    return;
  }

  /*
   * Parse input string for spectrum ID
   */
  void LoadMaskingFile::parseDetectorIDs(std::string inputstr, bool tomask){

    // g_log.information() << "Detector IDs: " << inputstr << std::endl;

    // 1. Parse range out
    std::vector<int32_t> singles;
    std::vector<int32_t> pairs;
    this->parseRangeText(inputstr, singles, pairs);

    // 2. Set to data storage
    if (tomask){
      for (size_t i = 0; i < singles.size(); i ++){
        mask_detid_single.push_back(singles[i]);
      }
      for (size_t i = 0; i < pairs.size()/2; i ++){
        mask_detid_pair_low.push_back(pairs[2*i]);
        mask_detid_pair_up.push_back(pairs[2*i+1]);
      }
    } else {
      for (size_t i = 0; i < singles.size(); i ++){
        unmask_detid_single.push_back(singles[i]);
      }
      for (size_t i = 0; i < pairs.size()/2; i ++){
        unmask_detid_pair_low.push_back(pairs[2*i]);
        unmask_detid_pair_up.push_back(pairs[2*i+1]);
      }
    }

    return;
  }

  /*
   * Parse index range text to singles and pairs
   * Example: 3,4,9-10,33
   */
  void LoadMaskingFile::parseRangeText(std::string inputstr, std::vector<int32_t>& singles, std::vector<int32_t>& pairs){
    // 1. Split ','
    std::vector<std::string> rawstrings;
    this->splitString(inputstr, rawstrings, ",");

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
      this->splitString(strpairs[i], ptemp, "-");
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

  void LoadMaskingFile::splitString(std::string inputstr, std::vector<std::string>& strings, std::string sep){

    // std::vector<std::string> SplitVec;
    boost::split(strings, inputstr, boost::is_any_of(sep), boost::token_compress_on);

    // g_log.information() << "Inside... split size = " << strings.size() << std::endl;

    return;
  }


  /*
   * Initialize the Mask Workspace with instrument
   */
  void LoadMaskingFile::intializeMaskWorkspace(){

    // 1. Execute algorithm LoadInstrument() to a temporary Workspace
    API::Algorithm_sptr childAlg =  createSubAlgorithm("LoadInstrument");
    MatrixWorkspace_sptr tempWS(new DataObjects::Workspace2D());
    childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
    childAlg->setPropertyValue("InstrumentName", mInstrumentName);
    childAlg->setProperty("RewriteSpectraMap", false);
    childAlg->executeAsSubAlg();

    if (!childAlg->isExecuted())
    {
      g_log.error() << "Unable to load Instrument " << mInstrumentName << std::endl;
      throw std::invalid_argument("Incorrect instrument name given!");
    }

    // 2. Use the instrument in the temp Workspace for new MaskWorkspace
    Geometry::Instrument_const_sptr minstrument = tempWS->getInstrument();
    API::MatrixWorkspace_sptr mm = API::MatrixWorkspace_sptr(new DataObjects::SpecialWorkspace2D(minstrument));
    mMaskWS = mm;
    mMaskWS->setTitle("Mask");

    return;
  }


} // namespace Mantid
} // namespace DataHandling

