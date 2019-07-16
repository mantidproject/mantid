// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
#include "Common/DllConfig.h"
#include "ProcessingInstructions.h"
#include "RangeInQ.h"
#include "TransmissionRunPair.h"
#include <array>
#include <boost/optional.hpp>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {

/** @class PerThetaDefaults

    The PerThetaDefaults model holds information about default experiment
   settings that should be applied during reduction for runs with a specific
   angle, theta. If theta is not set, then the settings will be applied to all
   runs that do not have PerThetaDefaults with a matching theta.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaults {
public:
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
    RUN_SPECTRA = 8
  };

  static auto constexpr OPTIONS_TABLE_COLUMN_COUNT = 9;
  using ValueArray = std::array<std::string, OPTIONS_TABLE_COLUMN_COUNT>;

  PerThetaDefaults(
      boost::optional<double> theta, TransmissionRunPair tranmissionRuns,
      boost::optional<ProcessingInstructions>
          transmissionProcessingInstructions,
      RangeInQ qRange, boost::optional<double> scaleFactor,
      boost::optional<ProcessingInstructions> processingInstructions);

  TransmissionRunPair const &transmissionWorkspaceNames() const;
  bool isWildcard() const;
  boost::optional<double> thetaOrWildcard() const;
  RangeInQ const &qRange() const;
  boost::optional<double> scaleFactor() const;
  boost::optional<ProcessingInstructions>
  transmissionProcessingInstructions() const;
  boost::optional<ProcessingInstructions> processingInstructions() const;

private:
  boost::optional<double> m_theta;
  TransmissionRunPair m_transmissionRuns;
  RangeInQ m_qRange;
  boost::optional<double> m_scaleFactor;
  boost::optional<ProcessingInstructions> m_transmissionProcessingInstructions;
  boost::optional<ProcessingInstructions> m_processingInstructions;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(PerThetaDefaults const &lhs,
                                               PerThetaDefaults const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(PerThetaDefaults const &lhs,
                                               PerThetaDefaults const &rhs);
PerThetaDefaults::ValueArray
perThetaDefaultsToArray(PerThetaDefaults const &perThetaDefaults);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
