#include "MantidDataHandling/LoadMask.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"

#include <fstream>
#include <sstream>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>

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
using namespace std;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadMask)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadMask::LoadMask(): m_maskWS(), m_instrumentPropValue(""), m_pDoc(NULL),
  m_pRootElem(NULL), m_defaultToUse(true) {
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadMask::~LoadMask() {
  // note Poco::XML::Document and Poco::XML::Element declare their constructors as protected
  if (m_pDoc)
    m_pDoc->release();
  // note that m_pRootElem does not need a release(), and that can
  // actually cause a double free corruption, as
  // Poco::DOM::Document::documentElement() does not require a
  // release(). So just to be explicit that they're gone:
  m_pDoct = NULL;
  m_pRootElem = NULL;
}

/// Initialise the properties
void LoadMask::init() {

  // 1. Declare property
  declareProperty("Instrument", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the instrument to apply the mask.");
  std::vector<std::string> exts;
  exts.push_back(".xml");
  exts.push_back(".msk");
  declareProperty(new FileProperty("InputFile", "", FileProperty::Load, exts),
                  "Masking file for masking. Supported file format is XML and "
                  "ISIS ASCII. ");
  declareProperty(new WorkspaceProperty<DataObjects::MaskWorkspace>(
                      "OutputWorkspace", "Masking", Direction::Output),
                  "Output Masking Workspace");

  return;
}

//----------------------------------------------------------------------------------------------
/** Main execution body of this algorithm
  */
void LoadMask::exec() {
  // 1. Load Instrument and create output Mask workspace
  const std::string instrumentname = getProperty("Instrument");
  m_instrumentPropValue = instrumentname;

  this->intializeMaskWorkspace();
  setProperty("OutputWorkspace", m_maskWS);

  m_defaultToUse = true;

  // 2. Parse Mask File
  std::string filename = getProperty("InputFile");

  if (boost::ends_with(filename, "l") || boost::ends_with(filename, "L")) {
    // 2.1 XML File
    this->initializeXMLParser(filename);
    this->parseXML();
  } else if (boost::ends_with(filename, "k") ||
             boost::ends_with(filename, "K")) {
    // 2.2 ISIS Masking file
    loadISISMaskFile(filename);
    m_defaultToUse = true;
  } else {
    g_log.error() << "File " << filename << " is not in supported format. "
                  << std::endl;
    return;
  }

  // 3. Translate and set geometry
  g_log.information() << "To Mask: " << std::endl;
  std::vector<int32_t> maskdetids;
  std::vector<int32_t> maskdetidpairsL;
  std::vector<int32_t> maskdetidpairsU;

  componentToDetectors(mask_bankid_single, maskdetids);
  detectorToDetectors(mask_detid_single, mask_detid_pair_low,
                      mask_detid_pair_up, maskdetids, maskdetidpairsL,
                      maskdetidpairsU);

  g_log.information() << "To UnMask: " << std::endl;
  std::vector<int32_t> unmaskdetids;
  std::vector<int32_t> unmaskdetidpairsL;
  std::vector<int32_t> unmaskdetidpairsU;

  this->bankToDetectors(unmask_bankid_single, unmaskdetids, unmaskdetidpairsL,
                        unmaskdetidpairsU);
  this->detectorToDetectors(unmask_detid_single, unmask_detid_pair_low,
                            unmask_detid_pair_up, unmaskdetids,
                            unmaskdetidpairsL, unmaskdetidpairsU);

  // 4. Apply
  this->initDetectors();
  this->processMaskOnDetectors(true, maskdetids, maskdetidpairsL,
                               maskdetidpairsU);
  this->processMaskOnWorkspaceIndex(true, mask_specid_pair_low,
                                    mask_specid_pair_up);

  this->processMaskOnDetectors(false, unmaskdetids, unmaskdetidpairsL,
                               unmaskdetidpairsU);

  return;
}

void LoadMask::initDetectors() {

  if (!m_defaultToUse) { // Default is to use all detectors
    size_t numHist = m_maskWS->getNumberHistograms();
    for (size_t wkspIndex = 0; wkspIndex < numHist; wkspIndex++) {
      m_maskWS->setMaskedIndex(wkspIndex);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/**  Mask detectors or Unmask detectors
 *   @param tomask:  true to mask, false to unmask
 *   @param singledetids: list of individual det ids to mask
 *   @param pairdetids_low: list of lower bound of det ids to mask
 *   @param pairdetids_up: list of upper bound of det ids to mask
 */
void LoadMask::processMaskOnDetectors(bool tomask,
                                      std::vector<int32_t> singledetids,
                                      std::vector<int32_t> pairdetids_low,
                                      std::vector<int32_t> pairdetids_up) {
  // 1. Get index map
  const detid2index_map indexmap =
      m_maskWS->getDetectorIDToWorkspaceIndexMap(true);

  // 2. Mask
  g_log.debug() << "Mask = " << tomask
                << "  Final Single IDs Size = " << singledetids.size()
                << std::endl;

  for (size_t i = 0; i < singledetids.size(); i++) {
    detid_t detid = singledetids[i];
    detid2index_map::const_iterator it;
    it = indexmap.find(detid);
    if (it != indexmap.end()) {
      size_t index = it->second;
      if (tomask)
        m_maskWS->dataY(index)[0] = 1;
      else
        m_maskWS->dataY(index)[0] = 0;
    } else {
      g_log.error() << "Pixel w/ ID = " << detid << " Cannot Be Located"
                    << std::endl;
    }
  }

  // 3. Mask pairs
  for (size_t i = 0; i < pairdetids_low.size(); i++) {
    g_log.error() << "To Be Implemented Soon For Pair (" << pairdetids_low[i]
                  << ", " << pairdetids_up[i] << "!" << std::endl;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert a component to detectors.  It is a generalized version of
 * bankToDetectors()
 */
void LoadMask::componentToDetectors(std::vector<std::string> componentnames,
                                    std::vector<int32_t> &detectors) {
  Geometry::Instrument_const_sptr minstrument = m_maskWS->getInstrument();

  for (size_t i = 0; i < componentnames.size(); i++) {
    g_log.debug() << "Component name = " << componentnames[i] << std::endl;

    // a) get component
    Geometry::IComponent_const_sptr component =
        minstrument->getComponentByName(componentnames[i]);
    if (component)
      g_log.debug() << "Component ID = " << component->getComponentID()
                    << std::endl;
    else {
      // A non-exiting component.  Ignore
      g_log.warning() << "Component " << componentnames[i] << " does not exist!"
                      << std::endl;
      continue;
    }

    // b) component -> component assembly --> children (more than detectors)
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);
    std::vector<Geometry::IComponent_const_sptr> children;
    asmb->getChildren(children, true);

    g_log.debug() << "Number of Children = " << children.size() << std::endl;

    int32_t numdets = 0;
    int32_t id_min = 1000000;
    int32_t id_max = 0;

    for (size_t ic = 0; ic < children.size(); ic++) {
      // c) convert component to detector
      Geometry::IComponent_const_sptr child = children[ic];
      Geometry::IDetector_const_sptr det =
          boost::dynamic_pointer_cast<const Geometry::IDetector>(child);

      if (det) {
        int32_t detid = det->getID();
        detectors.push_back(detid);
        numdets++;
        if (detid < id_min)
          id_min = detid;
        if (detid > id_max)
          id_max = detid;
      }
    }

    g_log.debug() << "Number of Detectors in Children = " << numdets
                  << "  Range = " << id_min << ", " << id_max << std::endl;
  } // for component

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert bank to detectors
 */
void LoadMask::bankToDetectors(std::vector<std::string> singlebanks,
                               std::vector<int32_t> &detectors,
                               std::vector<int32_t> &detectorpairslow,
                               std::vector<int32_t> &detectorpairsup) {
  std::stringstream infoss;
  infoss << "Bank IDs to be converted to detectors: " << endl;
  for (size_t i = 0; i < singlebanks.size(); i++) {
    infoss << "Bank: " << singlebanks[i] << std::endl;
  }
  g_log.debug(infoss.str());

  Geometry::Instrument_const_sptr minstrument = m_maskWS->getInstrument();

  for (size_t ib = 0; ib < singlebanks.size(); ib++) {
    std::vector<Geometry::IDetector_const_sptr> idetectors;

    minstrument->getDetectorsInBank(idetectors, singlebanks[ib]);
    g_log.debug() << "Bank: " << singlebanks[ib] << " has " << idetectors.size()
                  << " detectors" << std::endl;

    // a) get information
    size_t numdets = idetectors.size();
    detid_t detid_first = idetectors[0]->getID();
    detid_t detid_last = idetectors[idetectors.size() - 1]->getID();

    // b) set detectors
    if (detid_first + int32_t(numdets) == detid_last + 1 && false) {
      // TODO This save-time method is not used at this stage
      g_log.information() << "Using Range of Detectors" << std::endl;

      detectorpairslow.push_back(detid_first);
      detectorpairsup.push_back(detid_last);

    } else {
      g_log.debug() << "Apply 1 by 1  "
                    << "DetID: " << detid_first << ", " << detid_last
                    << std::endl;

      for (size_t i = 0; i < idetectors.size(); i++) {
        Geometry::IDetector_const_sptr det = idetectors[i];
        int32_t detid = det->getID();
        detectors.push_back(detid);
      }

    } // if-else
  }   // ENDFOR

  return;
}

//----------------------------------------------------------------------------------------------
/** Set the mask on the spectrum IDs
 */
void LoadMask::processMaskOnWorkspaceIndex(bool mask,
                                           std::vector<int32_t> pairslow,
                                           std::vector<int32_t> pairsup) {
  // 1. Check
  if (pairslow.size() == 0)
    return;
  if (pairslow.size() != pairsup.size()) {
    g_log.error() << "Input spectrum IDs are not paired.  Size(low) = "
                  << pairslow.size() << ", Size(up) = " << pairsup.size()
                  << std::endl;
    throw std::invalid_argument("Input spectrum IDs are not paired. ");
  }

  // 2. Get Map
  const spec2index_map s2imap = m_maskWS->getSpectrumToWorkspaceIndexMap();
  spec2index_map::const_iterator s2iter;

  // 3. Set mask
  for (size_t i = 0; i < pairslow.size(); i++) {
    // TODO Make this function work!
    g_log.debug() << "Mask Spectrum " << pairslow[i] << "  To " << pairsup[i]
                  << std::endl;

    for (int32_t specid = pairslow[i]; specid <= pairsup[i]; specid++) {
      s2iter = s2imap.find(specid);
      if (s2iter == s2imap.end()) {
        // spectrum not found.  bad brach
        g_log.error()
            << "Spectrum " << specid
            << " does not have an entry in GroupWorkspace's spec2index map"
            << std::endl;
        throw std::runtime_error("Logic error");
      } else {
        size_t wsindex = s2iter->second;
        if (wsindex >= m_maskWS->getNumberHistograms()) {
          // workspace index is out of range.  bad branch
          g_log.error() << "Group workspace's spec2index map is set wrong: "
                        << " Found workspace index = " << wsindex
                        << " for spectrum ID " << specid
                        << " with workspace size = "
                        << m_maskWS->getNumberHistograms() << std::endl;
        } else {
          // Finally set the group workspace.  only good branch
          if (mask)
            m_maskWS->dataY(wsindex)[0] = 1.0;
          else
            m_maskWS->dataY(wsindex)[0] = 0.0;
        } // IF-ELSE: ws index out of range
      }   // IF-ELSE: spectrum ID has an entry
    }     // FOR EACH SpecID
  }       // FOR EACH Pair

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert spectrum to detectors
 */
void LoadMask::detectorToDetectors(std::vector<int32_t> singles,
                                   std::vector<int32_t> pairslow,
                                   std::vector<int32_t> pairsup,
                                   std::vector<int32_t> &detectors,
                                   std::vector<int32_t> &detectorpairslow,
                                   std::vector<int32_t> &detectorpairsup) {
  UNUSED_ARG(detectorpairslow)
  UNUSED_ARG(detectorpairsup)

  /*
  for (size_t i = 0; i < singles.size(); i ++){
    g_log.information() << "Detector " << singles[i] << std::endl;
  }
  for (size_t i = 0; i < pairslow.size(); i ++){
    g_log.information() << "Detector " << pairslow[i] << "  To " << pairsup[i]
  << std::endl;
  }
  */
  for (size_t i = 0; i < singles.size(); i++) {
    detectors.push_back(singles[i]);
  }
  for (size_t i = 0; i < pairslow.size(); i++) {
    for (int32_t j = 0; j < pairsup[i] - pairslow[i] + 1; j++) {
      int32_t detid = pairslow[i] + j;
      detectors.push_back(detid);
    }
    /*
    detectorpairslow.push_back(pairslow[i]);
    detectorpairsup.push_back(pairsup[i]);
    */
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Initalize Poco XML Parser
 */
void LoadMask::initializeXMLParser(const std::string &filename) {
  // const std::string instName
  std::cout << "Load File " << filename << std::endl;
  const std::string xmlText = Kernel::Strings::loadFile(filename);
  std::cout << "Successfully Load XML File " << std::endl;

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  try {
    m_pDoc = pParser.parseString(xmlText);
  } catch (Poco::Exception &exc) {
    throw Kernel::Exception::FileError(
        exc.displayText() + ". Unable to parse File:", filename);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", filename);
  }
  // Get pointer to root element
  m_pRootElem = m_pDoc->documentElement();
  if (!m_pRootElem->hasChildNodes()) {
    g_log.error("XML file: " + filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError(
        "No root element in XML instrument file", filename);
  }
}

//----------------------------------------------------------------------------------------------
/** Parse XML file
 */
void LoadMask::parseXML() {
  // 0. Check
  if (!m_pDoc)
    throw std::runtime_error("Call LoadMask::initialize() before parseXML.");

  // 1. Parse and create a structure
  Poco::AutoPtr<NodeList> pNL_type = m_pRootElem->getElementsByTagName("type");
  g_log.information() << "Node Size = " << pNL_type->length() << std::endl;

  Poco::XML::NodeIterator it(m_pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  bool tomask = true;
  bool ingroup = false;
  while (pNode) {
    const Poco::XML::XMLString value = pNode->innerText();

    if (pNode->nodeName().compare("group") == 0) {
      // Node "group"
      ingroup = true;
      tomask = true;
      /*
      // get type
      Poco::AutoPtr<Poco::XML::NamedNodeMap> att = pNode->attributes();
      Poco::XML::Node* cNode = att->item(0);
      if (cNode->getNodeValue().compare("mask") == 0 ||
      cNode->getNodeValue().compare("notuse") == 0){
        tomask = true;
      } else if (cNode->getNodeValue().compare("unmask") == 0 ||
      cNode->getNodeValue().compare("use") == 0){
        tomask = false;
      } else {
        g_log.error() << "Type (" << cNode->localName() << ") = " <<
      cNode->getNodeValue() << " is not supported!" << std::endl;
      }
      g_log.information() << "Node Group:  child Node Name = " <<
      cNode->localName() << ": " << cNode->getNodeValue()
              << "(always)"<< std::endl;
      */

    } else if (pNode->nodeName().compare("component") == 0) {
      // Node "component"
      if (ingroup) {
        this->parseComponent(value, tomask);
      } else {
        g_log.error() << "XML File heirachial (component) error!" << std::endl;
      }
      // g_log.information() << "Component: " << value << std::endl;

    } else if (pNode->nodeName().compare("ids") == 0) {
      // Node "ids"
      if (ingroup) {
        this->parseSpectrumIDs(value, tomask);
      } else {
        g_log.error() << "XML File (ids) heirachial error!"
                      << "  Inner Text = " << pNode->innerText() << std::endl;
      }
      // g_log.information() << "detids: " << value << std::endl;

    } else if (pNode->nodeName().compare("detids") == 0) {
      // Node "detids"
      if (ingroup) {
        this->parseDetectorIDs(value, tomask);
      } else {
        g_log.error() << "XML File (detids) heirachial error!" << std::endl;
      }

    } else if (pNode->nodeName().compare("detector-masking") == 0) {
      // Node "detector-masking".  Check default value
      m_defaultToUse = true;
      /*
      Poco::AutoPtr<Poco::XML::NamedNodeMap> att = pNode->attributes();
      if (att->length() > 0){
        Poco::XML::Node* cNode = att->item(0);
        m_defaultToUse = true;
        if (cNode->localName().compare("default") == 0){
          if (cNode->getNodeValue().compare("use") == 0){
            m_defaultToUse = true;
          } else {
            m_defaultToUse = false;
          }
      } // if - att-length
      */
    } // END-IF-ELSE: pNode->nodeName()

    pNode = it.nextNode();
  } // ENDWHILE

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse bank IDs (string name)
 * Sample:  bank2
 * @param valuetext:  must be bank name
 * @param tomask: if true, mask, if not unmask
 */
void LoadMask::parseComponent(std::string valuetext, bool tomask) {

  // 1. Parse bank out
  /*
  std::vector<std::string> values;
  this->splitString(valuetext, values, "bank");
  if (values.size() <= 1){
    g_log.error() << "Bank information format error!" << std::endl;
    return;
  }
  */

  if (tomask) {
    mask_bankid_single.push_back(valuetext);
  } else {
    unmask_bankid_single.push_back(valuetext);
  }

  /*
  for (size_t i = 0; i < singles.size(); i ++){
    g_log.information() << "Bank " << singles[i] << std::endl;
  }
  for (size_t i = 0; i < pairs.size()/2; i ++){
    g_log.information() << "Bank " << pairs[2*i] << "  To " << pairs[2*i+1] <<
  std::endl;
  }
  */

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse input string for spectrum ID
 */
void LoadMask::parseSpectrumIDs(std::string inputstr, bool tomask) {

  // 1. Parse range out
  std::vector<int32_t> singles;
  std::vector<int32_t> pairs;
  this->parseRangeText(inputstr, singles, pairs);

  // 2. Set to data storage
  if (tomask) {
    for (size_t i = 0; i < singles.size(); i++) {
      mask_specid_single.push_back(singles[i]);
    }
    for (size_t i = 0; i < pairs.size() / 2; i++) {
      mask_specid_pair_low.push_back(pairs[2 * i]);
      mask_specid_pair_up.push_back(pairs[2 * i + 1]);
    }
  } else {
    for (size_t i = 0; i < singles.size(); i++) {
      unmask_specid_single.push_back(singles[i]);
    }
    for (size_t i = 0; i < pairs.size() / 2; i++) {
      unmask_specid_pair_low.push_back(pairs[2 * i]);
      unmask_specid_pair_up.push_back(pairs[2 * i + 1]);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse input string for spectrum ID
 */
void LoadMask::parseDetectorIDs(std::string inputstr, bool tomask) {
  // g_log.information() << "Detector IDs: " << inputstr << std::endl;

  // 1. Parse range out
  std::vector<int32_t> singles;
  std::vector<int32_t> pairs;
  this->parseRangeText(inputstr, singles, pairs);

  // 2. Set to data storage
  if (tomask) {
    for (size_t i = 0; i < singles.size(); i++) {
      mask_detid_single.push_back(singles[i]);
    }
    for (size_t i = 0; i < pairs.size() / 2; i++) {
      mask_detid_pair_low.push_back(pairs[2 * i]);
      mask_detid_pair_up.push_back(pairs[2 * i + 1]);
    }
  } else {
    for (size_t i = 0; i < singles.size(); i++) {
      unmask_detid_single.push_back(singles[i]);
    }
    for (size_t i = 0; i < pairs.size() / 2; i++) {
      unmask_detid_pair_low.push_back(pairs[2 * i]);
      unmask_detid_pair_up.push_back(pairs[2 * i + 1]);
    }
  }

  return;
}

/*
 * Parse index range text to singles and pairs
 * Example: 3,4,9-10,33
 */
void LoadMask::parseRangeText(std::string inputstr,
                              std::vector<int32_t> &singles,
                              std::vector<int32_t> &pairs) {
  // 1. Split ','
  std::vector<std::string> rawstrings;
  this->splitString(inputstr, rawstrings, ",");

  // 2. Filter
  std::vector<std::string> strsingles;
  std::vector<std::string> strpairs;
  for (size_t i = 0; i < rawstrings.size(); i++) {
    // a) Find '-':
    bool containto = false;
    const char *tempchs = rawstrings[i].c_str();
    for (size_t j = 0; j < rawstrings[i].size(); j++)
      if (tempchs[j] == '-') {
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
  for (size_t i = 0; i < strsingles.size(); i++) {
    int32_t itemp = atoi(strsingles[i].c_str());
    singles.push_back(itemp);
  }

  // 4. Treat pairs
  for (size_t i = 0; i < strpairs.size(); i++) {
    // a) split and check
    std::vector<std::string> ptemp;
    this->splitString(strpairs[i], ptemp, "-");
    if (ptemp.size() != 2) {
      g_log.error() << "Range string " << strpairs[i] << " has a wrong format!"
                    << std::endl;
      throw std::invalid_argument("Wrong format");
    }

    // b) parse
    int32_t intstart = atoi(ptemp[0].c_str());
    int32_t intend = atoi(ptemp[1].c_str());
    if (intstart >= intend) {
      g_log.error() << "Range string " << strpairs[i] << " has a reversed order"
                    << std::endl;
      throw std::invalid_argument("Wrong format");
    }
    pairs.push_back(intstart);
    pairs.push_back(intend);
  }

  return;
}

void LoadMask::splitString(std::string inputstr,
                           std::vector<std::string> &strings, std::string sep) {

  // std::vector<std::string> SplitVec;
  boost::split(strings, inputstr, boost::is_any_of(sep),
               boost::token_compress_on);

  // g_log.information() << "Inside... split size = " << strings.size() <<
  // std::endl;

  return;
}

/*
 * Load and parse an ISIS masking file
 */
void LoadMask::loadISISMaskFile(std::string isisfilename) {

  std::ifstream ifs;
  ifs.open(isisfilename.c_str(), std::ios::in);
  if (!ifs.is_open()) {
    g_log.error() << "Cannot open ISIS mask file " << isisfilename << endl;
    throw std::invalid_argument("Cannot open ISIS mask file");
  }

  std::string isisline;
  while (getline(ifs, isisline)) {
    boost::trim(isisline);

    // a. skip empty line
    if (isisline.size() == 0)
      continue;

    // b. skip comment line
    if (isisline.c_str()[0] < '0' || isisline.c_str()[0] > '9')
      continue;

    // c. parse
    g_log.debug() << "Input: " << isisline << std::endl;
    parseISISStringToVector(isisline, mask_specid_pair_low,
                            mask_specid_pair_up);
  }

  for (size_t i = 0; i < mask_specid_pair_low.size(); i++) {
    g_log.debug() << i << ": " << mask_specid_pair_low[i] << ", "
                  << mask_specid_pair_up[i] << std::endl;
  }

  ifs.close();

  return;
}

/*
 * Parse a line in an ISIS mask file string to vector
 * Combination of 5 types of format for unit
 * (1) a (2) a-b (3) a - b (4) a- b (5) a- b
 * separated by space(s)
 */
void LoadMask::parseISISStringToVector(string ins,
                                       std::vector<int> &rangestartvec,
                                       std::vector<int> &rangeendvec) {
  // 1. Split by space
  std::vector<string> splitstrings;
  boost::split(splitstrings, ins, boost::is_any_of(" "),
               boost::token_compress_on);

  // 2. Replace a-b to a - b, remove a-b and insert a, -, b
  bool tocontinue = true;
  size_t index = 0;
  while (tocontinue) {
    // a) Determine end of loop.  Note that loop size changes
    if (index == splitstrings.size() - 1) {
      tocontinue = false;
    }

    // b) Need to split?
    vector<string> temps;
    boost::split(temps, splitstrings[index], boost::is_any_of("-"),
                 boost::token_compress_on);
    if (splitstrings[index].compare("-") == 0 || temps.size() == 1) {
      // Nothing to split
      index++;
    } else if (temps.size() == 2) {
      // Has a '-' inside.  Delete and Replace
      temps.insert(temps.begin() + 1, "-");
      splitstrings.erase(splitstrings.begin() + index);
      for (size_t ic = 0; ic < 3; ic++) {
        if (temps[ic].size() > 0) {
          splitstrings.insert(splitstrings.begin() + index, temps[ic]);
          index++;
        }
      }
    } else {
      // Exception
      g_log.error() << "String " << splitstrings[index]
                    << " has a wrong format.  Too many '-'" << std::endl;
      throw std::invalid_argument("Invalid string in input");
    }

    if (index >= splitstrings.size())
      tocontinue = false;

  } // END WHILE

  // 3. Put to output integer vector
  tocontinue = true;
  index = 0;
  while (tocontinue) {
    // i)   push to the starting vector
    rangestartvec.push_back(atoi(splitstrings[index].c_str()));

    // ii)  push the ending vector
    if (index == splitstrings.size() - 1 ||
        splitstrings[index + 1].compare("-") != 0) {
      // the next one is not '-'
      rangeendvec.push_back(atoi(splitstrings[index].c_str()));
      index++;
    } else {
      // the next one is '-', thus read '-', next
      rangeendvec.push_back(atoi(splitstrings[index + 2].c_str()));
      index += 3;
    }

    if (index >= splitstrings.size())
      tocontinue = false;
  } // END-WHILE

  splitstrings.clear();

  return;
}

//----------------------------------------------------------------------------------------------
/** Initialize the Mask Workspace with instrument
 */
void LoadMask::intializeMaskWorkspace() {
  const bool ignoreDirs(true);
  const std::string idfPath =
    API::FileFinder::Instance().getFullPath(m_instrumentPropValue, ignoreDirs);

  MatrixWorkspace_sptr tempWs(new DataObjects::Workspace2D());

  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", tempWs);

  if (idfPath.empty())
    loadInst->setPropertyValue("InstrumentName", m_instrumentPropValue);
  else
    loadInst->setPropertyValue("Filename", m_instrumentPropValue);

  loadInst->setProperty("RewriteSpectraMap", false);
  loadInst->executeAsChildAlg();

  if (!loadInst->isExecuted()) {
    g_log.error() << "Unable to load Instrument " << m_instrumentPropValue
                  << std::endl;
    throw std::invalid_argument(
        "Incorrect instrument name or invalid IDF given.");
  }

  m_maskWS = DataObjects::MaskWorkspace_sptr(
      new DataObjects::MaskWorkspace(tempWs->getInstrument()));
  m_maskWS->setTitle("Mask");
}

} // namespace Mantid
} // namespace DataHandling
