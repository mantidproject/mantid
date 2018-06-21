#include "MantidTestHelpers/MuonGroupingXMLHelper.h"

#include "MantidAPI/GroupingLoader.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/XML/XMLWriter.h>

using namespace Mantid;
// using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace ScopedFileHelper;

namespace MuonGroupingXMLHelper {

/**
 * Simplest possible grouping file, with only a single group.
 * @param groupName :: Name of the group.
 * @param group :: Detector grouping string (e.g. "1-4,5,6-10").
 * @return ScopedFile object.
 */
ScopedFileHelper::ScopedFile
createGroupingXMLSingleGroup(const std::string &groupName,
                             const std::string &group) {
  std::string fileContents("");
  fileContents += "<detector-grouping description=\"test XML file\"> \n";
  fileContents += "\t<group name=\"" + groupName + "\"> \n";
  fileContents += "\t\t<ids val=\"" + group + "\"/>\n";
  fileContents += "\t</group>\n";
  fileContents += "\t<default name=\"" + groupName + "\"/>\n";
  fileContents += "</detector-grouping>";

  ScopedFileHelper::ScopedFile file(fileContents, "testXML_1.xml");
  return file;
}

/**
 * Grouping file with two groups (group1 and group2), combined into one pair.
 * @param pairName :: Name of the pair.
 * @param groupName :: Override the name of the second group of the pair, used
 * to test a possible failure case.
 * @return ScopedFile object.
 */
ScopedFileHelper::ScopedFile
createGroupingXMLSinglePair(const std::string &pairName,
                            const std::string &groupName) {

  std::string fileContents("");

  fileContents += "<detector-grouping description=\"test XML file\"> \n";
  fileContents += "\t<group name=\"group1\"> \n";
  fileContents += "\t\t<ids val=\"1\"/>\n";
  fileContents += "\t</group>\n";

  fileContents += "<detector-grouping description=\"test XML file\"> \n";
  fileContents += "\t<group name=\"group2\"> \n";
  fileContents += "\t\t<ids val=\"2\"/>\n";
  fileContents += "\t</group>\n";

  fileContents += "\t<pair name=\"" + pairName + "\"> \n";
  fileContents += "\t\t<forward-group val=\"group1\"/>\n";
  fileContents += "\t\t<backward-group val=\"" + groupName + "\"/>\n";
  fileContents += "\t\t<alpha val=\"1\"/>\n";
  fileContents += "\t</pair>\n";

  fileContents += "\t<default name=\"" + groupName + "\"/>\n";
  fileContents += "</detector-grouping>";

  ScopedFileHelper::ScopedFile file(fileContents, "testXML_1.xml");

  return file;
}

/**
 * Create an XML file with grouping/pairing information. With nGroups = 3 and
 * nDetectorPerGroup = 5 the grouping would be {"1-5","6-10","11-15"}.
 *
 * @param nGroups :: The number of groups to produce
 * @param nDetectorsPerGroup ::  The number of detector IDs per group
 * @return ScopedFile.
 */
ScopedFileHelper::ScopedFile
createXMLwithPairsAndGroups(const int &nGroups, const int &nDetectorsPerGroup) {

  API::Grouping grouping;
  std::string groupIDs;
  // groups
  for (auto group = 1; group <= nGroups; group++) {
    std::string groupName = "group" + std::to_string(group);
    if (nGroups == 1) {
      groupIDs = "1";
    } else {
      groupIDs = std::to_string((group - 1) * nDetectorsPerGroup + 1) + "-" +
                 std::to_string(group * nDetectorsPerGroup);
    }
    grouping.groupNames.emplace_back(groupName);
    grouping.groups.emplace_back(groupIDs);
  }
  // pairs
  for (auto pair = 1; pair < nGroups; pair++) {
    std::string pairName = "pair" + std::to_string(pair);
    std::pair<size_t, size_t> pairIndices;
    pairIndices.first = 0;
    pairIndices.second = pair;
    grouping.pairNames.emplace_back(pairName);
    grouping.pairAlphas.emplace_back(1.0);
    grouping.pairs.emplace_back(pairIndices);
  }

  auto fileContents = groupingToXML(grouping);
  ScopedFileHelper::ScopedFile file(fileContents, "testXML_1.xml");
  return file;
}

/**
 * Save grouping to the XML file specified.
 *
 * @param grouping :: Struct with grouping information
 * @param filename :: XML filename where information will be saved
 */
std::string groupingToXML(const Mantid::API::Grouping &grouping) {

  Poco::XML::DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT);

  Poco::AutoPtr<Poco::XML::Document> mDoc = new Poco::XML::Document();

  // Create root element with a description
  Poco::AutoPtr<Poco::XML::Element> rootElem =
      mDoc->createElement("detector-grouping");
  rootElem->setAttribute("description", grouping.description);
  mDoc->appendChild(rootElem);

  // Create group elements
  for (size_t gi = 0; gi < grouping.groups.size(); gi++) {
    Poco::AutoPtr<Poco::XML::Element> gElem = mDoc->createElement("group");
    gElem->setAttribute("name", grouping.groupNames[gi]);
    rootElem->appendChild(gElem);

    Poco::AutoPtr<Poco::XML::Element> idsElem = mDoc->createElement("ids");
    idsElem->setAttribute("val", grouping.groups[gi]);
    gElem->appendChild(idsElem);
  }

  // Create pair elements
  for (size_t pi = 0; pi < grouping.pairs.size(); pi++) {
    Poco::AutoPtr<Poco::XML::Element> gElem = mDoc->createElement("pair");
    gElem->setAttribute("name", grouping.pairNames[pi]);
    rootElem->appendChild(gElem);

    Poco::AutoPtr<Poco::XML::Element> fwElem =
        mDoc->createElement("forward-group");
    fwElem->setAttribute("val", grouping.groupNames[grouping.pairs[pi].first]);
    gElem->appendChild(fwElem);

    Poco::AutoPtr<Poco::XML::Element> bwElem =
        mDoc->createElement("backward-group");
    bwElem->setAttribute("val", grouping.groupNames[grouping.pairs[pi].second]);
    gElem->appendChild(bwElem);

    Poco::AutoPtr<Poco::XML::Element> alphaElem = mDoc->createElement("alpha");
    alphaElem->setAttribute(
        "val", boost::lexical_cast<std::string>(grouping.pairAlphas[pi]));
    gElem->appendChild(alphaElem);
  }

  // Create default group/pair name element
  Poco::AutoPtr<Poco::XML::Element> gElem = mDoc->createElement("default");
  gElem->setAttribute("name", grouping.defaultName);
  rootElem->appendChild(gElem);

  std::stringstream stream;
  writer.writeNode(stream, mDoc);
  return stream.str();
}

} // namespace MuonGroupingXMLHelper
