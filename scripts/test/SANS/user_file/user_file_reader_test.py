# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid
import os
from sans.common.enums import (ISISReductionMode, DetectorType, RangeStepType, FitType)
from sans.user_file.user_file_reader import UserFileReader
from sans.user_file.settings_tags import (DetectorId, BackId, range_entry, back_single_monitor_entry,
                                          single_entry_with_detector, mask_angle_entry, LimitsId, rebin_string_values,
                                          simple_range, complex_range, MaskId, mask_block, mask_block_cross,
                                          mask_line, range_entry_with_detector, SampleId, SetId, set_scales_entry,
                                          position_entry, TransId, TubeCalibrationFileId, QResolutionId, FitId,
                                          fit_general, MonId, monitor_length, monitor_file, GravityId,
                                          monitor_spectrum, PrintId, q_rebin_values)
from sans.test_helper.user_file_test_helper import create_user_file, sample_user_file


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
        expected_values = {LimitsId.wavelength: [simple_range(start=1.5, stop=12.5, step=0.125,
                                                              step_type=RangeStepType.Lin)],
                           LimitsId.q: [q_rebin_values(min=.001, max=.2, rebin_string="0.001,0.001,0.0126,-0.08,0.2")],
                           LimitsId.qxy: [simple_range(0, 0.05, 0.001, RangeStepType.Lin)],
                           BackId.single_monitors: [back_single_monitor_entry(1, 35000, 65000),
                                                    back_single_monitor_entry(2, 85000, 98000)],
                           DetectorId.reduction_mode: [ISISReductionMode.LAB],
                           GravityId.on_off: [True],
                           FitId.general: [fit_general(start=1.5, stop=12.5, fit_type=FitType.Logarithmic,
                                                       data_type=None, polynomial_order=0)],
                           MaskId.vertical_single_strip_mask: [single_entry_with_detector(191, DetectorType.LAB),
                                                               single_entry_with_detector(191, DetectorType.HAB),
                                                               single_entry_with_detector(0, DetectorType.LAB),
                                                               single_entry_with_detector(0, DetectorType.HAB)],
                           MaskId.horizontal_single_strip_mask: [single_entry_with_detector(0, DetectorType.LAB),
                                                                 single_entry_with_detector(0, DetectorType.HAB)],
                           MaskId.horizontal_range_strip_mask: [range_entry_with_detector(190, 191, DetectorType.LAB),
                                                                range_entry_with_detector(167, 172, DetectorType.LAB),
                                                                range_entry_with_detector(190, 191, DetectorType.HAB),
                                                                range_entry_with_detector(156, 159, DetectorType.HAB)
                                                                ],
                           MaskId.time: [range_entry_with_detector(17500, 22000, None)],
                           MonId.direct: [monitor_file("DIRECTM1_15785_12m_31Oct12_v12.dat", DetectorType.LAB),
                                          monitor_file("DIRECTM1_15785_12m_31Oct12_v12.dat", DetectorType.HAB)],
                           MonId.spectrum: [monitor_spectrum(1, True, True), monitor_spectrum(1, False, True)],
                           SetId.centre: [position_entry(155.45, -169.6, DetectorType.LAB)],
                           SetId.scales: [set_scales_entry(0.074, 1.0, 1.0, 1.0, 1.0)],
                           SampleId.offset: [53.0],
                           DetectorId.correction_x: [single_entry_with_detector(-16.0, DetectorType.LAB),
                                                     single_entry_with_detector(-44.0, DetectorType.HAB)],
                           DetectorId.correction_y: [single_entry_with_detector(-20.0, DetectorType.HAB)],
                           DetectorId.correction_z: [single_entry_with_detector(47.0, DetectorType.LAB),
                                                     single_entry_with_detector(47.0, DetectorType.HAB)],
                           DetectorId.correction_rotation: [single_entry_with_detector(0.0, DetectorType.HAB)],
                           LimitsId.events_binning: ["7000.0,500.0,60000.0"],
                           MaskId.clear_detector_mask: [True],
                           MaskId.clear_time_mask: [True],
                           LimitsId.radius: [range_entry(12, 15)],
                           TransId.spec_shift: [-70.],
                           PrintId.print_line: ["for changer"],
                           BackId.all_monitors: [range_entry(start=3500, stop=4500)],
                           FitId.monitor_times: [range_entry(start=1000, stop=2000)],
                           TransId.spec: [4],
                           BackId.trans: [range_entry(start=123, stop=466)],
                           TransId.radius: [7.0],
                           TransId.roi: ["test.xml", "test2.xml"],
                           TransId.mask: ["test3.xml", "test4.xml"],
                           SampleId.path: [True],
                           LimitsId.radius_cut: [200.0],
                           LimitsId.wavelength_cut: [8.0],
                           QResolutionId.on: [True],
                           QResolutionId.delta_r: [11.],
                           QResolutionId.collimation_length: [12.],
                           QResolutionId.a1: [13.],
                           QResolutionId.a2: [14.],
                           QResolutionId.moderator: ["moderator_rkh_file.txt"],
                           TubeCalibrationFileId.file: ["TUBE_SANS2D_BOTH_31681_25Sept15.nxs"]}

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
            UserFileReaderTest._sort(elements, lambda x: (x.file_path, DetectorType.to_string(x.detector_type)))
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
