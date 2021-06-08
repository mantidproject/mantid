// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

PerThetaDefaults::PerThetaDefaults(boost::optional<double> theta,
                                   // cppcheck-suppress passedByValue
                                   TransmissionRunPair transmissionRuns,
                                   boost::optional<ProcessingInstructions> transmissionProcessingInstructions,
                                   RangeInQ qRange, boost::optional<double> scaleFactor,
                                   boost::optional<ProcessingInstructions> processingInstructions,
                                   boost::optional<ProcessingInstructions> backgroundProcessingInstructions)
    : m_theta(std::move(theta)), m_transmissionRuns(std::move(transmissionRuns)), m_qRange(std::move(qRange)),
      m_scaleFactor(std::move(scaleFactor)),
      m_transmissionProcessingInstructions(std::move(transmissionProcessingInstructions)),
      m_processingInstructions(std::move(processingInstructions)),
      m_backgroundProcessingInstructions(std::move(backgroundProcessingInstructions)) {}

TransmissionRunPair const &PerThetaDefaults::transmissionWorkspaceNames() const { return m_transmissionRuns; }

bool PerThetaDefaults::isWildcard() const { return !m_theta.is_initialized(); }

boost::optional<double> PerThetaDefaults::thetaOrWildcard() const { return m_theta; }

RangeInQ const &PerThetaDefaults::qRange() const { return m_qRange; }

boost::optional<double> PerThetaDefaults::scaleFactor() const { return m_scaleFactor; }

boost::optional<ProcessingInstructions> PerThetaDefaults::processingInstructions() const {
  return m_processingInstructions;
}

boost::optional<ProcessingInstructions> PerThetaDefaults::transmissionProcessingInstructions() const {
  return m_transmissionProcessingInstructions;
}

boost::optional<ProcessingInstructions> PerThetaDefaults::backgroundProcessingInstructions() const {
  return m_backgroundProcessingInstructions;
}

bool operator==(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) {
  return lhs.thetaOrWildcard() == rhs.thetaOrWildcard() && lhs.qRange() == rhs.qRange() &&
         lhs.scaleFactor() == rhs.scaleFactor() &&
         lhs.transmissionProcessingInstructions() == rhs.transmissionProcessingInstructions() &&
         lhs.processingInstructions() == rhs.processingInstructions() &&
         lhs.backgroundProcessingInstructions() == rhs.backgroundProcessingInstructions();
}

bool operator!=(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) { return !(lhs == rhs); }

PerThetaDefaults::ValueArray perThetaDefaultsToArray(PerThetaDefaults const &perThetaDefaults) {
  auto result = PerThetaDefaults::ValueArray();
  if (perThetaDefaults.thetaOrWildcard())
    result[0] = std::to_string(*perThetaDefaults.thetaOrWildcard());
  result[1] = perThetaDefaults.transmissionWorkspaceNames().firstRunList();
  result[2] = perThetaDefaults.transmissionWorkspaceNames().secondRunList();
  if (perThetaDefaults.transmissionProcessingInstructions())
    result[3] = *perThetaDefaults.transmissionProcessingInstructions();
  if (perThetaDefaults.qRange().min())
    result[4] = std::to_string(*perThetaDefaults.qRange().min());
  if (perThetaDefaults.qRange().max())
    result[5] = std::to_string(*perThetaDefaults.qRange().max());
  if (perThetaDefaults.qRange().step())
    result[6] = std::to_string(*perThetaDefaults.qRange().step());
  if (perThetaDefaults.scaleFactor())
    result[7] = std::to_string(*perThetaDefaults.scaleFactor());
  if (perThetaDefaults.processingInstructions())
    result[8] = *perThetaDefaults.processingInstructions();
  if (perThetaDefaults.backgroundProcessingInstructions())
    result[9] = *perThetaDefaults.backgroundProcessingInstructions();
  return result;
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
