// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "GUI/Preview/ROIType.h"
#include "ProcessingInstructions.h"
#include "RangeInQ.h"
#include "TransmissionRunPair.h"
#include <array>
#include <boost/regex.hpp>
#include <optional>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class LookupRow

    The LookupRow model holds information about default experiment
   settings that should be applied during reduction for runs with a specific
   angle, theta. If theta is not set, then the settings will be applied to all
   runs that do not have LookupRow with a matching theta.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL LookupRow {
public:
  static auto constexpr OPTIONS_TABLE_COLUMN_COUNT = 12;
  using ValueArray = std::array<std::string, OPTIONS_TABLE_COLUMN_COUNT>;

  enum Column {
    // 0-based column indices for cells in a row. The Actual values are
    // important here so set them explicitly
    THETA = 0,
    TITLE = 1,
    FIRST_TRANS = 2,
    SECOND_TRANS = 3,
    TRANS_SPECTRA = 4,
    QMIN = 5,
    QMAX = 6,
    QSTEP = 7,
    SCALE = 8,
    RUN_SPECTRA = 9,
    BACKGROUND_SPECTRA = 10,
    ROI_DETECTOR_IDS = 11
  };

  LookupRow(std::optional<double> theta, std::optional<boost::regex> titleMatcher, TransmissionRunPair tranmissionRuns,
            std::optional<ProcessingInstructions> transmissionProcessingInstructions, RangeInQ qRange,
            std::optional<double> scaleFactor, std::optional<ProcessingInstructions> processingInstructions,
            std::optional<ProcessingInstructions> backgroundProcessingInstructions,
            std::optional<ProcessingInstructions> roiDetectorIDs);

  TransmissionRunPair const &transmissionWorkspaceNames() const;
  bool isWildcard() const;
  std::optional<double> thetaOrWildcard() const;
  std::optional<boost::regex> titleMatcher() const;
  RangeInQ const &qRange() const;
  std::optional<double> scaleFactor() const;
  std::optional<ProcessingInstructions> transmissionProcessingInstructions() const;
  std::optional<ProcessingInstructions> processingInstructions() const;
  std::optional<ProcessingInstructions> backgroundProcessingInstructions() const;
  std::optional<ProcessingInstructions> roiDetectorIDs() const;
  void setRoiDetectorIDs(std::optional<ProcessingInstructions> selectedBanks);
  void setProcessingInstructions(ROIType regionType, std::optional<ProcessingInstructions> processingInstructions);
  bool hasEqualThetaAndTitle(LookupRow const &lookupRow, double tolerance) const;

  MANTIDQT_ISISREFLECTOMETRY_DLL friend bool operator==(LookupRow const &lhs, LookupRow const &rhs);
  MANTIDQT_ISISREFLECTOMETRY_DLL friend bool operator!=(LookupRow const &lhs, LookupRow const &rhs);

private:
  std::optional<double> m_theta;
  std::optional<boost::regex> m_titleMatcher;
  TransmissionRunPair m_transmissionRuns;
  RangeInQ m_qRange;
  std::optional<double> m_scaleFactor;
  std::optional<ProcessingInstructions> m_transmissionProcessingInstructions;
  std::optional<ProcessingInstructions> m_processingInstructions;
  std::optional<ProcessingInstructions> m_backgroundProcessingInstructions;
  std::optional<ProcessingInstructions> m_roiDetectorIDs;
};

LookupRow::ValueArray lookupRowToArray(LookupRow const &lookupRow);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
