// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/PolSANSWorkspaceValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace API {

PolSANSWorkspaceValidator::PolSANSWorkspaceValidator(bool expectHistogramData, bool allowMultiPeriodData)
    : m_expectHistogramData(expectHistogramData), m_allowMultiPeriodData(allowMultiPeriodData) {};

Kernel::IValidator_sptr PolSANSWorkspaceValidator::clone() const {
  return std::make_shared<PolSANSWorkspaceValidator>(*this);
}

const std::string PolSANSWorkspaceValidator::validateGroupItem(API::MatrixWorkspace_sptr const &workspace) const {
  std::vector<std::string> workspaceIssues;
  if (!workspace) {
    return "All workspaces must be of type MatrixWorkspace.";
  }

  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    workspaceIssues.push_back("All workspaces must be in units of Wavelength.");
  }

  if (!m_allowMultiPeriodData && workspace->getNumberHistograms() != 1) {
    workspaceIssues.push_back("All workspaces must contain a single histogram.");
  }

  if (workspace->isHistogramData() != m_expectHistogramData) {
    if (m_expectHistogramData) {
      workspaceIssues.push_back("All workspaces must be histogram data.");
    } else {
      workspaceIssues.push_back("All workspaces must not be histogram data.");
    }
  }

  if (!workspaceIssues.empty()) {
    return Kernel::Strings::join(workspaceIssues.cbegin(), workspaceIssues.cend(), " ");
  }

  return "";
}

std::string PolSANSWorkspaceValidator::checkValidity(const WorkspaceGroup_sptr &workspace) const {

  if (workspace->getNumberOfEntries() != 4) {
    return "Input workspace group must have 4 entries.";
  }

  for (const API::Workspace_sptr &ws : workspace->getAllItems()) {
    const auto groupItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    const std::string errors = validateGroupItem(groupItem);
    if (errors != "") {
      return errors;
    }
  }

  return "";
}

} // namespace API
} // namespace Mantid
