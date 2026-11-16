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

/// Whether the instrument of the workspace asks for its units to be converted with a signed two theta
MANTID_API_DLL bool unitConversionUsesSignedTwoTheta(const MatrixWorkspace &workspace);

/// The fixed energy to convert the units of the workspace with: the one given, else Ei for direct geometry, if known
MANTID_API_DLL std::optional<double> unitConversionFixedEnergy(const MatrixWorkspace &workspace,
                                                               const Kernel::DeltaEMode::Type emode,
                                                               const std::optional<double> efixed);

/** Converts X values of single spectra between a workspace's X unit and another unit, as ConvertUnits converts whole
 * spectra. The geometry each conversion needs is looked up for every spectrum once, on construction, so that
 * converting a few values of many spectra, e.g. integration limits, does not cost as much as converting the workspace.
 * It must be made again if the geometry of the workspace changes.
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

  bool isFor(const MatrixWorkspace &workspace) const;
  bool canConvert(const size_t index) const;

  void fromWorkspaceUnit(const size_t index, std::span<double> values) const;
  void toWorkspaceUnit(const size_t index, std::span<double> values) const;

private:
  /// The parameters of Kernel::UnitParams, in order, with NaN for those not needed
  using Parameters = std::array<double, static_cast<size_t>(Kernel::UnitParams::tzero) + 1>;

  /// Converts a value by raising it to a power and multiplying it by a factor, which needs no geometry
  struct QuickConversion {
    double factor;
    double power;
  };

  void convert(const size_t index, std::span<double> values, const Kernel::Unit &from, const Kernel::Unit &to,
               const std::optional<QuickConversion> &quickConversion) const;

  /// The workspace this was made for. Only compared with, never dereferenced, as it may since have been deleted
  const MatrixWorkspace *m_workspace;
  size_t m_numberOfSpectra;
  Kernel::Unit_const_sptr m_workspaceUnit;
  Kernel::Unit_const_sptr m_unit;
  std::string m_workspaceUnitID;
  std::string m_unitID;
  Kernel::DeltaEMode::Type m_emode;
  std::optional<QuickConversion> m_quickFromWorkspaceUnit;
  std::optional<QuickConversion> m_quickToWorkspaceUnit;
  double m_l1{0.0};
  /// For each spectrum, or none if the spectrum cannot be converted. Empty if both conversions are quick
  std::vector<std::optional<Parameters>> m_parameters;
};

} // namespace Mantid::API
