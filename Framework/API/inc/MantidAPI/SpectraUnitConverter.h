// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Unit.h"

#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Mantid::API {
class MatrixWorkspace;

/** Converts X values of single spectra between a workspace's X unit and another unit, as ConvertUnits converts whole
 * spectra. The geometry each conversion needs is looked up for every spectrum once, on construction, so that
 * converting a few values of many spectra, e.g. integration limits, does not cost as much as converting the workspace.
 */
class MANTID_API_DLL SpectraUnitConverter {
public:
  SpectraUnitConverter(const MatrixWorkspace &workspace, const std::string &unit,
                       const Kernel::DeltaEMode::Type emode = Kernel::DeltaEMode::Elastic,
                       const std::optional<double> efixed = std::nullopt);

  /// The unit converted to and from the workspace's X unit
  const std::string &unit() const { return m_unitID; }
  /// The X unit of the workspace this was made for
  const std::string &workspaceUnit() const { return m_workspaceUnitID; }
  /// The number of spectra of the workspace this was made for
  size_t size() const { return m_parameters.size(); }

  void fromWorkspaceUnit(const size_t index, std::span<double> values) const;
  void toWorkspaceUnit(const size_t index, std::span<double> values) const;

private:
  /// The parameters of Kernel::UnitParams, in order, with NaN for those not needed
  using Parameters = std::array<double, 7>;

  void convert(const size_t index, std::span<double> values, const Kernel::Unit &from, const Kernel::Unit &to) const;

  Kernel::Unit_const_sptr m_workspaceUnit;
  Kernel::Unit_const_sptr m_unit;
  std::string m_workspaceUnitID;
  std::string m_unitID;
  double m_l1;
  Kernel::DeltaEMode::Type m_emode;
  /// For each spectrum, or none if the spectrum cannot be converted
  std::vector<std::optional<Parameters>> m_parameters;
};

} // namespace Mantid::API
