// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveDetectorsGrouping.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/System.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/XML/XMLWriter.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Poco::XML;

namespace {

bool hasExtention(std::string filename, const std::string &extention) {
  if (filename.length() < extention.length())
    return false;

  std::transform(filename.begin(), filename.end(), filename.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  return filename.compare(filename.length() - extention.length(),
                          extention.length(), extention) == 0;
}

int numberOfDetectorIDs(const std::vector<Mantid::detid_t> &detectorIDs) {
  return detectorIDs[1] - detectorIDs[0] + 1;
}

std::string
spaceSeperatedString(const std::vector<Mantid::detid_t> &detectorIDs) {
  if (detectorIDs[0] == detectorIDs[1]) {
    return std::to_string(detectorIDs[0]);
  } else {
    std::string detectorString;
    for (auto id = detectorIDs[0]; id <= detectorIDs[1]; ++id)
      detectorString += std::to_string(id) + " ";
    boost::algorithm::trim(detectorString);
    return detectorString;
  }
}

} // namespace

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(SaveDetectorsGrouping)

/// Define input parameters
void SaveDetectorsGrouping::init() {
  const std::initializer_list<std::string> extentions = {".xml", ".map"};

  declareProperty(
      std::make_unique<API::WorkspaceProperty<DataObjects::GroupingWorkspace>>(
          "InputWorkspace", "", Direction::Input),
      "GroupingWorkspace to output to a XML or MAP file (GroupingWorkspace)");
  declareProperty(
      std::make_unique<FileProperty>("OutputFile", "", FileProperty::Save,
                                     extentions),
      "File to save the detectors grouping to. Default is an XML file.");
}

/// Main body to execute algorithm
void SaveDetectorsGrouping::exec() {

  // 1. Get Input
  const std::string outputFilename = this->getProperty("OutputFile");
  mGroupWS = this->getProperty("InputWorkspace");

  // 2. Create Map(group ID, workspace-index vector)
  std::map<int, std::vector<detid_t>> groupIDwkspIDMap;
  this->createGroupDetectorIDMap(groupIDwkspIDMap);
  g_log.debug() << "Size of map = " << groupIDwkspIDMap.size() << '\n';

  // 3. Convert to detectors ranges
  std::map<int, std::vector<detid_t>> groupIDdetectorRangeMap;
  this->convertToDetectorsRanges(groupIDwkspIDMap, groupIDdetectorRangeMap);

  // 4. Print out
  if (hasExtention(outputFilename, "map"))
    printToMap(groupIDdetectorRangeMap, outputFilename);
  else
    printToXML(groupIDdetectorRangeMap, outputFilename);
}

/*
 * From GroupingWorkspace to generate a map.
 * Each entry, key = GroupID, value = vector of workspace index
 */
void SaveDetectorsGrouping::createGroupDetectorIDMap(
    std::map<int, std::vector<detid_t>> &groupwkspmap) {

  // 1. Create map
  for (size_t iws = 0; iws < mGroupWS->getNumberHistograms(); iws++) {
    // a) Group ID
    auto groupid = static_cast<int>(mGroupWS->y(iws)[0]);

    // b) Exist? Yes --> get handler on vector.  No --> create vector and
    auto it = groupwkspmap.find(groupid);
    if (it == groupwkspmap.end()) {
      std::vector<detid_t> tempvector;
      groupwkspmap[groupid] = tempvector;
    }
    it = groupwkspmap.find(groupid);
    if (it == groupwkspmap.end()) {
      throw std::invalid_argument(
          "Could not find group ID the after creating it in the map.");
    }

    // c) Convert workspace ID to detector ID
    const auto &mspec = mGroupWS->getSpectrum(iws);
    auto &detids = mspec.getDetectorIDs();
    if (detids.size() != 1) {
      g_log.error() << "Spectrum " << mspec.getSpectrumNo() << " has "
                    << detids.size() << " detectors.  Not allowed situation!\n";
      throw;
    }
    it->second.insert(it->second.end(), detids.begin(), detids.end());
  }
}

/*
 * Convert
 */
void SaveDetectorsGrouping::convertToDetectorsRanges(
    std::map<int, std::vector<detid_t>> groupdetidsmap,
    std::map<int, std::vector<detid_t>> &groupdetidrangemap) {

  for (auto &groupdetids : groupdetidsmap) {

    // a) Get handler of group ID and detector Id vector
    const int groupid = groupdetids.first;
    sort(groupdetids.second.begin(), groupdetids.second.end());

    g_log.debug() << "Group " << groupid << "  has "
                  << groupdetids.second.size() << " detectors. \n";

    // b) Group to ranges
    std::vector<detid_t> detranges;
    detid_t st = groupdetids.second[0];
    detid_t ed = st;
    for (size_t i = 1; i < groupdetids.second.size(); i++) {
      detid_t detid = groupdetids.second[i];
      if (detid == ed + 1) {
        // consecutive
        ed = detid;
      } else {
        // broken:  (1) store (2) start new
        detranges.emplace_back(st);
        detranges.emplace_back(ed);

        st = detid;
        ed = detid;
      }
    } // ENDFOR detectors
    // Complete the uncompleted
    detranges.emplace_back(st);
    detranges.emplace_back(ed);

    // c) Save entry in output
    groupdetidrangemap[groupid] = detranges;

  } // ENDFOR GroupID
}

void SaveDetectorsGrouping::printToXML(
    const std::map<int, std::vector<detid_t>> &groupdetidrangemap,
    const std::string &xmlfilename) {

  // 1. Get Instrument information
  const auto &instrument = mGroupWS->getInstrument();
  const std::string name = instrument->getName();
  g_log.debug() << "Instrument " << name << '\n';

  // 2. Start document (XML)
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-grouping");
  pDoc->appendChild(pRoot);
  pRoot->setAttribute("instrument", name);

  // Set description if was specified by user
  if (mGroupWS->run().hasProperty("Description")) {
    const std::string description =
        mGroupWS->run().getProperty("Description")->value();
    pRoot->setAttribute("description", description);
  }

  // 3. Append Groups
  for (const auto &groupdetidrange : groupdetidrangemap) {

    // a) Group Node
    const int groupid = groupdetidrange.first;
    std::stringstream sid;
    sid << groupid;

    AutoPtr<Element> pChildGroup = pDoc->createElement("group");
    pChildGroup->setAttribute("ID", sid.str());
    // Set name if was specified by user
    std::string groupNameProp = "GroupName_" + sid.str();
    if (mGroupWS->run().hasProperty(groupNameProp)) {
      const std::string groupName =
          mGroupWS->run().getProperty(groupNameProp)->value();
      pChildGroup->setAttribute("name", groupName);
    }

    pRoot->appendChild(pChildGroup);

    g_log.debug() << "Group ID = " << groupid << '\n';

    // b) Detector ID Child Nodes
    std::stringstream ss;

    for (size_t i = 0; i < groupdetidrange.second.size() / 2; i++) {
      // i. Generate text value

      detid_t ist = groupdetidrange.second[i * 2];
      detid_t ied = groupdetidrange.second[i * 2 + 1];
      // "a-b" or "a"
      if (ist < ied) {
        ss << ist << "-" << ied;
      } else if (ist == ied) {
        ss << ist;
      } else {
        throw std::invalid_argument("Impossible to have this sitaution!");
      }
      // add ","
      if (i < groupdetidrange.second.size() / 2 - 1) {
        ss << ",";
      }

      g_log.debug() << "Detectors:  " << groupdetidrange.second[i * 2] << ", "
                    << groupdetidrange.second[i * 2 + 1] << '\n';
    } // FOREACH Detectors Range Set

    const std::string textvalue = ss.str();

    g_log.debug() << "Detector IDs Node: " << textvalue << '\n';

    // c) Create element
    AutoPtr<Element> pDetid = pDoc->createElement("detids");
    AutoPtr<Text> pText1 = pDoc->createTextNode(textvalue);
    pDetid->appendChild(pText1);
    pChildGroup->appendChild(pDetid);

  } // FOREACH GroupID

  // 4. Write file
  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  std::ofstream ofs;
  ofs.open(xmlfilename.c_str(), std::fstream::out);

  ofs << "<?xml version=\"1.0\"?>\n";

  writer.writeNode(std::cout, pDoc);
  writer.writeNode(ofs, pDoc);
  ofs.close();
}

/** Creates a MAP file using the detector IDs found in a grouping workspace. The
 * format of a MAP file is as follows:
 *
 * Line 1        - Number of groups.
 *
 * For N = 2 to N = Number of groups:
 *
 * Line N        - Group ID.
 * Line N + 1    - Number of detectors in group.
 * Line N + 2    - Space seperated list of Detector IDs.
 */
void SaveDetectorsGrouping::printToMap(
    std::map<int, std::vector<detid_t>> &groupToDetectorIDsMap,
    const std::string &mapFilename) {
  const auto removeIter = groupToDetectorIDsMap.find(0);
  if (removeIter != groupToDetectorIDsMap.cend())
    groupToDetectorIDsMap.erase(removeIter);

  std::ofstream ofs;
  ofs.open(mapFilename.c_str(), std::fstream::out);
  ofs << std::to_string(groupToDetectorIDsMap.size()) + "\n";
  for (auto detectorIds : groupToDetectorIDsMap) {
    ofs << std::to_string(detectorIds.first) + "\n";
    ofs << std::to_string(numberOfDetectorIDs(detectorIds.second)) + "\n";
    ofs << spaceSeperatedString(detectorIds.second) + "\n";
  }

  ofs.close();
}

} // namespace DataHandling
} // namespace Mantid
