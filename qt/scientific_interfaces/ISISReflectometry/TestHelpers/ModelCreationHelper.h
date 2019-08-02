// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MODELCREATIONHELPER_H_
#define MANTID_CUSTOMINTERFACES_MODELCREATIONHELPER_H_

#include "../../ISISReflectometry/Reduction/Experiment.h"
#include "../../ISISReflectometry/Reduction/Instrument.h"
#include "../../ISISReflectometry/Reduction/ReductionJobs.h"
#include "Common/DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ModelCreationHelper {

/* Helper methods to create reduction configuration models for the
 * reflectometry GUI tests */

/* Rows */
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeEmptyRow();
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(std::string const &run,
                                           double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(std::vector<std::string> const &runs,
                                           double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row
makeRowWithMainCellsFilled(double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row
makeRowWithOptionsCellFilled(double theta, ReductionOptionsMap options);

/* Groups */
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeEmptyGroup();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithOneRow();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRows();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRowsWithNonstandardNames();
MANTIDQT_ISISREFLECTOMETRY_DLL Group
makeGroupWithTwoRowsWithMixedQResolutions();
MANTIDQT_ISISREFLECTOMETRY_DLL Group
makeGroupWithTwoRowsWithOutputQResolutions();

/* Reduction Jobs */
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneEmptyGroupModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoEmptyGroupsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithAnInvalidRowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs
oneGroupWithARowWithInputQRangeModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs
oneGroupWithARowWithOutputQRangeModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithAnotherRowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs
oneGroupWithAnotherRunWithSameAngleModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithTwoRunsInARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithTwoRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs anotherGroupWithARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithTwoRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithMixedRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs emptyReductionJobs();

/* Experiment */
MANTIDQT_ISISREFLECTOMETRY_DLL std::vector<PerThetaDefaults>
makePerThetaDefaults();
MANTIDQT_ISISREFLECTOMETRY_DLL std::vector<PerThetaDefaults>
makePerThetaDefaultsWithTwoAnglesAndWildcard();
MANTIDQT_ISISREFLECTOMETRY_DLL std::map<std::string, std::string>
makeStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL std::map<std::string, std::string>
makeEmptyStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections
makePolarizationCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections
makeEmptyPolarizationCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL FloodCorrections makeFloodCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeTransmissionRunRange();
MANTIDQT_ISISREFLECTOMETRY_DLL TransmissionStitchOptions
makeEmptyTransmissionStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL Experiment makeExperiment();
MANTIDQT_ISISREFLECTOMETRY_DLL Experiment makeEmptyExperiment();

/* Instrument */
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeWavelengthRange();
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeMonitorBackgroundRange();
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeMonitorIntegralRange();
MANTIDQT_ISISREFLECTOMETRY_DLL MonitorCorrections makeMonitorCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL DetectorCorrections makeDetectorCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL Instrument makeInstrument();
MANTIDQT_ISISREFLECTOMETRY_DLL Instrument makeEmptyInstrument();
} // namespace ModelCreationHelper
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_MODELCREATIONHELPER_H_
