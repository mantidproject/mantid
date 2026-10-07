// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidMDAlgorithms/DllConfig.h"

#include <optional>
#include <vector>

namespace Mantid {
namespace MDAlgorithms {

/** ConvertHFIRSCDtoMDE : TODO: DESCRIPTION
 */
class MANTID_MDALGORITHMS_DLL ConvertHFIRSCDtoMDE : public API::BoxControllerSettingsAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertWANDSCDtoQ", "LoadWANDSCD"}; }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;
  bool checkGroups() override { return false; }

private:
  void init() override;
  void exec() override;
  std::string validateInputWorkspace(const API::IMDHistoWorkspace_sptr &inputWS) const;
  API::IMDEventWorkspace_sptr convertWorkspace(const API::IMDHistoWorkspace_sptr &inputWS, double wavelength);
  API::IMDEventWorkspace_sptr mergeWorkspaces(const std::vector<API::IMDEventWorkspace_sptr> &workspaces);
  static std::vector<API::IMDHistoWorkspace_sptr> inputWorkspaceList(const API::Workspace_sptr &input);
  static std::optional<double> fallbackWavelength(const std::vector<double> &wavelengths, size_t index);
  double resolveWavelength(const API::IMDHistoWorkspace &inputWS, const std::optional<double> &fallback) const;
};

} // namespace MDAlgorithms
} // namespace Mantid
