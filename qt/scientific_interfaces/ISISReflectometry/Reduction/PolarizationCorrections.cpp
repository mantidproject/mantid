// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PolarizationCorrections.h"
namespace MantidQt::CustomInterfaces::ISISReflectometry {

PolarizationCorrections::PolarizationCorrections(PolarizationCorrectionType correctionType,
                                                 boost::optional<std::string> workspace)
    : m_correctionType(correctionType), m_workspace(workspace) {}

PolarizationCorrectionType PolarizationCorrections::correctionType() const { return m_correctionType; }

boost::optional<std::string> PolarizationCorrections::workspace() const { return m_workspace; }

bool operator!=(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs) { return !(lhs == rhs); }

bool operator==(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType() && lhs.workspace() == rhs.workspace();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
