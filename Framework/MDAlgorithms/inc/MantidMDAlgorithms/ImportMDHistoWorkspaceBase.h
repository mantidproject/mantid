// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"
#include <vector>

namespace Mantid {
namespace MDAlgorithms {

/** ImportMDHistoWorkspaceBase : Base class for algorithms Importing data as
  MDHistoWorkspaces.

  @date 2012-06-21
*/
class MANTID_MDALGORITHMS_DLL ImportMDHistoWorkspaceBase : public API::Algorithm {
public:
  std::map<std::string, std::string> validateInputs() override;

protected:
  /// Creates an empty md histo workspace (with dimensions)
  DataObjects::MDHistoWorkspace_sptr createEmptyOutputWorkspace();
  /// Initialise the properties associated with the generic import (those to do
  /// with dimensionality).
  void initGenericImportProps();
  /// Getter for the number of bins (product accross all dimensions)
  size_t getBinProduct() const { return m_bin_product; }

private:
  // Product of the bins across all dimensions.
  size_t m_bin_product = 0;
  Mantid::Geometry::MDFrame_uptr createMDFrame(const std::string &frame, const std::string &unit);
  bool checkIfFrameValid(const std::string &frame, const std::vector<std::string> &targetFrames);
  std::vector<std::string> parseNames(const std::string &names_string);
};

} // namespace MDAlgorithms
} // namespace Mantid
