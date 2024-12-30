// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../ISISReflectometry/Reduction/Experiment.h"
#include "../../ISISReflectometry/Reduction/Instrument.h"
#include "../../ISISReflectometry/Reduction/PreviewRow.h"
#include "../../ISISReflectometry/Reduction/ReductionJobs.h"

#include "Common/DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
namespace ModelCreationHelper {

/* Helper methods to create reduction configuration models for the
 * reflectometry GUI tests */

/* Rows */
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeEmptyRow();
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(std::string const &run, double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeSimpleRow(std::string const &run, double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(std::string const &run, double theta, std::string const &trans1,
                                           std::string const &trans2, std::optional<double> qMin = std::nullopt,
                                           std::optional<double> qMax = std::nullopt,
                                           std::optional<double> qStep = std::nullopt,
                                           boost::optional<double> scale = boost::none,
                                           ReductionOptionsMap const &optionsMap = ReductionOptionsMap());
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRow(std::vector<std::string> const &runs, double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRowWithMainCellsFilled(double theta = 0.5);
MANTIDQT_ISISREFLECTOMETRY_DLL Row makeRowWithOptionsCellFilled(double theta, ReductionOptionsMap options);

/* Groups */
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeEmptyGroup();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithOneRow();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRows();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRowsWithDifferentAngles();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRowsWithNonstandardNames();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRowsWithMixedQResolutions();
MANTIDQT_ISISREFLECTOMETRY_DLL Group makeGroupWithTwoRowsWithOutputQResolutions();

/* Reduction Jobs */
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneEmptyGroupModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoEmptyGroupsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithAnInvalidRowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithARowWithInputQRangeModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithARowWithOutputQRangeModel();
MANTIDQT_ISISREFLECTOMETRY_DLL
ReductionJobs oneGroupWithARowWithInputQRangeModelMixedPrecision();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithAnotherRowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithAnotherRunWithSameAngleModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithTwoRunsInARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithTwoRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs oneGroupWithTwoSimpleRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs anotherGroupWithARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithARowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithTwoRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL
ReductionJobs twoGroupsWithTwoRowsAndOneEmptyGroupModel();
MANTIDQT_ISISREFLECTOMETRY_DLL
ReductionJobs oneGroupWithOneRowAndOneGroupWithOneRowAndOneInvalidRowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithOneRowAndOneInvalidRowModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs twoGroupsWithMixedRowsModel();
MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs emptyReductionJobs();
MANTIDQT_ISISREFLECTOMETRY_DLL
ReductionJobs oneGroupWithTwoRowsWithOutputNamesModel();

/* Experiment */
MANTIDQT_ISISREFLECTOMETRY_DLL LookupRow makeWildcardLookupRow();
MANTIDQT_ISISREFLECTOMETRY_DLL LookupRow makeLookupRow(boost::optional<double> angle,
                                                       boost::optional<boost::regex> titleMatcher = boost::none);
MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable makeEmptyLookupTable();
MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable makeLookupTable();
MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable makeLookupTableWithTwoAngles();
MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable makeLookupTableWithTwoAnglesAndWildcard();
MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable makeLookupTableWithTwoValidDuplicateCriteria();
MANTIDQT_ISISREFLECTOMETRY_DLL std::map<std::string, std::string> makeStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL std::map<std::string, std::string> makeEmptyStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL BackgroundSubtraction makeBackgroundSubtraction();
MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections makePolarizationCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections makeEmptyPolarizationCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL FloodCorrections makeFloodCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL TransmissionStitchOptions makeTransmissionStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeTransmissionRunRange();
MANTIDQT_ISISREFLECTOMETRY_DLL TransmissionStitchOptions makeEmptyTransmissionStitchOptions();
MANTIDQT_ISISREFLECTOMETRY_DLL Experiment makeExperiment();
MANTIDQT_ISISREFLECTOMETRY_DLL Experiment makeEmptyExperiment();
MANTIDQT_ISISREFLECTOMETRY_DLL Experiment makeExperimentWithValidDuplicateCriteria();
MANTIDQT_ISISREFLECTOMETRY_DLL Experiment makeExperimentWithReductionTypeSetForSumInLambda();

/* Instrument */
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeWavelengthRange();
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeMonitorBackgroundRange();
MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda makeMonitorIntegralRange();
MANTIDQT_ISISREFLECTOMETRY_DLL MonitorCorrections makeMonitorCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL DetectorCorrections makeDetectorCorrections();
MANTIDQT_ISISREFLECTOMETRY_DLL Instrument makeInstrument();
MANTIDQT_ISISREFLECTOMETRY_DLL Instrument makeEmptyInstrument();

/* Preview */
MANTIDQT_ISISREFLECTOMETRY_DLL PreviewRow makePreviewRow(const double theta);
MANTIDQT_ISISREFLECTOMETRY_DLL PreviewRow makePreviewRow(std::vector<std::string> const &runNumbers,
                                                         const double theta);
MANTIDQT_ISISREFLECTOMETRY_DLL PreviewRow makePreviewRow(const double theta, const std::string &title);

} // namespace ModelCreationHelper
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
