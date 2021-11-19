// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FloodCorrections.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

FloodCorrections::FloodCorrections(FloodCorrectionType correctionType, std::optional<std::string> workspace)
    : m_correctionType(correctionType), m_workspace(std::move(workspace)) {}

FloodCorrectionType FloodCorrections::correctionType() const { return m_correctionType; }

std::optional<std::string> FloodCorrections::workspace() const { return m_workspace; }

bool operator!=(FloodCorrections const &lhs, FloodCorrections const &rhs) { return !(lhs == rhs); }

bool operator==(FloodCorrections const &lhs, FloodCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType() && lhs.workspace() == rhs.workspace();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
