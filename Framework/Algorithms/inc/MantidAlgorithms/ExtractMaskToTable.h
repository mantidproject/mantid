// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace Algorithms {

/** ExtractMaskToTable : Extract the mask in a workspace to a table workspace.
  The table workspace must be compatible to algorithm MaskBinsFromTable.
*/
class MANTID_ALGORITHMS_DLL ExtractMaskToTable final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ExtractMaskToTable"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The output TableWorkspace should be compatible to "
           "MaskBinsFromTable.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ExtractMask"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Masking"; }

  /// Remove the items appeared in a vector from another
  std::vector<detid_t> subtractVector(const std::vector<detid_t> &minuend, std::vector<detid_t> subtrahend);

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Input matrix workspace
  API::MatrixWorkspace_const_sptr m_dataWS;
  /// Input table workspace
  DataObjects::TableWorkspace_sptr m_inputTableWS;

  /// Parse input TableWorkspace to get a list of detectors IDs of which
  /// detector are already masked
  std::vector<detid_t> parseMaskTable(const DataObjects::TableWorkspace_sptr &masktablews);

  /// Parse a string containing list in format (x, xx-yy, x, x, ...) to a vector
  /// of detid_t
  std::vector<detid_t> parseStringToVector(const std::string &liststr);

  /// Extract mask from a workspace to a list of detectors
  std::vector<detid_t> extractMaskFromMatrixWorkspace();

  /// Extract masked detectors from a MaskWorkspace
  std::vector<detid_t> extractMaskFromMaskWorkspace();

  /// Copy table workspace content from one workspace to another
  void copyTableWorkspaceContent(const DataObjects::TableWorkspace_sptr &sourceWS,
                                 const DataObjects::TableWorkspace_sptr &targetWS);

  /// Add a list of spectra (detector IDs) to the output table workspace
  void addToTableWorkspace(const DataObjects::TableWorkspace_sptr &outws, std::vector<detid_t> maskeddetids,
                           double xmin, double xmax, std::vector<detid_t> prevmaskedids);
};

} // namespace Algorithms
} // namespace Mantid
