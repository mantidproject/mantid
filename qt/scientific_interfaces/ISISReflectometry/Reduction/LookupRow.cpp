// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "LookupRow.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

LookupRow::LookupRow(boost::optional<double> theta,
                     // cppcheck-suppress passedByValue
                     TransmissionRunPair transmissionRuns,
                     boost::optional<ProcessingInstructions> transmissionProcessingInstructions, RangeInQ qRange,
                     boost::optional<double> scaleFactor,
                     boost::optional<ProcessingInstructions> processingInstructions,
                     boost::optional<ProcessingInstructions> backgroundProcessingInstructions)
    : m_theta(std::move(theta)), m_transmissionRuns(std::move(transmissionRuns)), m_qRange(std::move(qRange)),
      m_scaleFactor(std::move(scaleFactor)),
      m_transmissionProcessingInstructions(std::move(transmissionProcessingInstructions)),
      m_processingInstructions(std::move(processingInstructions)),
      m_backgroundProcessingInstructions(std::move(backgroundProcessingInstructions)) {}

TransmissionRunPair const &LookupRow::transmissionWorkspaceNames() const { return m_transmissionRuns; }

bool LookupRow::isWildcard() const { return !m_theta.is_initialized(); }

boost::optional<double> LookupRow::thetaOrWildcard() const { return m_theta; }

RangeInQ const &LookupRow::qRange() const { return m_qRange; }

boost::optional<double> LookupRow::scaleFactor() const { return m_scaleFactor; }

boost::optional<ProcessingInstructions> LookupRow::processingInstructions() const { return m_processingInstructions; }

boost::optional<ProcessingInstructions> LookupRow::transmissionProcessingInstructions() const {
  return m_transmissionProcessingInstructions;
}

boost::optional<ProcessingInstructions> LookupRow::backgroundProcessingInstructions() const {
  return m_backgroundProcessingInstructions;
}

bool operator==(LookupRow const &lhs, LookupRow const &rhs) {
  return lhs.thetaOrWildcard() == rhs.thetaOrWildcard() && lhs.qRange() == rhs.qRange() &&
         lhs.scaleFactor() == rhs.scaleFactor() &&
         lhs.transmissionProcessingInstructions() == rhs.transmissionProcessingInstructions() &&
         lhs.processingInstructions() == rhs.processingInstructions() &&
         lhs.backgroundProcessingInstructions() == rhs.backgroundProcessingInstructions();
}

bool operator!=(LookupRow const &lhs, LookupRow const &rhs) { return !(lhs == rhs); }

LookupRow::ValueArray lookupRowToArray(LookupRow const &lookupRow) {
  auto result = LookupRow::ValueArray();
  if (lookupRow.thetaOrWildcard())
    result[0] = std::to_string(*lookupRow.thetaOrWildcard());
  result[1] = lookupRow.transmissionWorkspaceNames().firstRunList();
  result[2] = lookupRow.transmissionWorkspaceNames().secondRunList();
  if (lookupRow.transmissionProcessingInstructions())
    result[3] = *lookupRow.transmissionProcessingInstructions();
  if (lookupRow.qRange().min())
    result[4] = std::to_string(*lookupRow.qRange().min());
  if (lookupRow.qRange().max())
    result[5] = std::to_string(*lookupRow.qRange().max());
  if (lookupRow.qRange().step())
    result[6] = std::to_string(*lookupRow.qRange().step());
  if (lookupRow.scaleFactor())
    result[7] = std::to_string(*lookupRow.scaleFactor());
  if (lookupRow.processingInstructions())
    result[8] = *lookupRow.processingInstructions();
  if (lookupRow.backgroundProcessingInstructions())
    result[9] = *lookupRow.backgroundProcessingInstructions();
  return result;
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
