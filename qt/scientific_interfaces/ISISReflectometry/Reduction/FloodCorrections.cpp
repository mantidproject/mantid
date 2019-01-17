// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FloodCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

FloodCorrections::FloodCorrections(FloodCorrectionType correctionType,
                                   boost::optional<std::string> workspace)
    : m_correctionType(correctionType), m_workspace(workspace) {}

FloodCorrectionType FloodCorrections::correctionType() const {
  return m_correctionType;
}

boost::optional<std::string> FloodCorrections::workspace() const {
  return m_workspace;
}

bool operator!=(FloodCorrections const &lhs, FloodCorrections const &rhs) {
  return !(lhs == rhs);
}

bool operator==(FloodCorrections const &lhs, FloodCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType() &&
         lhs.workspace() == rhs.workspace();
}
} // namespace CustomInterfaces
} // namespace MantidQt
