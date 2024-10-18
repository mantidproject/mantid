// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** Algorithm to save a 5-column ascii .cal file from  to 3 workspaces:
 * a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.
 *
 * @author
 * @date 2011-05-10 09:48:31.796980
 */
class MANTID_DATAHANDLING_DLL SaveCalFile final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveCalFile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 5-column ASCII .cal file from up to 3 workspaces: a "
           "GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveDiffCal",          "ReadGroupsFromFile", "CreateDummyCalFile", "CreateCalFileByNames",
            "DiffractionFocussing", "LoadCalFile",        "MergeCalFiles"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return R"(DataHandling\Text;Diffraction\DataHandling\CalFiles)"; }

  void saveCalFile(const std::string &calFileName, const Mantid::DataObjects::GroupingWorkspace_sptr &groupWS,
                   const Mantid::DataObjects::OffsetsWorkspace_sptr &offsetsWS,
                   const Mantid::DataObjects::MaskWorkspace_sptr &maskWS);

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Offset precision
  int m_precision{7};
};

} // namespace DataHandling
} // namespace Mantid
