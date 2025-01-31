// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PolarizationCorrections.h"
namespace MantidQt::CustomInterfaces::ISISReflectometry {

PolarizationCorrections::PolarizationCorrections(PolarizationCorrectionType correctionType,
                                                 boost::optional<std::string> workspace,
                                                 std::string const &fredrikzeSpinStateOrder)
    : m_correctionType(correctionType), m_workspace(workspace), m_fredrikzeSpinStateOrder(fredrikzeSpinStateOrder) {}

PolarizationCorrectionType PolarizationCorrections::correctionType() const { return m_correctionType; }

boost::optional<std::string> PolarizationCorrections::workspace() const { return m_workspace; }

std::string const &PolarizationCorrections::fredrikzeSpinStateOrder() const { return m_fredrikzeSpinStateOrder; }

bool operator!=(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs) { return !(lhs == rhs); }

bool operator==(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType() && lhs.workspace() == rhs.workspace() &&
         lhs.fredrikzeSpinStateOrder() == rhs.fredrikzeSpinStateOrder();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
