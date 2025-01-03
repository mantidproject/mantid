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
#include <boost/optional.hpp>
#include <boost/regex.hpp>
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

  LookupRow(boost::optional<double> theta, std::optional<boost::regex> titleMatcher,
            TransmissionRunPair tranmissionRuns,
            boost::optional<ProcessingInstructions> transmissionProcessingInstructions, RangeInQ qRange,
            boost::optional<double> scaleFactor, boost::optional<ProcessingInstructions> processingInstructions,
            boost::optional<ProcessingInstructions> backgroundProcessingInstructions,
            boost::optional<ProcessingInstructions> roiDetectorIDs);

  TransmissionRunPair const &transmissionWorkspaceNames() const;
  bool isWildcard() const;
  boost::optional<double> thetaOrWildcard() const;
  std::optional<boost::regex> titleMatcher() const;
  RangeInQ const &qRange() const;
  boost::optional<double> scaleFactor() const;
  boost::optional<ProcessingInstructions> transmissionProcessingInstructions() const;
  boost::optional<ProcessingInstructions> processingInstructions() const;
  boost::optional<ProcessingInstructions> backgroundProcessingInstructions() const;
  boost::optional<ProcessingInstructions> roiDetectorIDs() const;
  void setRoiDetectorIDs(boost::optional<ProcessingInstructions> selectedBanks);
  void setProcessingInstructions(ROIType regionType, boost::optional<ProcessingInstructions> processingInstructions);
  bool hasEqualThetaAndTitle(LookupRow const &lookupRow, double tolerance) const;

  MANTIDQT_ISISREFLECTOMETRY_DLL friend bool operator==(LookupRow const &lhs, LookupRow const &rhs);
  MANTIDQT_ISISREFLECTOMETRY_DLL friend bool operator!=(LookupRow const &lhs, LookupRow const &rhs);

private:
  boost::optional<double> m_theta;
  std::optional<boost::regex> m_titleMatcher;
  TransmissionRunPair m_transmissionRuns;
  RangeInQ m_qRange;
  boost::optional<double> m_scaleFactor;
  boost::optional<ProcessingInstructions> m_transmissionProcessingInstructions;
  boost::optional<ProcessingInstructions> m_processingInstructions;
  boost::optional<ProcessingInstructions> m_backgroundProcessingInstructions;
  boost::optional<ProcessingInstructions> m_roiDetectorIDs;
};

LookupRow::ValueArray lookupRowToArray(LookupRow const &lookupRow);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
