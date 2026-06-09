// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PolarizationCorrections.h"
namespace MantidQt::CustomInterfaces::ISISReflectometry {

PolarizationCorrections::PolarizationCorrections(PolarizationCorrectionType correctionType,
                                                 std::optional<std::string> workspace,
                                                 std::string const &inputSpinStateOrder)
    : m_correctionType(correctionType), m_workspace(workspace), m_inputSpinStateOrder(inputSpinStateOrder) {}

PolarizationCorrectionType PolarizationCorrections::correctionType() const { return m_correctionType; }

std::optional<std::string> PolarizationCorrections::workspace() const { return m_workspace; }

std::string const &PolarizationCorrections::inputSpinStateOrder() const { return m_inputSpinStateOrder; }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
