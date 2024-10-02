// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Strings.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <memory>
#include <utility>

using namespace Poco::XML;

namespace Mantid::API {

//----------------------------------------------------------------------------------------------
/** Constructor without field direction
 * @param instrument :: [input] Instrument
 */
GroupingLoader::GroupingLoader(Geometry::Instrument_const_sptr instrument) : m_instrument(std::move(instrument)) {}

/** Constructor with field direction
 * @param instrument :: [input] Instrument
 * @param mainFieldDirection :: [input] Direction of main field (for MUSR)
 */
GroupingLoader::GroupingLoader(Geometry::Instrument_const_sptr instrument, std::string mainFieldDirection)
    : m_instrument(std::move(instrument)), m_mainFieldDirection(std::move(mainFieldDirection)) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GroupingLoader::~GroupingLoader() = default;

/**
 * Attempts to load a grouping information referenced by IDF.
 * @return Grouping information
 */
std::shared_ptr<Grouping> GroupingLoader::getGroupingFromIDF() const {
  std::string parameterName = "Default grouping file";
  auto loadedGrouping = std::make_shared<Grouping>();

  // Special case for MUSR or CHRONUS, because it has two possible groupings
  if (m_instrument->getName() == "MUSR" || m_instrument->getName() == "CHRONUS") {
    parameterName.append(" - " + m_mainFieldDirection);
  }

  std::vector<std::string> groupingFiles = m_instrument->getStringParameter(parameterName);

  if (groupingFiles.size() == 1) {
    const std::string groupingFile = groupingFiles[0];

    // Get search directory for XML instrument definition files (IDFs)
    std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

    loadGroupingFromXML(directoryName + groupingFile, *loadedGrouping);
  } else {
    throw std::runtime_error("Multiple groupings specified for the instrument");
  }
  return loadedGrouping;
}

/**
 * Loads grouping from the XML file specified.
 *
 * @param filename :: XML filename to load grouping information from
 * @param grouping :: Struct to store grouping information to
 */
void GroupingLoader::loadGroupingFromXML(const std::string &filename, Grouping &grouping) {
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parse(filename);
  } catch (...) {
    throw Mantid::Kernel::Exception::FileError("Unable to parse File", filename);
  }

  // Get pointer to root element
  Element *pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes())
    throw Mantid::Kernel::Exception::FileError("No root element in XML grouping file", filename);

  // Parse information for groups
  Poco::AutoPtr<NodeList> groups = pRootElem->getElementsByTagName("group");
  if (groups->length() == 0)
    throw Mantid::Kernel::Exception::FileError("No groups specified in XML grouping file", filename);

  // Resize vectors
  grouping.groupNames.resize(groups->length());
  grouping.groups.resize(groups->length());

  for (size_t ig = 0; ig < groups->length(); ig++) {
    auto *pGroupElem = static_cast<Element *>(groups->item(static_cast<long>(ig)));

    if (!pGroupElem->hasAttribute("name"))
      throw Mantid::Kernel::Exception::FileError("Group element without name", filename);

    grouping.groupNames[ig] = pGroupElem->getAttribute("name");

    Element *idlistElement = pGroupElem->getChildElement("ids");
    if (!idlistElement)
      throw Mantid::Kernel::Exception::FileError("Group element without <ids>", filename);

    grouping.groups[ig] = idlistElement->getAttribute("val");
  }

  // Parse information for pairs
  Poco::AutoPtr<NodeList> pairs = pRootElem->getElementsByTagName("pair");

  // Resize vectors
  grouping.pairNames.resize(pairs->length());
  grouping.pairs.resize(pairs->length());
  grouping.pairAlphas.resize(pairs->length());

  for (size_t ip = 0; ip < pairs->length(); ip++) {
    auto *pPairElem = static_cast<Element *>(pairs->item(static_cast<long>(ip)));

    if (!pPairElem->hasAttribute("name"))
      throw Mantid::Kernel::Exception::FileError("Pair element without name", filename);

    grouping.pairNames[ip] = pPairElem->getAttribute("name");

    size_t fwdGroupId, bwdGroupId; // Ids of forward/backward groups

    // Try to get id of the first group
    if (Element *fwdElement = pPairElem->getChildElement("forward-group")) {
      if (!fwdElement->hasAttribute("val"))
        throw Mantid::Kernel::Exception::FileError("Pair forward-group without <val>", filename);

      // Find the group with the given name
      auto it = std::find(grouping.groupNames.begin(), grouping.groupNames.end(), fwdElement->getAttribute("val"));

      if (it == grouping.groupNames.end())
        throw Mantid::Kernel::Exception::FileError("Pair forward-group name not recognized", filename);

      // Get index of the iterator
      fwdGroupId = it - grouping.groupNames.begin();
    } else {
      throw Mantid::Kernel::Exception::FileError("Pair element without <forward-group>", filename);
    }

    // Try to get id of the second group
    if (Element *bwdElement = pPairElem->getChildElement("backward-group")) {
      if (!bwdElement->hasAttribute("val"))
        throw Mantid::Kernel::Exception::FileError("Pair backward-group without <val>", filename);

      // Find the group with the given name
      auto it = std::find(grouping.groupNames.begin(), grouping.groupNames.end(), bwdElement->getAttribute("val"));

      if (it == grouping.groupNames.end())
        throw Mantid::Kernel::Exception::FileError("Pair backward-group name not recognized", filename);

      // Get index of the iterator
      bwdGroupId = it - grouping.groupNames.begin();
    } else {
      throw Mantid::Kernel::Exception::FileError("Pair element without <backward-group>", filename);
    }

    grouping.pairs[ip] = std::make_pair(fwdGroupId, bwdGroupId);

    // Try to get alpha element
    if (Element *aElement = pPairElem->getChildElement("alpha")) {
      if (!aElement->hasAttribute("val"))
        throw Mantid::Kernel::Exception::FileError("Pair alpha element with no <val>", filename);

      try // ... to convert value to double
      {
        grouping.pairAlphas[ip] = boost::lexical_cast<double>(aElement->getAttribute("val"));
      } catch (boost::bad_lexical_cast &) {
        throw Mantid::Kernel::Exception::FileError("Pair alpha value is not a number", filename);
      }
    }
    // If alpha element not there, default it to 1.0
    else {
      grouping.pairAlphas[ip] = 1.0;
    }
  }

  // Try to get description
  if (pRootElem->hasAttribute("description")) {
    grouping.description = pRootElem->getAttribute("description");
  }

  // Try to get default group/pair name
  if (Element *defaultElement = pRootElem->getChildElement("default")) {
    if (!defaultElement->hasAttribute("name"))
      throw Mantid::Kernel::Exception::FileError("Default element with no <name>", filename);

    grouping.defaultName = defaultElement->getAttribute("name");
  }
}

/**
 * Returns a "dummy" grouping: a single group with all the detectors in it.
 * @return Grouping information
 */
std::shared_ptr<Grouping> GroupingLoader::getDummyGrouping() {
  // Group with all the detectors
  std::ostringstream all;
  all << "1-" << m_instrument->getNumberDetectors();

  auto dummyGrouping = std::make_shared<Mantid::API::Grouping>();
  dummyGrouping->description = "Dummy grouping";
  dummyGrouping->groupNames.emplace_back("all");
  dummyGrouping->groups.emplace_back(all.str());
  return dummyGrouping;
}

//----------------------------------------------------------------------------------------------
/**
 * Construct a Grouping from a table
 * @param table :: [input] Table to construct from
 */
Grouping::Grouping(const ITableWorkspace_sptr &table) {
  for (size_t row = 0; row < table->rowCount(); ++row) {
    std::vector<int> detectors = table->cell<std::vector<int>>(row, 0);

    // toString() expects the sequence to be sorted
    std::sort(detectors.begin(), detectors.end());

    // Convert to a range string, i.e. 1-5,6-8,9
    std::string detectorRange = Kernel::Strings::toString(detectors);

    this->groupNames.emplace_back(std::to_string(row + 1));
    this->groups.emplace_back(detectorRange);
  }

  // If we have 2 groups only - create a longitudinal pair
  if (this->groups.size() == 2) {
    this->pairNames.emplace_back("long");
    this->pairAlphas.emplace_back(1.0);
    this->pairs.emplace_back(0, 1);
  }
}

/**
 * Converts a grouping information to a grouping table. Discard all the
 * information not stored in a
 * table - group names, pairing, description.
 * @return A grouping table as accepted by MuonGroupDetectors
 */
ITableWorkspace_sptr Grouping::toTable() const {
  auto newTable =
      std::dynamic_pointer_cast<ITableWorkspace>(WorkspaceFactory::Instance().createTable("TableWorkspace"));

  newTable->addColumn("vector_int", "Detectors");

  for (const auto &group : this->groups) {
    TableRow newRow = newTable->appendRow();
    newRow << Kernel::Strings::parseRange(group);
  }

  return newTable;
}

/**
 * Define destructor here because the type is complete
 */
Grouping::~Grouping() = default;

} // namespace Mantid::API
