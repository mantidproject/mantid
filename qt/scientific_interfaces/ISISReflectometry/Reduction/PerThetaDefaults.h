// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "ProcessingInstructions.h"
#include "RangeInQ.h"
#include "TransmissionRunPair.h"
#include <array>
#include <boost/optional.hpp>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class PerThetaDefaults

    The PerThetaDefaults model holds information about default experiment
   settings that should be applied during reduction for runs with a specific
   angle, theta. If theta is not set, then the settings will be applied to all
   runs that do not have PerThetaDefaults with a matching theta.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaults {
public:
  static auto constexpr OPTIONS_TABLE_COLUMN_COUNT = 10;
  using ValueArray = std::array<std::string, OPTIONS_TABLE_COLUMN_COUNT>;

  enum Column {
    // 0-based column indices for cells in a row. The Actual values are
    // important here so set them explicitly
    THETA = 0,
    FIRST_TRANS = 1,
    SECOND_TRANS = 2,
    TRANS_SPECTRA = 3,
    QMIN = 4,
    QMAX = 5,
    QSTEP = 6,
    SCALE = 7,
    RUN_SPECTRA = 8,
    BACKGROUND_SPECTRA = 9
  };

  static auto constexpr ColumnPropertyName =
      std::array<const char *, OPTIONS_TABLE_COLUMN_COUNT>{"ThetaIn",
                                                           "FirstTransmissionRunList",
                                                           "SecondTransmissionRunList",
                                                           "TransmissionProcessingInstructions",
                                                           "MomentumTransferMin",
                                                           "MomentumTransferMax",
                                                           "MomentumTransferStep",
                                                           "ScaleFactor",
                                                           "ProcessingInstructions",
                                                           "BackgroundProcessingInstructions"};

  PerThetaDefaults(boost::optional<double> theta, TransmissionRunPair tranmissionRuns,
                   boost::optional<ProcessingInstructions> transmissionProcessingInstructions, RangeInQ qRange,
                   boost::optional<double> scaleFactor, boost::optional<ProcessingInstructions> processingInstructions,
                   boost::optional<ProcessingInstructions> backgroundProcessingInstructions);

  TransmissionRunPair const &transmissionWorkspaceNames() const;
  bool isWildcard() const;
  boost::optional<double> thetaOrWildcard() const;
  RangeInQ const &qRange() const;
  boost::optional<double> scaleFactor() const;
  boost::optional<ProcessingInstructions> transmissionProcessingInstructions() const;
  boost::optional<ProcessingInstructions> processingInstructions() const;
  boost::optional<ProcessingInstructions> backgroundProcessingInstructions() const;

private:
  boost::optional<double> m_theta;
  TransmissionRunPair m_transmissionRuns;
  RangeInQ m_qRange;
  boost::optional<double> m_scaleFactor;
  boost::optional<ProcessingInstructions> m_transmissionProcessingInstructions;
  boost::optional<ProcessingInstructions> m_processingInstructions;
  boost::optional<ProcessingInstructions> m_backgroundProcessingInstructions;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs);
PerThetaDefaults::ValueArray perThetaDefaultsToArray(PerThetaDefaults const &perThetaDefaults);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt