// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/GroupingLoader.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

namespace MuonGroupingXMLHelper {

// Simplest possible grouping file, with only a single group
ScopedFileHelper::ScopedFile createGroupingXMLSingleGroup(const std::string &groupName, const std::string &group);
/**
 * Create an XML with two simple groups and a pair made from them. groupName
 * applies only to the pairing so that we can test a failure case.
 */
ScopedFileHelper::ScopedFile createGroupingXMLSinglePair(const std::string &pairName, const std::string &groupName);

/**
 * Create an XML file with grouping/pairing information. With nGroups = 3 and
 * nDetectorPerGroup = 5 the grouping would be {"1-5","6-10","11-15"}.
 */
ScopedFileHelper::ScopedFile createXMLwithPairsAndGroups(const int &nGroups = 1, const int &nDetectorsPerGroup = 1);

// Saves grouping to the XML file specified
DLLExport std::string groupingToXML(const Mantid::API::Grouping &grouping);

} // namespace MuonGroupingXMLHelper
