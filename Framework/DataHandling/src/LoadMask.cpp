// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMask.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"

#include "MantidKernel/Strings.h"

#include <fstream>
#include <map>
#include <sstream>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>

#include <boost/algorithm/string.hpp>

using Poco::XML::DOMParser;
using Poco::XML::Node;
using Poco::XML::NodeFilter;
using Poco::XML::NodeIterator;
using Poco::XML::NodeList;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace std;

namespace {
// service routines
//-------------------------------------------------------------------------------------------
/** Convert ranged vectors to single-valued vector
 * @param singles -- input vector of single numbers to copy to result without
 *                   changes.
 * @param ranges  -- input vector of data ranges -- pairs of min-max values,
 *                   expanded to result as numbers from min to max
 *                   inclusively, with step 1
 * @param tot_singles -- on input contains range of single values already
 *                   copied into the result by previous call to the routine,
 *                   on output, all singles and expanded pairs
 *                   from the input are added to it.
 */
template <typename T>
void convertToVector(const std::vector<T> &singles, const std::vector<T> &ranges, std::vector<T> &tot_singles) {

  // find the size of the final vector of masked values
  size_t n_total(singles.size() + tot_singles.size());
  for (size_t i = 0; i < ranges.size(); i += 2) {
    n_total += ranges[i + 1] - ranges[i] + 1;
  }
  // reserve space for all masked spectra
  // for efficient memory operations
  tot_singles.reserve(n_total);
  // add singles to the existing singles
  tot_singles.insert(tot_singles.end(), singles.begin(), singles.end());
  // expand pairs
  for (size_t i = 0; i < ranges.size(); i += 2) {
    for (T obj_id = ranges[i]; obj_id < ranges[i + 1] + 1; ++obj_id) {
      tot_singles.emplace_back(obj_id);
    }
  }
}

/** Parse index range text to singles and pairs
 * Example: 3,4,9-10,33
 *
 * @param inputstr -- input string to process in the format as above
 * @param singles -- vector of objects, defined as singles
 * @param pairs   -- vector of objects, defined as pairs, in the form min,max
 *                   value
 */
template <typename T> void parseRangeText(const std::string &inputstr, std::vector<T> &singles, std::vector<T> &pairs) {
  // 1. Split ','
  std::vector<std::string> rawstrings;
  boost::split(rawstrings, inputstr, boost::is_any_of(","), boost::token_compress_on);

  for (auto &rawstring : rawstrings) {
    // a) Find '-':
    boost::trim(rawstring);
    bool containDash(true);
    if (rawstring.find_first_of('-') == std::string::npos) {
      containDash = false;
    }

    // Process appropriately
    if (containDash) { // 4. Treat pairs
      std::vector<std::string> ptemp;
      boost::split(ptemp, rawstring, boost::is_any_of("-"), boost::token_compress_on);
      if (ptemp.size() != 2) {
        std::string error = "Range string " + rawstring + " has a wrong format!";
        throw std::invalid_argument(error);
      }
      // b) parse
      auto intstart = boost::lexical_cast<T>(ptemp[0]);
      auto intend = boost::lexical_cast<T>(ptemp[1]);
      if (intstart >= intend) {
        std::string error = "Range string " + rawstring + " has wrong order of detectors ID!";
        throw std::invalid_argument(error);
      }
      pairs.emplace_back(intstart);
      pairs.emplace_back(intend);

    } else { // 3. Treat singles
      auto itemp = boost::lexical_cast<T>(rawstring);
      singles.emplace_back(itemp);
    }
  } // ENDFOR i
}

/*
 * Parse a line in an ISIS mask file string to vector
 * Combination of 5 types of format for unit
 * (1) a (2) a-b (3) a - b (4) a- b (5) a -b
 * separated by space(s)
 * @param  ins    -- input string in ISIS ASCII format
 * @return ranges -- vector of a,b pairs converted from input
 */
void parseISISStringToVector(const std::string &ins, std::vector<Mantid::specnum_t> &ranges) {
  // 1. Split by space
  std::vector<string> splitstrings;
  boost::split(splitstrings, ins, boost::is_any_of(" "), boost::token_compress_on);

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
    boost::split(temps, splitstrings[index], boost::is_any_of("-"), boost::token_compress_on);
    if (splitstrings[index] == "-" || temps.size() == 1) {
      // Nothing to split
      index++;
    } else if (temps.size() == 2) {
      // Has a '-' inside.  Delete and Replace
      temps.insert(temps.begin() + 1, "-");
      splitstrings.erase(splitstrings.begin() + index);
      for (size_t ic = 0; ic < 3; ic++) {
        if (!temps[ic].empty()) {
          splitstrings.insert(splitstrings.begin() + index, temps[ic]);
          index++;
        }
      }
    } else {
      // Exception
      std::string err = "String " + splitstrings[index] + " has too many '-'";
      throw std::invalid_argument(err);
    }

    if (index >= splitstrings.size())
      tocontinue = false;

  } // END WHILE

  // 3. Put to output integer vector
  tocontinue = true;
  index = 0;
  while (tocontinue) {
    // i)   push to the starting vector
    ranges.emplace_back(boost::lexical_cast<Mantid::specnum_t>(splitstrings[index]));

    // ii)  push the ending vector
    if (index == splitstrings.size() - 1 || splitstrings[index + 1] != "-") {
      // the next one is not '-'
      ranges.emplace_back(boost::lexical_cast<Mantid::specnum_t>(splitstrings[index]));
      index++;
    } else {
      // the next one is '-', thus read '-', next
      ranges.emplace_back(boost::lexical_cast<Mantid::specnum_t>(splitstrings[index + 2]));
      index += 3;
    }

    if (index >= splitstrings.size())
      tocontinue = false;
  } // END-WHILE
}
/*
* Load and parse an ISIS masking file
@param isisfilename :: the string containing full path to an ISIS mask file
@param SpectraMasks :: output list of the spectra numbers to mask.
*/
void loadISISMaskFile(const std::string &isisfilename, std::vector<Mantid::specnum_t> &spectraMasks) {

  std::vector<Mantid::specnum_t> ranges;

  std::ifstream ifs;
  ifs.open(isisfilename, std::ios::in);
  if (!ifs.is_open()) {
    throw std::invalid_argument("Cannot open ISIS mask file" + isisfilename);
  }

  std::string isisline;
  while (getline(ifs, isisline)) {
    boost::trim(isisline);

    // a. skip empty line
    if (isisline.empty())
      continue;

    // b. skip comment line
    if (isisline.c_str()[0] < '0' || isisline.c_str()[0] > '9')
      continue;

    // c. parse
    parseISISStringToVector(isisline, ranges);
  }

  // dummy helper vector as ISIS mask is always processed as pairs.
  std::vector<Mantid::specnum_t> dummy;
  convertToVector(dummy, ranges, spectraMasks);
}

} // namespace

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(LoadMask)

/// Initialise the properties
void LoadMask::init() {

  // 1. Declare property
  declareProperty("Instrument", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the instrument to apply the mask.");

  const std::vector<std::string> maskExts{".xml", ".msk"};
  declareProperty(std::make_unique<FileProperty>("InputFile", "", FileProperty::Load, maskExts),
                  "Masking file for masking. Supported file format is XML and "
                  "ISIS ASCII. ");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("RefWorkspace", "", Direction::Input,
                                                                            PropertyMode::Optional),
                  "The name of the workspace wich defines instrument and spectra, "
                  "used as the source of the spectra-detector map for the mask to load. "
                  "The instrument, attached to this workspace has to be the same as the "
                  "one specified by 'Instrument' property");

  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>("OutputWorkspace", "Masking", Direction::Output),
      "Output Masking Workspace");
}

//----------------------------------------------------------------------------------------------
/** Main execution body of this algorithm
 */
void LoadMask::exec() {
  reset();

  // 1. Load Instrument and create output Mask workspace
  const std::string instrumentname = getProperty("Instrument");
  m_sourceMapWS = getProperty("RefWorkspace");

  m_instrumentPropValue = instrumentname;
  setProperty("Instrument", instrumentname);

  this->intializeMaskWorkspace();

  if (m_sourceMapWS) { // check if the instruments are compatible
    auto t_inst_name = m_maskWS->getInstrument()->getName();
    auto r_inst_name = m_sourceMapWS->getInstrument()->getName();
    if (t_inst_name != r_inst_name) {
      throw std::invalid_argument("If reference workspace is provided, it has "
                                  "to have instrument with the same name as "
                                  "specified by 'Instrument' property");
    }
  }

  setProperty("OutputWorkspace", m_maskWS);

  m_defaultToUse = true;

  // 2. Parse Mask File
  std::string filename = getProperty("InputFile");

  if (filename.ends_with("l") || filename.ends_with("L")) {
    // 2.1 XML File
    this->initializeXMLParser(filename);
    this->parseXML();
  } else if (filename.ends_with("k") || filename.ends_with("K")) {
    // 2.2 ISIS Masking file
    loadISISMaskFile(filename, m_maskSpecID);
    m_defaultToUse = true;
  } else {
    g_log.error() << "File " << filename << " is not in supported format. \n";
    return;
  }
  // 3. Translate and set geometry
  g_log.information() << "To Mask: \n";

  this->componentToDetectors(m_maskCompIdSingle, m_maskDetID);

  // unmasking is not implemented
  // g_log.information() << "To UnMask: \n";

  // As m_uMaskCompIdSingle is empty, this never works
  this->bankToDetectors(m_uMaskCompIdSingle, m_unMaskDetID);

  // convert spectra ID to corresponded det-id-s
  this->processMaskOnWorkspaceIndex(true, m_maskSpecID, m_maskDetID);

  // 4. Apply
  this->initDetectors();
  const detid2index_map indexmap = m_maskWS->getDetectorIDToWorkspaceIndexMap(true);

  this->processMaskOnDetectors(indexmap, true, m_maskDetID);
  // TODO: Not implemented, but should work as soon as m_unMask contains
  // something
  this->processMaskOnDetectors(indexmap, false, m_unMaskDetID);
}

void LoadMask::initDetectors() {

  if (!m_defaultToUse) { // Default is to use all detectors
    size_t numHist = m_maskWS->getNumberHistograms();
    for (size_t wkspIndex = 0; wkspIndex < numHist; wkspIndex++) {
      m_maskWS->setMaskedIndex(wkspIndex);
    }
  }
}

//----------------------------------------------------------------------------------------------
/**  Mask detectors or Unmask detectors
 *   @param indexmap: spectraId to spectraNum map used
 *                   in masking
 *   @param tomask:  true to mask, false to unmask
 *   @param singledetids: list of individual det ids to mask
 */
void LoadMask::processMaskOnDetectors(const detid2index_map &indexmap, bool tomask,
                                      const std::vector<detid_t> &singledetids) {
  // 1. Get index map
  // 2. Mask
  g_log.debug() << "Mask = " << tomask << "  Final Single IDs Size = " << singledetids.size() << '\n';

  for (auto detid : singledetids) {
    detid2index_map::const_iterator it;
    it = indexmap.find(detid);
    if (it != indexmap.end()) {
      size_t index = it->second;
      m_maskWS->mutableY(index)[0] = (tomask) ? 1 : 0;
    } else {
      g_log.warning() << "Pixel w/ ID = " << detid << " Cannot Be Located\n";
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Extract a component's detectors and return it within detectors array
 *  It is a generalized version of bankToDetectors()
 *
 * @param componentnames -- vector of component names to process
 * @param detectors      -- vector of detector ids, which belongs to components
 *provided as input.
 */
void LoadMask::componentToDetectors(const std::vector<std::string> &componentnames, std::vector<detid_t> &detectors) {
  Geometry::Instrument_const_sptr minstrument = m_maskWS->getInstrument();

  for (auto &componentname : componentnames) {
    g_log.debug() << "Component name = " << componentname << '\n';

    // a) get component
    Geometry::IComponent_const_sptr component = minstrument->getComponentByName(componentname);
    if (component)
      g_log.debug() << "Component ID = " << component->getComponentID() << '\n';
    else {
      // A non-exiting component.  Ignore
      g_log.warning() << "Component " << componentname << " does not exist!\n";
      continue;
    }

    // b) component -> component assembly --> children (more than detectors)
    std::shared_ptr<const Geometry::ICompAssembly> asmb =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);
    std::vector<Geometry::IComponent_const_sptr> children;
    asmb->getChildren(children, true);

    g_log.debug() << "Number of Children = " << children.size() << '\n';

    size_t numdets(0);
    detid_t id_min(std::numeric_limits<Mantid::detid_t>::max());
    detid_t id_max(0);

    for (const auto &child : children) {
      // c) convert component to detector
      Geometry::IDetector_const_sptr det = std::dynamic_pointer_cast<const Geometry::IDetector>(child);

      if (det) {
        detid_t detid = det->getID();
        detectors.emplace_back(detid);
        numdets++;
        if (detid < id_min)
          id_min = detid;
        if (detid > id_max)
          id_max = detid;
      }
    }

    g_log.debug() << "Number of Detectors in Children = " << numdets << "  Range = " << id_min << ", " << id_max
                  << '\n';
  } // for component
}

//----------------------------------------------------------------------------------------------
/** Convert bank to detectors
 * This routine has never been used. Dead code.
 * @param   singlebanks -- vector of string containing bank names
 * @param  detectors   -- vector of detector-id-s belonging to these banks
 */
void LoadMask::bankToDetectors(const std::vector<std::string> &singlebanks, std::vector<detid_t> &detectors) {
  std::stringstream infoss;
  infoss << "Bank IDs to be converted to detectors: \n";
  for (auto &singlebank : singlebanks) {
    infoss << "Bank: " << singlebank << '\n';
  }
  g_log.debug(infoss.str());

  Geometry::Instrument_const_sptr minstrument = m_maskWS->getInstrument();

  for (auto &singlebank : singlebanks) {
    std::vector<Geometry::IDetector_const_sptr> idetectors;

    minstrument->getDetectorsInBank(idetectors, singlebank);
    g_log.debug() << "Bank: " << singlebank << " has " << idetectors.size() << " detectors\n";

    // a) get information
    size_t numdets = idetectors.size();
    detid_t detid_first = idetectors.front()->getID();
    detid_t detid_last = idetectors.back()->getID();

    // b) set detectors

    for (const auto &det : idetectors) {
      detid_t detid = det->getID();
      detectors.emplace_back(detid);
    }
    g_log.debug() << "Number of Detectors in Bank  " << singlebank << "  is: " << numdets
                  << "\nRange From: " << detid_first << " To: " << detid_last << '\n';

  } // ENDFOR
}

//----------------------------------------------------------------------------------------------
/** Set the mask on the spectrum numbers or convert them to detector-s id if
 *  sample workspace is provided
 *@param  mask           -- to mask or unmask appropriate spectra
 *@param maskedSpecID    -- vector of the spectra numbers to process
 *@param singleDetIds    -- vector of det-id-s to extend if workspace-
 *                          source of spectra-detector map is provided
 */
void LoadMask::processMaskOnWorkspaceIndex(bool mask, std::vector<int32_t> &maskedSpecID,
                                           std::vector<int32_t> &singleDetIds) {
  // 1. Check
  if (maskedSpecID.empty())
    return;

  if (m_sourceMapWS) {
    // convert spectra masks into det-id mask using source workspace
    convertSpMasksToDetIDs(*m_sourceMapWS, maskedSpecID, singleDetIds);
    maskedSpecID.clear(); // spectra ID not needed any more as all converted to det-ids
    return;
  }
  // 2. Get Map
  const spec2index_map s2imap = m_maskWS->getSpectrumToWorkspaceIndexMap();

  spec2index_map::const_iterator s2iter;

  // 3. Set mask
  auto spec0 = maskedSpecID[0];
  auto prev_masks = spec0;
  for (int spec2mask : maskedSpecID) {

    s2iter = s2imap.find(spec2mask);
    if (s2iter == s2imap.end()) {
      // spectrum not found.  bad branch
      g_log.error() << "Spectrum " << spec2mask << " does not have an entry in GroupWorkspace's spec2index map\n";
      throw std::runtime_error("Logic error");
    } else {
      size_t wsindex = s2iter->second;
      if (wsindex >= m_maskWS->getNumberHistograms()) {
        // workspace index is out of range.  bad branch
        g_log.error() << "Group workspace's spec2index map is set wrong: "
                      << " Found workspace index = " << wsindex << " for spectrum No " << spec2mask
                      << " with workspace size = " << m_maskWS->getNumberHistograms() << '\n';
      } else {
        // Finally set the masking;
        m_maskWS->mutableY(wsindex)[0] = (mask) ? 1.0 : 0.0;
      } // IF-ELSE: ws index out of range
    } // IF-ELSE: spectrum No has an entry

    if (spec2mask > prev_masks + 1) {
      g_log.debug() << "Masked Spectrum " << spec0 << "  To " << prev_masks << '\n';
      spec0 = spec2mask;
    }
  } // FOR EACH SpecNo
}

//----------------------------------------------------------------------------------------------
/** Initalize Poco XML Parser
 * @param filename  -- name of the xml file to process.
 */
void LoadMask::initializeXMLParser(const std::string &filename) {
  // const std::string instName
  g_log.information() << "Load File " << filename << '\n';
  const std::string xmlText = Kernel::Strings::loadFile(filename);
  g_log.information("Successfully Load XML File");

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  try {
    m_pDoc = pParser.parseString(xmlText);
  } catch (Poco::Exception &exc) {
    throw Kernel::Exception::FileError(exc.displayText() + ". Unable to parse File:", filename);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", filename);
  }
  // Get pointer to root element
  m_pRootElem = m_pDoc->documentElement();
  if (!m_pRootElem->hasChildNodes()) {
    g_log.error("XML file: " + filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument file", filename);
  }
}

//----------------------------------------------------------------------------------------------
/** Parse XML file and define the following internal variables:
    std::vector<detid_t> m_maskDetID;
    //spectrum id-s to unmask
    std::vector<detid_t> m_unMaskDetID;

    spectra mask provided
    std::vector<specnum_t> m_maskSpecID;
    spectra unmask provided NOT IMPLEMENTED
    std::vector<specnum_t> m_unMaskSpecID;

    std::vector<std::string> m_maskCompIDSingle;
    std::vector<std::string> m_uMaskCompIDSingle;
//
Supported xml Node names are:
component:  the name of an instrument component, containing detectors.
ids      : spectra numbers
detids   : detector numbers
Full implementation needs unit tests verifying all these. Only detector id-s are
currently implemented
// There are also no current support for keyword, switching on un-masking
 */
void LoadMask::parseXML() {
  // 0. Check
  if (!m_pDoc)
    throw std::runtime_error("Call LoadMask::initialize() before parseXML.");

  // 1. Parse and create a structure
  Poco::AutoPtr<NodeList> pNL_type = m_pRootElem->getElementsByTagName("type");
  g_log.information() << "Node Size = " << pNL_type->length() << '\n';

  Poco::XML::NodeIterator it(m_pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  std::vector<specnum_t> singleSp, pairSp;
  std::vector<detid_t> maskSingleDet, maskPairDet;

  bool ingroup = false;
  while (pNode) {
    const Poco::XML::XMLString value = pNode->innerText();

    if (pNode->nodeName() == "group") {
      // Node "group"
      ingroup = true;

    } else if (pNode->nodeName() == "component") {
      // Node "component"
      if (ingroup) {
        m_maskCompIdSingle.emplace_back(value);
      } else {
        g_log.error() << "XML File hierarchical (component) error!\n";
      }

    } else if (pNode->nodeName() == "ids") {
      // Node "ids"
      if (ingroup) {
        parseRangeText(value, singleSp, pairSp);
      } else {
        g_log.error() << "XML File (ids) hierarchical error!"
                      << "  Inner Text = " << pNode->innerText() << '\n';
      }

    } else if (pNode->nodeName() == "detids") {
      // Node "detids"
      if (ingroup) {
        parseRangeText(value, maskSingleDet, maskPairDet);
      } else {
        g_log.error() << "XML File (detids) hierarchical error!\n";
      }

    } else if (pNode->nodeName() == "detector-masking") {
      // Node "detector-masking".  Check default value
      m_defaultToUse = true;
    } // END-IF-ELSE: pNode->nodeName()

    pNode = it.nextNode();
  } // ENDWHILE

  convertToVector(singleSp, pairSp, m_maskSpecID);
  convertToVector(maskSingleDet, maskPairDet, m_maskDetID);
  // NOTE: -- TODO: NOT IMPLEMENTD -- if unmasking is implemented, should be
  // enabled
  // convertToVector(umaskSingleDet, umaskPairDet, m_unMaskDetID);
}

/* Convert spectra mask into det-id mask using workspace as source of
 *spectra-detector maps
 *
 * @param sourceWS       -- the workspace containing source spectra-detector map
 *                          to use on masks
 * @param maskedSpecID   -- vector of spectra id to mask
 * @param singleDetIds   -- output vector of detector ids to mask
 */
void LoadMask::convertSpMasksToDetIDs(const API::MatrixWorkspace &sourceWS, const std::vector<int32_t> &maskedSpecID,
                                      std::vector<int32_t> &singleDetIds) {

  spec2index_map s2imap = sourceWS.getSpectrumToWorkspaceIndexMap();
  detid2index_map sourceDetMap = sourceWS.getDetectorIDToWorkspaceIndexMap(false);

  std::multimap<size_t, Mantid::detid_t> spectr2index_map;
  for (auto &it : sourceDetMap) {
    spectr2index_map.insert(std::pair<size_t, Mantid::detid_t>(it.second, it.first));
  }
  for (int i : maskedSpecID) {
    // find spectra number from spectra ID for the source workspace
    const auto itSpec = s2imap.find(i);
    if (itSpec == s2imap.end()) {
      throw std::runtime_error("Can not find spectra with ID: " + boost::lexical_cast<std::string>(i) +
                               " in the workspace" + sourceWS.getName());
    }
    size_t specN = itSpec->second;

    // find detector range related to this spectra id in the source workspace
    const auto source_range = spectr2index_map.equal_range(specN);
    if (source_range.first == spectr2index_map.end()) {
      throw std::runtime_error("Can not find spectra N: " + boost::lexical_cast<std::string>(specN) +
                               " in the workspace" + sourceWS.getName());
    }
    // add detectors to the masked det-id list
    for (auto it = source_range.first; it != source_range.second; ++it) {
      singleDetIds.emplace_back(it->second);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the Mask Workspace with instrument
 */
void LoadMask::intializeMaskWorkspace() {

  if (m_sourceMapWS) {
    m_maskWS = DataObjects::MaskWorkspace_sptr(new DataObjects::MaskWorkspace(m_sourceMapWS->getInstrument()));
  } else {
    MatrixWorkspace_sptr tempWs(new DataObjects::Workspace2D());
    const bool ignoreDirs(true);
    const std::string idfPath = API::FileFinder::Instance().getFullPath(m_instrumentPropValue, ignoreDirs);

    auto loadInst = createChildAlgorithm("LoadInstrument");
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", tempWs);

    if (idfPath.empty())
      loadInst->setPropertyValue("InstrumentName", m_instrumentPropValue);
    else
      loadInst->setPropertyValue("Filename", m_instrumentPropValue);

    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
    loadInst->executeAsChildAlg();

    if (!loadInst->isExecuted()) {
      g_log.error() << "Unable to load Instrument " << m_instrumentPropValue << '\n';
      throw std::invalid_argument("Incorrect instrument name or invalid IDF given.");
    }

    m_maskWS = DataObjects::MaskWorkspace_sptr(new DataObjects::MaskWorkspace(tempWs->getInstrument()));
  }
  m_maskWS->setTitle("Mask");
}

/**Validates if either input workspace or instrument name is defined
@return the inconsistency between Instrument/Workspace properties or empty list
if no errors is found.
*/
std::map<std::string, std::string> LoadMask::validateInputs() {

  std::map<std::string, std::string> result;

  API::MatrixWorkspace_sptr inputWS = getProperty("RefWorkspace");
  std::string InstrName = getProperty("Instrument");
  if (inputWS) {
    boost::trim(InstrName);
    boost::algorithm::to_lower(InstrName);
    size_t len = InstrName.size();
    /// input property contains name of instrument definition file rather than
    /// instrument name itself
    bool IDF_provided{false};
    // Check if the name ends up with .xml which means that idf file name
    // is provided rather then an instrument name.
    if (len > 4) {
      if (InstrName.compare(len - 4, len, ".xml") == 0) {
        IDF_provided = true;
      } else {
        IDF_provided = false;
      }
    } else {
      IDF_provided = false;
    }
    try {
      auto inst = inputWS->getInstrument();
      std::string Name = inst->getName();
      boost::algorithm::to_lower(Name);
      if (Name != InstrName && !IDF_provided) {
        result["RefWorkspace"] = "If both reference workspace and instrument name are defined, "
                                 "workspace has to have the instrument with the same name\n"
                                 "'Instrument' value: " +
                                 InstrName + " Workspace Instrument name: " + Name;
      }
    } catch (Kernel::Exception::NotFoundError &) {
      result["RefWorkspace"] = "If reference workspace is defined, it mast have an instrument";
    }
  }

  return result;
}

void LoadMask::reset() {
  // LoadMask instance may be reused, need to clear buffers.
  m_maskDetID.clear();
  m_unMaskDetID.clear();
  m_maskSpecID.clear();
  m_maskCompIdSingle.clear();
  m_uMaskCompIdSingle.clear();
}

} // namespace Mantid::DataHandling
