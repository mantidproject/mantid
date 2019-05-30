// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MODELCREATIONHELPERS_H_
#define MANTID_CUSTOMINTERFACES_MODELCREATIONHELPERS_H_

#include "../../ISISReflectometry/Reduction/Experiment.h"
#include "../../ISISReflectometry/Reduction/Instrument.h"
#include "../../ISISReflectometry/Reduction/ReductionJobs.h"

namespace MantidQt {
namespace CustomInterfaces {

/* Helper methods to create reduction configuration models for the
 * reflectometry GUI tests */

/* Rows */
Row makeEmptyRow();
Row makeRow(double theta = 0.5);
Row makeRow(std::string const &run, double theta = 0.5);
Row makeRow(std::vector<std::string> const &runs, double theta = 0.5);
Row makeRowWithMainCellsFilled(double theta = 0.5);
Row makeRowWithOptionsCellFilled(double theta, ReductionOptionsMap options);

/* Groups */
Group makeEmptyGroup();
Group makeGroupWithOneRow();
Group makeGroupWithTwoRows();
Group makeGroupWithTwoRowsWithNonstandardNames();

/* Reduction Jobs */
ReductionJobs oneEmptyGroupModel();
ReductionJobs twoEmptyGroupsModel();
ReductionJobs oneGroupWithAnInvalidRowModel();
ReductionJobs oneGroupWithARowModel();
ReductionJobs oneGroupWithAnotherRowModel();
ReductionJobs oneGroupWithAnotherRunWithSameAngleModel();
ReductionJobs oneGroupWithTwoRunsInARowModel();
ReductionJobs oneGroupWithTwoRowsModel();
ReductionJobs anotherGroupWithARowModel();
ReductionJobs twoGroupsWithARowModel();
ReductionJobs twoGroupsWithTwoRowsModel();
ReductionJobs twoGroupsWithMixedRowsModel();

/* Experiment */
std::vector<PerThetaDefaults> makePerThetaDefaults();
std::vector<PerThetaDefaults> makePerThetaDefaultsWithTwoAnglesAndWildcard();
std::map<std::string, std::string> makeStitchOptions();
std::map<std::string, std::string> makeEmptyStitchOptions();
PolarizationCorrections makePolarizationCorrections();
PolarizationCorrections makeEmptyPolarizationCorrections();
FloodCorrections makeFloodCorrections();
RangeInLambda makeTransmissionRunRange();
RangeInLambda makeEmptyTransmissionRunRange();
Experiment makeExperiment();
Experiment makeEmptyExperiment();

/* Instrument */
RangeInLambda makeWavelengthRange();
RangeInLambda makeMonitorBackgroundRange();
RangeInLambda makeMonitorIntegralRange();
MonitorCorrections makeMonitorCorrections();
DetectorCorrections makeDetectorCorrections();
Instrument makeInstrument();
Instrument makeEmptyInstrument();
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_MODELCREATIONHELPERS_H_
