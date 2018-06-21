#ifndef MUONGROUPINGXMLHELPER_H_
#define MUONGROUPINGXMLHELPER_H_

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

namespace MuonGroupingXMLHelper {

// Simplest possible grouping file, with only a single group
ScopedFileHelper::ScopedFile
createGroupingXMLSingleGroup(const std::string &groupName,
                             const std::string &group);
/**
 * Create an XML with two simple groups and a pair made from them. groupName
 * applies only to the pairing so that we can test a failure case.
 */
ScopedFileHelper::ScopedFile
createGroupingXMLSinglePair(const std::string &pairName,
                            const std::string &groupName);

/**
 * Create an XML file with grouping/pairing information. With nGroups = 3 and
 * nDetectorPerGroup = 5 the grouping would be {"1-5","6-10","11-15"}.
 */
ScopedFileHelper::ScopedFile
createXMLwithPairsAndGroups(const int &nGroups = 1,
                            const int &nDetectorsPerGroup = 1);

} // namespace MuonGroupingXMLHelper

#endif /*MUONGROUPINGXMLHELPER_H_*/
