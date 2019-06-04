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
    // cppcheck-suppress passedByValue
    TransmissionRunPair transmissionRuns, RangeInQ qRange,
    boost::optional<double> scaleFactor,
    boost::optional<ProcessingInstructions> processingInstructions)
    : m_theta(std::move(theta)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_qRange(std::move(qRange)), m_scaleFactor(std::move(scaleFactor)),
      m_processingInstructions(std::move(processingInstructions)) {}

TransmissionRunPair const &
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

std::array<std::string, 8>
perThetaDefaultsToArray(PerThetaDefaults const &perThetaDefaults) {
  auto result = std::array<std::string, 8>();
  if (perThetaDefaults.thetaOrWildcard())
    result[0] = std::to_string(*perThetaDefaults.thetaOrWildcard());
  result[1] = perThetaDefaults.transmissionWorkspaceNames().firstRunList();
  result[2] = perThetaDefaults.transmissionWorkspaceNames().secondRunList();
  if (perThetaDefaults.qRange().min())
    result[3] = std::to_string(*perThetaDefaults.qRange().min());
  if (perThetaDefaults.qRange().max())
    result[4] = std::to_string(*perThetaDefaults.qRange().max());
  if (perThetaDefaults.qRange().step())
    result[5] = std::to_string(*perThetaDefaults.qRange().step());
  if (perThetaDefaults.scaleFactor())
    result[6] = std::to_string(*perThetaDefaults.scaleFactor());
  if (perThetaDefaults.processingInstructions())
    result[7] = *perThetaDefaults.processingInstructions();
  return result;
}
} // namespace CustomInterfaces
} // namespace MantidQt
