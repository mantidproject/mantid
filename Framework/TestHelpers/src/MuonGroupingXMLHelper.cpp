#include "MantidTestHelpers/MuonGroupingXMLHelper.h"

//#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"
//#include "MantidTestHelpers/ComponentCreationHelper.h"
//#include "MantidTestHelpers/InstrumentCreationHelper.h"
//#include "MantidTestHelpers/WorkspaceCreationHelper.h"
//
#include "MantidMuon/MuonAlgorithmHelper.h"
//
//#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/GroupingLoader.h"
//#include "MantidAPI/MatrixWorkspace.h"
//#include "MantidAPI/ScopedWorkspace.h"
//#include "MantidAPI/TableRow.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidAPI/WorkspaceGroup.h"
//#include "MantidAlgorithms/CompareWorkspaces.h"
//#include "MantidDataHandling/LoadMuonNexus2.h"
//#include "MantidDataObjects/TableWorkspace.h"
//#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid;
//using namespace Mantid::Kernel;
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

  auto fileContents = MuonAlgorithmHelper::groupingToXML(grouping);
  ScopedFileHelper::ScopedFile file(fileContents, "testXML_1.xml");
  return file;
}

} // namespace MuonGroupingXMLHelper
