// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/GroupingWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** SaveDetectorsGrouping : TODO: DESCRIPTION

  @date 2011-11-16
*/
class MANTID_DATAHANDLING_DLL SaveDetectorsGrouping final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveDetectorsGrouping"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Save a GroupingWorkspace to an XML file."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LoadDetectorsGroupingFile", "GroupDetectors"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Grouping;Transforms\\Grouping"; }

private:
  /// Define input parameters
  void init() override;

  /// Main body to execute algorithm
  void exec() override;

  /// Create map for GroupID -- vector<detector ID>
  void createGroupDetectorIDMap(std::map<int, std::vector<detid_t>> &groupwkspmap);

  /// Convert vector of detector ID to range of Detector ID
  void convertToDetectorsRanges(std::map<int, std::vector<detid_t>> groupdetidsmap,
                                std::map<int, std::vector<detid_t>> &groupdetidrangemap);

  /// Print Grouping to XML file
  void printToXML(const std::map<int, std::vector<detid_t>> &groupdetidrangemap, const std::string &xmlfilename);

  // GroupingWorkspace
  DataObjects::GroupingWorkspace_const_sptr mGroupWS;
};

} // namespace DataHandling
} // namespace Mantid
