# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from sans.common.enums import ReductionMode, DetectorType, RangeStepType, FitType
from sans.test_helper.user_file_test_helper import create_user_file, sample_user_file
from sans.user_file.settings_tags import (
    DetectorId,
    BackId,
    range_entry,
    back_single_monitor_entry,
    single_entry_with_detector,
    LimitsId,
    simple_range,
    complex_range,
    MaskId,
    range_entry_with_detector,
    SampleId,
    SetId,
    set_scales_entry,
    position_entry,
    TransId,
    TubeCalibrationFileId,
    QResolutionId,
    FitId,
    fit_general,
    MonId,
    monitor_file,
    GravityId,
    monitor_spectrum,
    PrintId,
    q_rebin_values,
)
from sans.user_file.user_file_reader import UserFileReader


# -----------------------------------------------------------------
# --- Tests -------------------------------------------------------
# -----------------------------------------------------------------
class UserFileReaderTest(unittest.TestCase):
    def test_that_can_read_user_file(self):
        # Arrange
        user_file_path = create_user_file(sample_user_file)
        reader = UserFileReader(user_file_path)

        # Act
        output = reader.read_user_file()

        # Assert
        expected_values = {
            LimitsId.WAVELENGTH: [simple_range(start=1.5, stop=12.5, step=0.125, step_type=RangeStepType.LIN)],
            LimitsId.Q: [q_rebin_values(min=0.001, max=0.2, rebin_string="0.001,0.001,0.0126,-0.08,0.2")],
            LimitsId.QXY: [simple_range(0, 0.05, 0.001, RangeStepType.LIN)],
            BackId.SINGLE_MONITORS: [back_single_monitor_entry(1, 35000, 65000), back_single_monitor_entry(2, 85000, 98000)],
            DetectorId.REDUCTION_MODE: [ReductionMode.LAB],
            GravityId.ON_OFF: [True],
            FitId.GENERAL: [fit_general(start=1.5, stop=12.5, fit_type=FitType.LOGARITHMIC, data_type=None, polynomial_order=0)],
            MaskId.VERTICAL_SINGLE_STRIP_MASK: [
                single_entry_with_detector(191, DetectorType.LAB),
                single_entry_with_detector(191, DetectorType.HAB),
                single_entry_with_detector(0, DetectorType.LAB),
                single_entry_with_detector(0, DetectorType.HAB),
            ],
            MaskId.HORIZONTAL_SINGLE_STRIP_MASK: [
                single_entry_with_detector(0, DetectorType.LAB),
                single_entry_with_detector(0, DetectorType.HAB),
            ],
            MaskId.HORIZONTAL_RANGE_STRIP_MASK: [
                range_entry_with_detector(190, 191, DetectorType.LAB),
                range_entry_with_detector(167, 172, DetectorType.LAB),
                range_entry_with_detector(190, 191, DetectorType.HAB),
                range_entry_with_detector(156, 159, DetectorType.HAB),
            ],
            MaskId.TIME: [range_entry_with_detector(17500, 22000, None)],
            MonId.DIRECT: [
                monitor_file("DIRECTM1_15785_12m_31Oct12_v12.dat", DetectorType.LAB),
                monitor_file("DIRECTM1_15785_12m_31Oct12_v12.dat", DetectorType.HAB),
            ],
            MonId.SPECTRUM: [monitor_spectrum(1, True, True), monitor_spectrum(1, False, True)],
            SetId.CENTRE: [position_entry(155.45, -169.6, DetectorType.LAB)],
            SetId.SCALES: [set_scales_entry(0.074, 1.0, 1.0, 1.0, 1.0)],
            SampleId.OFFSET: [53.0],
            DetectorId.CORRECTION_X: [
                single_entry_with_detector(-16.0, DetectorType.LAB),
                single_entry_with_detector(-44.0, DetectorType.HAB),
            ],
            DetectorId.CORRECTION_Y: [single_entry_with_detector(-20.0, DetectorType.HAB)],
            DetectorId.CORRECTION_Z: [
                single_entry_with_detector(47.0, DetectorType.LAB),
                single_entry_with_detector(47.0, DetectorType.HAB),
            ],
            DetectorId.CORRECTION_ROTATION: [single_entry_with_detector(0.0, DetectorType.HAB)],
            LimitsId.EVENTS_BINNING: ["7000.0,500.0,60000.0"],
            MaskId.CLEAR_DETECTOR_MASK: [True],
            MaskId.CLEAR_TIME_MASK: [True],
            LimitsId.RADIUS: [range_entry(12, 15)],
            TransId.SPEC_4_SHIFT: [-70.0],
            PrintId.PRINT_LINE: ["for changer"],
            BackId.ALL_MONITORS: [range_entry(start=3500, stop=4500)],
            FitId.MONITOR_TIMES: [range_entry(start=1000, stop=2000)],
            TransId.SPEC: [4],
            BackId.TRANS: [range_entry(start=123, stop=466)],
            TransId.RADIUS: [7.0],
            TransId.ROI: ["test.xml", "test2.xml"],
            TransId.MASK: ["test3.xml", "test4.xml"],
            SampleId.PATH: [True],
            LimitsId.RADIUS_CUT: [200.0],
            LimitsId.WAVELENGTH_CUT: [8.0],
            QResolutionId.ON: [True],
            QResolutionId.DELTA_R: [11.0],
            QResolutionId.COLLIMATION_LENGTH: [12.0],
            QResolutionId.A1: [13.0],
            QResolutionId.A2: [14.0],
            QResolutionId.MODERATOR: ["moderator_rkh_file.txt"],
            TubeCalibrationFileId.FILE: ["TUBE_SANS2D_BOTH_31681_25Sept15.nxs"],
        }

        self.assertEqual(len(expected_values), len(output))
        for key, value in list(expected_values.items()):
            self.assertTrue(key in output)
            self.assertEqual(len(output[key]), len(value))
            elements = output[key]
            # Make sure that the different entries are sorted
            UserFileReaderTest._sort_list(elements)
            UserFileReaderTest._sort_list(value)
            self.assertEqual(elements, value, "{} is not {}".format(elements, value))

        # clean up
        if os.path.exists(user_file_path):
            os.remove(user_file_path)

    @staticmethod
    def _sort_list(elements):
        if len(elements) == 1:
            return

        if isinstance(elements[0], single_entry_with_detector):
            UserFileReaderTest._sort(elements, lambda x: x.entry)
        elif isinstance(elements[0], simple_range):
            UserFileReaderTest._sort(elements, lambda x: x.start)
        elif isinstance(elements[0], complex_range):
            UserFileReaderTest._sort(elements, lambda x: x.start)
        elif isinstance(elements[0], back_single_monitor_entry):
            UserFileReaderTest._sort(elements, lambda x: x.monitor)
        elif isinstance(elements[0], fit_general):
            UserFileReaderTest._sort(elements, lambda x: x.start)
        elif isinstance(elements[0], range_entry_with_detector):
            UserFileReaderTest._sort(elements, lambda x: x.start)
        elif isinstance(elements[0], monitor_file):
            UserFileReaderTest._sort(elements, lambda x: (x.file_path, x.detector_type.value))
        elif isinstance(elements[0], monitor_spectrum):
            UserFileReaderTest._sort(elements, lambda x: x.spectrum)
        elif isinstance(elements[0], position_entry):
            UserFileReaderTest._sort(elements, lambda x: x.pos1)
        elif isinstance(elements[0], set_scales_entry):
            UserFileReaderTest._sort(elements, lambda x: x.s)
        elif isinstance(elements[0], range_entry):
            UserFileReaderTest._sort(elements, lambda x: x.start)
        else:
            elements.sort()

    @staticmethod
    def _sort(elements, predicate):
        elements.sort(key=predicate)


if __name__ == "__main__":
    unittest.main()
