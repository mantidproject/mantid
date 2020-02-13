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
namespace ISISReflectometry {

/** @class PerThetaDefaults

    The PerThetaDefaults model holds information about default experiment
   settings that should be applied during reduction for runs with a specific
   angle, theta. If theta is not set, then the settings will be applied to all
   runs that do not have PerThetaDefaults with a matching theta.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaults {
public:
  static auto constexpr OPTIONS_TABLE_COLUMN_COUNT = 9;
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
    RUN_SPECTRA = 8
  };

  // The property name associated with each column. Unfortunately in
  // QtBatchView we are still using ReflectometryReductionOneAuto to get the
  // tooltips rather than ReflectometryISISLoadAndProcess. If we change it we
  // will break the Encoder and Decoder unit tests because C++ tests cannot use
  // python algorithms. The code needs to be reorganised to avoid creating the
  // algorithm, or the tests rewritten in python. This only affects tooltips
  // for a couple of the properties so is not an urgent fix, so for now just
  // make the properties here match RROA.
  static auto constexpr ColumnPropertyName =
      std::array<const char *, OPTIONS_TABLE_COLUMN_COUNT>{
          "ThetaIn",
          "FirstTransmissionRun",  // FirstTransmissionRunList in RILAP
          "SecondTransmissionRun", // SecondTransmissionRunList in RILAP
          "TransmissionProcessingInstructions",
          "MomentumTransferMin",
          "MomentumTransferMax",
          "MomentumTransferStep",
          "ScaleFactor",
          "ProcessingInstructions"};

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
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
