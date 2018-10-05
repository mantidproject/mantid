// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "PerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

PerThetaDefaults::PerThetaDefaults(
    boost::optional<double> theta,
    std::pair<std::string, std::string> transmissionRuns, RangeInQ qRange,
    boost::optional<double> scaleFactor,
    boost::optional<ProcessingInstructions> processingInstructions)
    : m_theta(std::move(theta)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_qRange(std::move(qRange)), m_scaleFactor(std::move(scaleFactor)),
      m_processingInstructions(std::move(processingInstructions)) {}

std::pair<std::string, std::string> const &
PerThetaDefaults::transmissionWorkspaceNames() const {
  return m_transmissionRuns;
}

bool PerThetaDefaults::isWildcard() const { return !m_theta.is_initialized(); }

boost::optional<double> PerThetaDefaults::thetaOrWildcard() const {
  return m_theta;
}

RangeInQ const &PerThetaDefaults::qRange() const { return m_qRange; }

boost::optional<double> PerThetaDefaults::scaleFactor() const {
  return m_scaleFactor;
}

boost::optional<ProcessingInstructions>
PerThetaDefaults::processingInstructions() const {
  return m_processingInstructions;
}

bool operator==(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) {
  return lhs.thetaOrWildcard() == rhs.thetaOrWildcard() &&
         lhs.qRange() == rhs.qRange() &&
         lhs.scaleFactor() == rhs.scaleFactor() &&
         lhs.processingInstructions() == rhs.processingInstructions();
}

bool operator!=(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
