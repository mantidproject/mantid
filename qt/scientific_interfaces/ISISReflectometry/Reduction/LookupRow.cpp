// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "LookupRow.h"
#include "GUI/Preview/ROIType.h"

namespace {
constexpr double EPSILON = std::numeric_limits<double>::epsilon();
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupRow::LookupRow(std::optional<double> theta, std::optional<boost::regex> titleMatcher,
                     TransmissionRunPair transmissionRuns,
                     std::optional<ProcessingInstructions> transmissionProcessingInstructions, RangeInQ qRange,
                     std::optional<double> scaleFactor, std::optional<ProcessingInstructions> processingInstructions,
                     std::optional<ProcessingInstructions> backgroundProcessingInstructions,
                     std::optional<ProcessingInstructions> roiDetectorIDs)
    : m_theta(std::move(theta)), m_titleMatcher(std::move(titleMatcher)),
      m_transmissionRuns(std::move(transmissionRuns)), m_qRange(std::move(qRange)),
      m_scaleFactor(std::move(scaleFactor)),
      m_transmissionProcessingInstructions(std::move(transmissionProcessingInstructions)),
      m_processingInstructions(std::move(processingInstructions)),
      m_backgroundProcessingInstructions(std::move(backgroundProcessingInstructions)),
      m_roiDetectorIDs(std::move(roiDetectorIDs)) {}

TransmissionRunPair const &LookupRow::transmissionWorkspaceNames() const { return m_transmissionRuns; }

bool LookupRow::isWildcard() const { return !m_theta.has_value() && !m_titleMatcher.has_value(); }

std::optional<double> LookupRow::thetaOrWildcard() const { return m_theta; }

std::optional<boost::regex> LookupRow::titleMatcher() const { return m_titleMatcher; }

RangeInQ const &LookupRow::qRange() const { return m_qRange; }

std::optional<double> LookupRow::scaleFactor() const { return m_scaleFactor; }

std::optional<ProcessingInstructions> LookupRow::processingInstructions() const { return m_processingInstructions; }

std::optional<ProcessingInstructions> LookupRow::transmissionProcessingInstructions() const {
  return m_transmissionProcessingInstructions;
}

std::optional<ProcessingInstructions> LookupRow::backgroundProcessingInstructions() const {
  return m_backgroundProcessingInstructions;
}

std::optional<ProcessingInstructions> LookupRow::roiDetectorIDs() const { return m_roiDetectorIDs; }

void LookupRow::setRoiDetectorIDs(std::optional<ProcessingInstructions> selectedBanks) {
  m_roiDetectorIDs = std::move(selectedBanks);
}

void LookupRow::setProcessingInstructions(ROIType regionType,
                                          std::optional<ProcessingInstructions> processingInstructions) {
  switch (regionType) {
  case ROIType::Signal:
    m_processingInstructions = std::move(processingInstructions);
    return;
  case ROIType::Background:
    m_backgroundProcessingInstructions = std::move(processingInstructions);
    return;
  case ROIType::Transmission:
    m_transmissionProcessingInstructions = std::move(processingInstructions);
    return;
  }
  throw std::invalid_argument("Unexpected ROIType provided");
}

bool LookupRow::hasEqualThetaAndTitle(LookupRow const &lookupRow, double tolerance) const {
  if (!m_theta.has_value() && !lookupRow.m_theta.has_value()) {
    return m_titleMatcher == lookupRow.m_titleMatcher;
  }
  if (m_theta.has_value() && lookupRow.m_theta.has_value()) {
    return std::abs(*m_theta - *lookupRow.m_theta) <= (tolerance + 2.0 * EPSILON) &&
           m_titleMatcher == lookupRow.m_titleMatcher;
  }
  return false;
}

bool operator==(LookupRow const &lhs, LookupRow const &rhs) {
  return (lhs.m_theta == rhs.m_theta && lhs.m_titleMatcher == rhs.m_titleMatcher &&
          lhs.m_transmissionRuns == rhs.m_transmissionRuns && lhs.m_qRange == rhs.m_qRange &&
          lhs.m_scaleFactor == rhs.m_scaleFactor &&
          lhs.m_transmissionProcessingInstructions == rhs.m_transmissionProcessingInstructions &&
          lhs.m_processingInstructions == rhs.m_processingInstructions &&
          lhs.m_backgroundProcessingInstructions == rhs.m_backgroundProcessingInstructions &&
          lhs.m_roiDetectorIDs == rhs.m_roiDetectorIDs);
}

bool operator!=(LookupRow const &lhs, LookupRow const &rhs) { return !(lhs == rhs); }

LookupRow::ValueArray lookupRowToArray(LookupRow const &lookupRow) {
  auto result = LookupRow::ValueArray();
  if (lookupRow.thetaOrWildcard())
    result[LookupRow::Column::THETA] = std::to_string(*lookupRow.thetaOrWildcard());
  if (lookupRow.titleMatcher())
    result[LookupRow::Column::TITLE] = lookupRow.titleMatcher()->expression();
  result[LookupRow::Column::FIRST_TRANS] = lookupRow.transmissionWorkspaceNames().firstRunList();
  result[LookupRow::Column::SECOND_TRANS] = lookupRow.transmissionWorkspaceNames().secondRunList();
  if (lookupRow.transmissionProcessingInstructions())
    result[LookupRow::Column::TRANS_SPECTRA] = *lookupRow.transmissionProcessingInstructions();
  if (lookupRow.qRange().min())
    result[LookupRow::Column::QMIN] = std::to_string(*lookupRow.qRange().min());
  if (lookupRow.qRange().max())
    result[LookupRow::Column::QMAX] = std::to_string(*lookupRow.qRange().max());
  if (lookupRow.qRange().step())
    result[LookupRow::Column::QSTEP] = std::to_string(*lookupRow.qRange().step());
  if (lookupRow.scaleFactor())
    result[LookupRow::Column::SCALE] = std::to_string(*lookupRow.scaleFactor());
  if (lookupRow.processingInstructions())
    result[LookupRow::Column::RUN_SPECTRA] = *lookupRow.processingInstructions();
  if (lookupRow.backgroundProcessingInstructions())
    result[LookupRow::Column::BACKGROUND_SPECTRA] = *lookupRow.backgroundProcessingInstructions();
  if (lookupRow.roiDetectorIDs())
    result[LookupRow::Column::ROI_DETECTOR_IDS] = *lookupRow.roiDetectorIDs();
  return result;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
