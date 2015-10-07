#include <algorithm>
#include <fstream>
#include <sstream>

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidDataHandling/SaveDetectorsGrouping.h"
#include "MantidKernel/System.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>

#ifdef _MSC_VER
// Disable a flood of warnings from Poco about inheriting from
// std::basic_istream
// See
// http://connect.microsoft.com/VisualStudio/feedback/details/733720/inheriting-from-std-fstream-produces-c4250-warning
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

#include <Poco/XML/XMLWriter.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Poco::XML;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(SaveDetectorsGrouping)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveDetectorsGrouping::SaveDetectorsGrouping() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveDetectorsGrouping::~SaveDetectorsGrouping() {}

/// Define input parameters
void SaveDetectorsGrouping::init() {
  declareProperty(
      new API::WorkspaceProperty<DataObjects::GroupingWorkspace>(
          "InputWorkspace", "", Direction::Input),
      "GroupingWorkspace to output to XML file (GroupingWorkspace)");
  declareProperty(
      new FileProperty("OutputFile", "", FileProperty::Save, ".xml"),
      "File to save the detectors mask in XML format");
}

/// Main body to execute algorithm
void SaveDetectorsGrouping::exec() {

  // 1. Get Input
  std::string xmlfilename = this->getProperty("OutputFile");
  mGroupWS = this->getProperty("InputWorkspace");

  // 2. Create Map(group ID, workspace-index vector)
  std::map<int, std::vector<detid_t>> groupIDwkspIDMap;
  this->createGroupDetectorIDMap(groupIDwkspIDMap);
  g_log.debug() << "Size of map = " << groupIDwkspIDMap.size() << std::endl;

  // 3. Convert to detectors ranges
  std::map<int, std::vector<detid_t>> groupIDdetectorRangeMap;
  this->convertToDetectorsRanges(groupIDwkspIDMap, groupIDdetectorRangeMap);

  // 4. Print out
  this->printToXML(groupIDdetectorRangeMap, xmlfilename);
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
    int groupid = static_cast<int>(mGroupWS->dataY(iws)[0]);

    // b) Exist? Yes --> get hanlder on vector.  No --> create vector and
    std::map<int, std::vector<detid_t>>::iterator it =
        groupwkspmap.find(groupid);
    if (it == groupwkspmap.end()) {
      std::vector<detid_t> tempvector;
      // groupwkspmap.insert(std::pair<int, std::vector<detid_t> >(groupid,
      // tempvector));
      groupwkspmap[groupid] = tempvector;
    }
    it = groupwkspmap.find(groupid);
    if (it == groupwkspmap.end()) {
      g_log.error() << "Impossible situation! " << std::endl;
      throw std::invalid_argument("Cannot Happen!");
    }

    // c) Convert workspace ID to detector ID
    const API::ISpectrum *mspec = mGroupWS->getSpectrum(iws);
    if (!mspec) {
      g_log.error() << "Workspace index " << iws
                    << " has no spectrum.  Impossible!" << std::endl;
      throw;
    }
    std::set<detid_t> detids = mspec->getDetectorIDs();
    if (detids.size() != 1) {
      g_log.error() << "Spectrum " << mspec->getSpectrumNo() << " has "
                    << detids.size() << " detectors.  Not allowed situation!"
                    << std::endl;
      throw;
    }
    detid_t detid = 0;
    for (std::set<detid_t>::iterator it = detids.begin(); it != detids.end();
         ++it) {
      detid = *it;
    }
    it->second.push_back(detid);
  }

  return;
}

/*
 * Convert
 */
void SaveDetectorsGrouping::convertToDetectorsRanges(
    std::map<int, std::vector<detid_t>> groupdetidsmap,
    std::map<int, std::vector<detid_t>> &groupdetidrangemap) {

  for (std::map<int, std::vector<detid_t>>::iterator it =
           groupdetidsmap.begin();
       it != groupdetidsmap.end(); ++it) {

    // a) Get handler of group ID and detector Id vector
    int groupid = it->first;
    sort(it->second.begin(), it->second.end());

    g_log.debug() << "Group " << groupid << "  has " << it->second.size()
                  << " detectors. " << std::endl;

    // b) Group to ranges
    std::vector<detid_t> detranges;
    detid_t st = it->second[0];
    detid_t ed = st;
    for (size_t i = 1; i < it->second.size(); i++) {
      detid_t detid = it->second[i];
      if (detid == ed + 1) {
        // consecutive
        ed = detid;
      } else {
        // broken:  (1) store (2) start new
        detranges.push_back(st);
        detranges.push_back(ed);

        st = detid;
        ed = detid;
      }
    } // ENDFOR detectors
    // Complete the uncompleted
    detranges.push_back(st);
    detranges.push_back(ed);

    // c) Save entry in output
    groupdetidrangemap[groupid] = detranges;

  } // ENDFOR GroupID

  return;
}

void SaveDetectorsGrouping::printToXML(
    std::map<int, std::vector<detid_t>> groupdetidrangemap,
    std::string xmlfilename) {

  // 1. Get Instrument information
  Geometry::Instrument_const_sptr instrument = mGroupWS->getInstrument();
  std::string name = instrument->getName();
  g_log.debug() << "Instrument " << name << std::endl;

  // 2. Start document (XML)
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-grouping");
  pDoc->appendChild(pRoot);
  pRoot->setAttribute("instrument", name);

  // Set description if was specified by user
  if (mGroupWS->run().hasProperty("Description")) {
    std::string description =
        mGroupWS->run().getProperty("Description")->value();
    pRoot->setAttribute("description", description);
  }

  // 3. Append Groups
  for (std::map<int, std::vector<detid_t>>::iterator it =
           groupdetidrangemap.begin();
       it != groupdetidrangemap.end(); ++it) {

    // a) Group Node
    int groupid = it->first;
    std::stringstream sid;
    sid << groupid;

    AutoPtr<Element> pChildGroup = pDoc->createElement("group");
    pChildGroup->setAttribute("ID", sid.str());
    // Set name if was specified by user
    std::string groupNameProp = "GroupName_" + sid.str();
    if (mGroupWS->run().hasProperty(groupNameProp)) {
      std::string groupName =
          mGroupWS->run().getProperty(groupNameProp)->value();
      pChildGroup->setAttribute("name", groupName);
    }

    pRoot->appendChild(pChildGroup);

    g_log.debug() << "Group ID = " << groupid << std::endl;

    // b) Detector ID Child Nodes
    std::stringstream ss;

    for (size_t i = 0; i < it->second.size() / 2; i++) {
      // i. Generate text value

      bool writedata = true;
      detid_t ist = it->second[i * 2];
      detid_t ied = it->second[i * 2 + 1];
      // "a-b" or "a"
      if (ist < ied) {
        ss << ist << "-" << ied;
      } else if (ist == ied) {
        ss << ist;
      } else {
        writedata = false;
        g_log.error() << "Impossible to have this situation!" << std::endl;
        throw std::invalid_argument("Impossible to have this sitaution!");
      }
      // add ","
      if (writedata && i < it->second.size() / 2 - 1) {
        ss << ",";
      }

      g_log.debug() << "Detectors:  " << it->second[i * 2] << ", "
                    << it->second[i * 2 + 1] << std::endl;
    } // FOREACH Detectors Range Set

    std::string textvalue = ss.str();

    g_log.debug() << "Detector IDs Node: " << textvalue << std::endl;

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

} // namespace Mantid
} // namespace DataHandling
