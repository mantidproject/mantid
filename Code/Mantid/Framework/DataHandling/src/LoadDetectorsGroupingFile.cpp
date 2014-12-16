#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidAPI/SpectraAxis.h"

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
#include <Poco/String.h>

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadDetectorsGroupingFile)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadDetectorsGroupingFile::LoadDetectorsGroupingFile() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadDetectorsGroupingFile::~LoadDetectorsGroupingFile() {}

void LoadDetectorsGroupingFile::init() {
  /// Initialise the properties

  std::vector<std::string> fileExts(2);
  fileExts[0] = ".xml";
  fileExts[1] = ".map";

  declareProperty(
      new FileProperty("InputFile", "", FileProperty::Load, fileExts),
      "The XML or Map file with full path.");

  declareProperty(new WorkspaceProperty<DataObjects::GroupingWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");
}

/// Run the algorithm
void LoadDetectorsGroupingFile::exec() {
  Poco::Path inputFile(static_cast<std::string>(getProperty("InputFile")));

  std::string ext = Poco::toLower(inputFile.getExtension());

  if (ext == "xml") {
    // Deal with file as xml

    // 1. Parse XML File
    LoadGroupXMLFile loader;
    loader.loadXMLFile(inputFile.toString());

    // Load an instrument, if given
    if (loader.isGivenInstrumentName()) {
      const std::string instrumentName = loader.getInstrumentName();

      std::string date("");

      if (loader.isGivenDate())
        date = loader.getDate();

      // Get a relevant IDF for a given instrument name and date. If date is
      // empty -
      // the most recent will be used.
      const std::string instrumentFilename =
          ExperimentInfo::getInstrumentFilename(instrumentName, date);

      // Load an instrument
      Algorithm_sptr childAlg = this->createChildAlgorithm("LoadInstrument");
      MatrixWorkspace_sptr tempWS(new DataObjects::Workspace2D());
      childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
      childAlg->setPropertyValue("Filename", instrumentFilename);
      childAlg->setProperty("RewriteSpectraMap", false);
      childAlg->executeAsChildAlg();
      mInstrument = tempWS->getInstrument();
    }

    // 2. Check if detector IDs are given
    if (!mInstrument) {
      std::map<int, std::vector<detid_t>>::iterator dit;
      for (dit = mGroupDetectorsMap.begin(); dit != mGroupDetectorsMap.end();
           ++dit) {
        if (dit->second.size() > 0)
          throw std::invalid_argument(
              "Grouping file specifies detector ID without instrument name");
      }
    }

    mGroupComponentsMap = loader.getGroupComponentsMap();
    mGroupDetectorsMap = loader.getGroupDetectorsMap();
    mGroupSpectraMap = loader.getGroupSpectraMap();

    // 3. Create output workspace
    this->intializeGroupingWorkspace();
    mGroupWS->mutableRun().addProperty("Filename", inputFile.toString());
    setProperty("OutputWorkspace", mGroupWS);

    // 4. Translate and set geometry
    this->setByComponents();
    this->setByDetectors();
    this->setBySpectrumIDs();

    // 5. Add grouping description, if specified
    if (loader.isGivenDescription()) {
      std::string description = loader.getDescription();
      mGroupWS->mutableRun().addProperty("Description", description);
    }

    // 6. Add group names, if user has specified any
    std::map<int, std::string> groupNamesMap = loader.getGroupNamesMap();

    for (auto it = groupNamesMap.begin(); it != groupNamesMap.end(); it++) {
      std::string groupIdStr = boost::lexical_cast<std::string>(it->first);
      mGroupWS->mutableRun().addProperty("GroupName_" + groupIdStr, it->second);
    }
  } else if (ext == "map") {
    // Deal with file as map

    // Load data from file
    LoadGroupMapFile loader(inputFile.toString(), g_log);
    loader.parseFile();

    // In .map files we are dealing with spectra numbers only.
    mGroupSpectraMap = loader.getGroupSpectraMap();

    // There is no way to specify instrument name in .map file
    generateNoInstrumentGroupWorkspace();

    mGroupWS->mutableRun().addProperty("Filename", inputFile.toString());
    setProperty("OutputWorkspace", mGroupWS);

    this->setBySpectrumIDs();
  } else {
    // Unknown file type
    throw std::invalid_argument("File type is not supported: " + ext);
  }
}

/*
 * Convert Componenet -> Detector IDs -> Workspace Indices -> set group ID
 */
void LoadDetectorsGroupingFile::setByComponents() {

  // 0. Check
  if (!mInstrument) {
    std::map<int, std::vector<std::string>>::iterator mapiter;
    bool norecord = true;
    for (mapiter = mGroupComponentsMap.begin();
         mapiter != mGroupComponentsMap.end(); ++mapiter) {
      if (mapiter->second.size() > 0) {
        g_log.error() << "Instrument is not specified in XML file.  "
                      << "But tag 'component' is used in XML file for Group "
                      << mapiter->first << " It is not allowed" << std::endl;
        norecord = false;
        break;
      }
    }
    if (!norecord)
      throw std::invalid_argument(
          "XML definition involving component causes error");
  }

  // 1. Prepare
  const detid2index_map indexmap =
      mGroupWS->getDetectorIDToWorkspaceIndexMap(true);

  // 2. Set
  for (std::map<int, std::vector<std::string>>::iterator it =
           mGroupComponentsMap.begin();
       it != mGroupComponentsMap.end(); ++it) {
    g_log.debug() << "Group ID = " << it->first << " With " << it->second.size()
                  << " Components" << std::endl;

    for (size_t i = 0; i < it->second.size(); i++) {

      // a) get component
      Geometry::IComponent_const_sptr component =
          mInstrument->getComponentByName(it->second[i]);

      // b) component -> component assembly --> children (more than detectors)
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);
      std::vector<Geometry::IComponent_const_sptr> children;
      asmb->getChildren(children, true);

      g_log.debug() << "Component Name = " << it->second[i]
                    << "  Component ID = " << component->getComponentID()
                    << "Number of Children = " << children.size() << std::endl;

      for (size_t ic = 0; ic < children.size(); ic++) {
        // c) convert component to detector
        Geometry::IComponent_const_sptr child = children[ic];
        Geometry::IDetector_const_sptr det =
            boost::dynamic_pointer_cast<const Geometry::IDetector>(child);

        if (det) {
          // Component is DETECTOR:
          int32_t detid = det->getID();
          detid2index_map::const_iterator itx = indexmap.find(detid);
          if (itx != indexmap.end()) {
            size_t wsindex = itx->second;
            mGroupWS->dataY(wsindex)[0] = it->first;
          } else {
            g_log.error() << "Pixel w/ ID = " << detid << " Cannot Be Located"
                          << std::endl;
          }
        } // ENDIF Detector

      } // ENDFOR (children of component)
    }   // ENDFOR (component)

  } // ENDFOR GroupID

  return;
}

/* Set workspace->group ID map by detectors (range)
 *
 */
void LoadDetectorsGroupingFile::setByDetectors() {

  // 0. Check
  if (!mInstrument && mGroupDetectorsMap.size() > 0) {
    std::map<int, std::vector<detid_t>>::iterator mapiter;
    bool norecord = true;
    for (mapiter = mGroupDetectorsMap.begin();
         mapiter != mGroupDetectorsMap.end(); ++mapiter)
      if (mapiter->second.size() > 0) {
        norecord = false;
        g_log.error() << "Instrument is not specified in XML file. "
                      << "But tag 'detid' is used in XML file for Group "
                      << mapiter->first << ". It is not allowed. " << std::endl;
        break;
      }

    if (!norecord)
      throw std::invalid_argument(
          "XML definition involving detectors causes error");
  }

  // 1. Prepare
  const detid2index_map indexmap =
      mGroupWS->getDetectorIDToWorkspaceIndexMap(true);

  // 2. Set GroupingWorkspace
  for (std::map<int, std::vector<detid_t>>::iterator it =
           mGroupDetectorsMap.begin();
       it != mGroupDetectorsMap.end(); ++it) {
    g_log.debug() << "Group ID = " << it->first << std::endl;

    for (size_t i = 0; i < it->second.size(); i++) {
      detid_t detid = it->second[i];
      detid2index_map::const_iterator itx = indexmap.find(detid);

      if (itx != indexmap.end()) {
        size_t wsindex = itx->second;
        mGroupWS->dataY(wsindex)[0] = it->first;
      } else {
        g_log.error() << "Pixel w/ ID = " << detid << " Cannot Be Located"
                      << std::endl;
      }
    } // ENDFOR detid (in range)
  }   // ENDFOR each group ID

  return;
}

/*
 * Set workspace index/group id by spectrum IDs
 */
void LoadDetectorsGroupingFile::setBySpectrumIDs() {
  // 1. Get map
  const spec2index_map s2imap = mGroupWS->getSpectrumToWorkspaceIndexMap();
  spec2index_map::const_iterator s2iter;

  // 2. Locate in loop
  //      std::map<int, std::vector<int> > mGroupSpectraMap;
  std::map<int, std::vector<int>>::iterator gsiter;
  for (gsiter = mGroupSpectraMap.begin(); gsiter != mGroupSpectraMap.end();
       ++gsiter) {
    int groupid = gsiter->first;
    for (size_t isp = 0; isp < gsiter->second.size(); isp++) {
      int specid = gsiter->second[isp];
      s2iter = s2imap.find(specid);
      if (s2iter == s2imap.end()) {
        g_log.error()
            << "Spectrum " << specid
            << " does not have an entry in GroupWorkspace's spec2index map"
            << std::endl;
        throw std::runtime_error("Logic error");
      } else {
        size_t wsindex = s2iter->second;
        if (wsindex >= mGroupWS->getNumberHistograms()) {
          g_log.error() << "Group workspace's spec2index map is set wrong: "
                        << " Found workspace index = " << wsindex
                        << " for spectrum ID " << specid
                        << " with workspace size = "
                        << mGroupWS->getNumberHistograms() << std::endl;
        } else {
          // Finally set the group workspace
          mGroupWS->dataY(wsindex)[0] = groupid;
        } // IF-ELSE: ws index out of range
      }   // IF-ELSE: spectrum ID has an entry
    }     // FOR: each spectrum ID
  }       // FOR: each group ID

  return;
}

/* Initialize a GroupingWorkspace
 *
 */
void LoadDetectorsGroupingFile::intializeGroupingWorkspace() {

  if (mInstrument) {
    // Create GroupingWorkspace with  instrument
    mGroupWS = DataObjects::GroupingWorkspace_sptr(
        new DataObjects::GroupingWorkspace(mInstrument));
  } else {
    // 1b. Create GroupingWorkspace w/o instrument
    generateNoInstrumentGroupWorkspace();
  }

  return;
}

/*
 * Generate a GroupingWorkspace without instrument information
 */
void LoadDetectorsGroupingFile::generateNoInstrumentGroupWorkspace() {
  // 1. Generate a map
  std::map<int, int> spectrumidgroupmap;
  std::map<int, std::vector<int>>::iterator groupspeciter;
  std::vector<int> specids;
  for (groupspeciter = mGroupSpectraMap.begin();
       groupspeciter != mGroupSpectraMap.end(); ++groupspeciter) {
    int groupid = groupspeciter->first;
    for (size_t i = 0; i < groupspeciter->second.size(); i++) {
      spectrumidgroupmap.insert(
          std::pair<int, int>(groupspeciter->second[i], groupid));
      specids.push_back(groupspeciter->second[i]);
    }
  }

  std::sort(specids.begin(), specids.end());

  if (specids.size() != spectrumidgroupmap.size()) {
    g_log.warning() << "Duplicate spectrum ID is defined in input XML file!"
                    << std::endl;
  }

  // 2. Initialize group workspace and set the spectrum workspace map
  size_t numvectors = spectrumidgroupmap.size();
  mGroupWS = DataObjects::GroupingWorkspace_sptr(
      new DataObjects::GroupingWorkspace(numvectors));

  for (size_t i = 0; i < mGroupWS->getNumberHistograms(); i++) {
    mGroupWS->getSpectrum(i)->setSpectrumNo(specids[i]);
  }

  return;
}

/*
 * Initialization
 */
LoadGroupXMLFile::LoadGroupXMLFile() {
  mStartGroupID = 1;
  return;
}

/*
 * Initialization
 */
LoadGroupXMLFile::~LoadGroupXMLFile() { return; }

void LoadGroupXMLFile::loadXMLFile(std::string xmlfilename) {

  this->initializeXMLParser(xmlfilename);
  this->parseXML();

  return;
}

/*
 * Initalize Poco XML Parser
 */
void LoadGroupXMLFile::initializeXMLParser(const std::string &filename) {
  const std::string xmlText = Kernel::Strings::loadFile(filename);

  // Set up the DOM parser and parse xml file
  Poco::XML::DOMParser pParser;
  try {
    pDoc = pParser.parseString(xmlText);
  } catch (Poco::Exception &exc) {
    throw Kernel::Exception::FileError(
        exc.displayText() + ". Unable to parse File:", filename);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", filename);
  }
  // Get pointer to root element
  pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "No root element in XML instrument file", filename);
  }
}

/*
 * Parse XML file
 */
void LoadGroupXMLFile::parseXML() {
  // 0. Check
  if (!pDoc)
    throw std::runtime_error(
        "Call LoadDetectorsGroupingFile::initialize() before parseXML.");

  // 1. Parse and create a structure
  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  int curgroupid = mStartGroupID - 1;
  bool isfirstgroup = true;

  // Add flag to figure out it is automatic group ID or user-defined group ID
  bool autogroupid = true;

  // While loop to go over all nodes!
  while (pNode) {

    const Poco::XML::XMLString value = pNode->innerText();

    if (pNode->nodeName().compare("detector-grouping") == 0) {
      // Node "detector-grouping" (first level)

      // Optional instrument name
      mInstrumentName =
          getAttributeValueByName(pNode, "instrument", mUserGiveInstrument);

      // Optional date for which is relevant
      mDate = getAttributeValueByName(pNode, "idf-date", mUserGiveDate);

      // Optional grouping description
      mDescription =
          getAttributeValueByName(pNode, "description", mUserGiveDescription);

    } // "detector-grouping"
    else if (pNode->nodeName().compare("group") == 0) {
      // Group Node:  get ID, set map
      // a) Find group ID
      bool foundid;
      std::string idstr = getAttributeValueByName(pNode, "ID", foundid);

      if (isfirstgroup && foundid)
        autogroupid = false;
      else if (!isfirstgroup && !autogroupid && foundid)
        autogroupid = false;
      else
        autogroupid = true;

      isfirstgroup = false;

      if (autogroupid) {
        curgroupid++;
      } else {
        curgroupid = atoi(idstr.c_str());
      }

      // b) Set in map
      std::map<int, std::vector<std::string>>::iterator itc =
          mGroupComponentsMap.find(curgroupid);
      if (itc != mGroupComponentsMap.end()) {
        // Error! Duplicate Group ID defined in XML
        std::stringstream ss;
        ss << "Map (group ID, components) has group ID " << curgroupid
           << " already.  Duplicate Group ID error!" << std::endl;
        throw std::invalid_argument(ss.str());
      } else {
        // When group ID is sorted, check if user has specified a group name
        bool foundName;
        std::string name = getAttributeValueByName(pNode, "name", foundName);
        if (foundName)
          mGroupNamesMap[curgroupid] = name;

        // Set map
        std::vector<std::string> tempcomponents;
        std::vector<detid_t> tempdetids;
        std::vector<int> tempspectrumids;
        mGroupComponentsMap[curgroupid] = tempcomponents;
        mGroupDetectorsMap[curgroupid] = tempdetids;
        mGroupSpectraMap[curgroupid] = tempspectrumids;
      }
    } // "group"
    else if (pNode->nodeName().compare("component") == 0) {
      // Node "component" = value
      std::map<int, std::vector<std::string>>::iterator it =
          mGroupComponentsMap.find(curgroupid);
      if (it == mGroupComponentsMap.end()) {
        std::stringstream ss;
        ss << "XML File (component) heirachial error!"
           << "  Inner Text = " << pNode->innerText() << std::endl;
        throw std::invalid_argument(ss.str());
      } else {
        bool valfound;
        std::string val_value =
            this->getAttributeValueByName(pNode, "val", valfound);
        std::string finalvalue;
        if (valfound && value.size() > 0)
          finalvalue = value + ", " + val_value;
        else if (value.size() == 0)
          finalvalue = val_value;
        else
          finalvalue = value;
        it->second.push_back(finalvalue);
      }

    } // Component
    else if (pNode->nodeName().compare("detids") == 0) {
      // Node "detids"
      std::map<int, std::vector<detid_t>>::iterator it =
          mGroupDetectorsMap.find(curgroupid);
      if (it == mGroupDetectorsMap.end()) {
        std::stringstream ss;
        ss << "XML File (detids) hierarchal error!"
           << "  Inner Text = " << pNode->innerText() << std::endl;
        throw std::invalid_argument(ss.str());
      } else {
        bool valfound;
        std::string val_value =
            this->getAttributeValueByName(pNode, "val", valfound);
        std::string finalvalue;
        if (valfound && value.size() > 0)
          finalvalue = value + ", " + val_value;
        else if (value.size() == 0)
          finalvalue = val_value;
        else
          finalvalue = value;

        std::vector<int> parsedRange = Strings::parseRange(finalvalue);
        it->second.insert(it->second.end(), parsedRange.begin(),
                          parsedRange.end());
      }
    } // "detids"
    else if (pNode->nodeName().compare("ids") == 0) {
      // Node ids: for spectrum number
      std::map<int, std::vector<int>>::iterator it =
          mGroupSpectraMap.find(curgroupid);
      if (it == mGroupSpectraMap.end()) {
        std::stringstream ss;
        ss << "XML File (ids) hierarchal error! "
           << "  Inner Text = " << pNode->innerText() << std::endl;
        throw std::invalid_argument(ss.str());
      } else {
        bool valfound;
        std::string val_value =
            this->getAttributeValueByName(pNode, "val", valfound);
        std::string finalvalue;
        if (valfound && value.size() > 0)
          finalvalue = value + ", " + val_value;
        else if (value.size() == 0)
          finalvalue = val_value;
        else
          finalvalue = value;

        std::vector<int> parsedRange = Strings::parseRange(finalvalue);
        it->second.insert(it->second.end(), parsedRange.begin(),
                          parsedRange.end());
      }
    }

    // Next Node!
    pNode = it.nextNode();

  } // ENDWHILE

  return;
}

/*
 * Get attribute's value by name from a Node
 */
std::string LoadGroupXMLFile::getAttributeValueByName(Poco::XML::Node *pNode,
                                                      std::string attributename,
                                                      bool &found) {
  // 1. Init
  Poco::XML::NamedNodeMap *att = pNode->attributes();
  found = false;
  std::string value = "";

  // 2. Loop to find
  for (unsigned long i = 0; i < att->length(); ++i) {
    Poco::XML::Node *cNode = att->item(i);
    if (cNode->localName().compare(attributename) == 0) {
      value = cNode->getNodeValue();
      found = true;
      break;
    }
  } // ENDFOR

  return value;
}

// -----------------------------------------------------------------------------------------------

/**
 * Constructor. Opens a file.
 * @param fileName Full path to the .map file
 * @param log Logger to use
 */
LoadGroupMapFile::LoadGroupMapFile(const std::string &fileName,
                                   Kernel::Logger &log)
    : m_fileName(fileName), m_log(log), m_lastLineRead(0) {
  m_file.open(m_fileName.c_str(), std::ifstream::in);

  if (!m_file)
    throw Exception::FileError("Couldn't open file for reading", fileName);
}

/**
 * Destructor. Closes the file.
 */
LoadGroupMapFile::~LoadGroupMapFile() {
  // Close the file, if terminate was not called
  if (m_file.is_open())
    m_file.close();
}

/**
 * Creates a group -> [spectra list] map by parsing the input file.
 */
void LoadGroupMapFile::parseFile() {
  std::string line;

  try {
    // We don't use the total number of groups report at the top of the file but
    // we'll tell them
    // later if there is a problem with it for their diagnostic purposes
    size_t givenNoOfGroups;

    if (!nextDataLine(line))
      throw std::invalid_argument(
          "The input file doesn't appear to contain any data");

    if (Kernel::Strings::convert(line, givenNoOfGroups) != 1)
      throw std::invalid_argument(
          "Expected a single int for the number of groups");

    // Parse groups
    int currentGroupNo = 1;
    while (true) {
      // Read next line ("group spectrum no.") -> ignore the number itself
      if (!nextDataLine(line))
        // If file ended -> no more groups to read, so exit the loop silently
        break;

      // Try to read number of spectra
      size_t noOfGroupSpectra;

      if (!nextDataLine(line))
        throw std::invalid_argument(
            "Premature end of file, expecting the number of group spectra");

      if (Kernel::Strings::convert(line, noOfGroupSpectra) != 1)
        throw std::invalid_argument(
            "Expected a single int for the number of group spectra");

      std::vector<int> &groupSpectra = m_groupSpectraMap[currentGroupNo];

      groupSpectra.reserve(noOfGroupSpectra);

      // While have not read all the group spectra
      while (groupSpectra.size() < noOfGroupSpectra) {
        if (!nextDataLine(line))
          throw std::invalid_argument(
              "Premature end of file, expecting spectra list");

        // Parse line with range. Exceptions will be catched as all others.
        std::vector<int> readSpectra = Kernel::Strings::parseRange(line, " ");

        groupSpectra.insert(groupSpectra.end(), readSpectra.begin(),
                            readSpectra.end());
      }

      if (groupSpectra.size() != noOfGroupSpectra)
        throw std::invalid_argument("Bad number of spectra list");

      currentGroupNo++;
    }

    if (m_groupSpectraMap.size() != givenNoOfGroups) {
      m_log.warning() << "The input file header states there are "
                      << givenNoOfGroups << ", but the file contains "
                      << m_groupSpectraMap.size() << " groups" << std::endl;
    }
  } catch (std::invalid_argument &e) {
    throw Exception::ParseError(e.what(), m_fileName, m_lastLineRead);
  }
}

/**
 * Returns a next data line. By "data line" I mean not empty and not a comment
 * line.
 * @param line Where a line string will be stored
 * @return true if line is read, false if eof or file error
 */
bool LoadGroupMapFile::nextDataLine(std::string &line) {
  while (m_file) {
    std::getline(m_file, line);
    m_lastLineRead++;

    if (!m_file)
      return false;

    line = Poco::trim(line);

    if (!line.empty() && line[0] != '#')
      return true;
  }

  return false;
}

} // namespace Mantid
} // namespace DataHandling
