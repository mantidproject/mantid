// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidReflectometry/CreatePolarizationEfficienciesBase.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/** JoinISISPolarizationEfficiencies : Joins reflectometry polarization
  efficiency correction factors to form a single matrix workspace.
*/
class MANTID_REFLECTOMETRY_DLL JoinISISPolarizationEfficiencies
    : public CreatePolarizationEfficienciesBase {
public:
  const std::string name() const override;
  int version() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &props) override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &labels,
                     std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  std::vector<API::MatrixWorkspace_sptr> interpolateWorkspaces(
      std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  API::MatrixWorkspace_sptr
  interpolatePointDataWorkspace(const API::MatrixWorkspace_sptr &ws,
                                size_t const maxSize);
  API::MatrixWorkspace_sptr
  interpolateHistogramWorkspace(const API::MatrixWorkspace_sptr &ws,
                                size_t const maxSize);
};

} // namespace Reflectometry
} // namespace Mantid
