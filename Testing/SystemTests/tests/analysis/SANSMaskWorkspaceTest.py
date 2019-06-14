# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import systemtesting
import os

import mantid
from mantid.api import AlgorithmManager

from sans.common.enums import SANSFacility
from sans.test_helper.test_director import TestDirector
from sans.state.data import get_data_builder
from sans.state.mask import get_mask_builder
from sans.state.move import get_move_builder
from sans.common.file_information import SANSFileInformationFactory


def get_masked_spectrum_numbers(workspace):
    for index in range(workspace.getNumberHistograms()):
        try:
            det = workspace.getDetector(index)
        except RuntimeError:
            break
        if det.isMasked():
            yield workspace.getSpectrum(index).getSpectrumNo()


def get_non_masked_spectrum_numbers(workspace):
    for index in range(workspace.getNumberHistograms()):
        try:
            det = workspace.getDetector(index)
        except RuntimeError:
            break
        if not det.isMasked():
            yield workspace.getSpectrum(index).getSpectrumNo()


def elements_in_range(range_start, range_stop, collection):
    for element in collection:
        if range_start <= element <= range_stop:
            yield element


# -----------------------------------------------
# Tests for the SANSLoad algorithm
# -----------------------------------------------
class SANSMaskWorkspaceTest(unittest.TestCase):
    def _load_workspace(self, state):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.initialize()

        state_dict = state.property_manager
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", False)
        load_alg.setProperty("UseCached", False)
        load_alg.setProperty("SampleScatterWorkspace", "dummy")
        load_alg.setProperty("SampleScatterMonitorWorkspace", "dummy")

        # Act
        load_alg.execute()
        self.assertTrue(load_alg.isExecuted())
        return load_alg.getProperty("SampleScatterWorkspace").value

    def _run_mask(self, state, workspace, component):
        mask_alg = AlgorithmManager.createUnmanaged("SANSMaskWorkspace")
        mask_alg.setChild(True)
        mask_alg.initialize()

        state_dict = state.property_manager
        mask_alg.setProperty("SANSState", state_dict)
        mask_alg.setProperty("Workspace", workspace)
        mask_alg.setProperty("Component", component)

        mask_alg.execute()
        self.assertTrue(mask_alg.isExecuted())
        return mask_alg.getProperty("Workspace").value

    def _do_assert(self, workspace, expected_spectra):
        # Remove duplicate masks from expected
        expected_spectra = list(set(expected_spectra))
        masked_spectra = list(get_masked_spectrum_numbers(workspace))

        self.assertEqual(len(expected_spectra), len(masked_spectra),
                         "{} does not equal {}".format(len(expected_spectra), len(masked_spectra)))
        for expected, actual in zip(sorted(expected_spectra), sorted(masked_spectra)):
            self.assertEqual(expected,  actual, "{} does not equal {}".format(expected, actual))

    def _do_assert_non_masked(self, workspace, expected_spectra):
        # Remove duplicate masks from expected
        expected_spectra = list(set(expected_spectra))

        non_masked_spectra = list(get_non_masked_spectrum_numbers(workspace))
        self.assertEqual(len(expected_spectra), len(non_masked_spectra),
                         "Expected length {}, got length {}".format(len(expected_spectra), len(non_masked_spectra)))
        for expected, actual in zip(sorted(expected_spectra), sorted(non_masked_spectra)):
            self.assertEqual(expected,  actual)

    def test_that_spectra_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()

        mask_builder = get_mask_builder(data_info)

        # Expected_spectra
        expected_spectra = []

        # Standard spectra
        single_spectra = [13, 14, 17]
        expected_spectra.extend(single_spectra)

        spectrum_range_start = [20, 30]
        spectrum_range_stop = [25, 35]
        expected_spectra.extend(list(range(20, 25 + 1)))
        expected_spectra.extend(list(range(30, 35 + 1)))

        # Detector-specific single horizontal strip mask
        # The horizontal strip will be evaluated for SANS2D on the LAB as:
        # e.g. [(50*512 + 9(monitors)] + x in range(0, 512)
        single_horizontal_strip_masks = [50, 53]
        for single_horizontal_strip_mask in single_horizontal_strip_masks:
            expected_spectra.extend(((single_horizontal_strip_mask*512 + 9) + x for x in range(0, 512)))

        # Detector-specific range horizontal strip mask
        # The horizontal range will be evaluated for SANS2D on the LAB as:
        # e.g. [(62*512 + 9(monitors)] + x in range(0, 512) + (63*512 + 9(monitors)] + x in range(0, 512) + ...]
        range_horizontal_strip_start = [62, 67]
        range_horizontal_strip_stop = [64, 70]
        for start, stop in zip(range_horizontal_strip_start, range_horizontal_strip_stop):
            expected_spectra.extend(((start*512 + 9) + y*512 + x for y in range(0, stop - start + 1)
                                     for x in range(0, 512)))

        # Detector-specific single vertical strip mask
        # The vertical strip will be evaluated for SANS2D on the LAB as:
        # e.g. [(45 + 9(monitors)] + y*512  for y in range(0, 120)]
        single_vertical_strip_masks = [45, 89]
        for single_vertical_strip_mask in single_vertical_strip_masks:
            expected_spectra.extend(((single_vertical_strip_mask + 9) + y*512 for y in range(0, 120)))

        # Detector-specific range vertical strip mask
        # The vertical range will be evaluated for SANS2D on the LAB as:
        range_vertical_strip_start = [99]
        range_vertical_strip_stop = [102]
        for start, stop in zip(range_vertical_strip_start, range_vertical_strip_stop):
            expected_spectra.extend(((start_elem + 9) + y * 512 for start_elem in range(start, stop + 1)
                                     for y in range(0, 120)))

        mask_builder.set_single_spectra_on_detector(single_spectra)
        mask_builder.set_spectrum_range_on_detector(spectrum_range_start, spectrum_range_stop)
        mask_builder.set_LAB_single_horizontal_strip_mask(single_horizontal_strip_masks)
        mask_builder.set_LAB_range_horizontal_strip_start(range_horizontal_strip_start)
        mask_builder.set_LAB_range_horizontal_strip_stop(range_horizontal_strip_stop)
        mask_builder.set_LAB_single_vertical_strip_mask(single_vertical_strip_masks)
        mask_builder.set_LAB_range_vertical_strip_start(range_vertical_strip_start)
        mask_builder.set_LAB_range_vertical_strip_stop(range_vertical_strip_stop)
        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)

    def test_that_block_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()

        mask_builder = get_mask_builder(data_info)

        # Expected_spectra
        expected_spectra = []

        # Block
        # Detector-specific block
        # The block will be evaluated for SANS2D on the LAB as:
        block_horizontal_start = [12, 17]
        block_horizontal_stop = [14, 21]
        block_vertical_start = [45, 87]
        block_vertical_stop = [48, 91]

        for h_start, h_stop, v_start, v_stop in zip(block_horizontal_start, block_horizontal_stop,
                                                    block_vertical_start, block_vertical_stop):
            expected_spectra.extend(((h_start*512 + 9) + y*512 + x for y in range(0, h_stop - h_start + 1)
                                     for x in range(v_start, v_stop + 1)))

        mask_builder.set_LAB_block_horizontal_start(block_horizontal_start)
        mask_builder.set_LAB_block_horizontal_stop(block_horizontal_stop)
        mask_builder.set_LAB_block_vertical_start(block_vertical_start)
        mask_builder.set_LAB_block_vertical_stop(block_vertical_stop)
        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)

    def test_that_cross_block_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()

        mask_builder = get_mask_builder(data_info)

        # Expected_spectra
        expected_spectra = []

        # Block
        # Detector-specific cross block
        # The block will be evaluated for SANS2D on the LAB as:
        block_cross_horizontal = [12, 17]
        block_cross_vertical = [49, 67]
        for h, v in zip(block_cross_horizontal, block_cross_vertical):
            expected_spectra.extend([h*512 + 9 + v])

        mask_builder.set_LAB_block_cross_horizontal(block_cross_horizontal)
        mask_builder.set_LAB_block_cross_vertical(block_cross_vertical)
        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)

    def test_that_mask_files_are_applied(self):
        def create_shape_xml_file(xml_string):
            f_name = os.path.join(mantid.config.getString('defaultsave.directory'), 'sample_mask_file.xml')
            if os.path.exists(f_name):
                os.remove(f_name)
            with open(f_name, 'w') as f:
                f.write(xml_string)
            return f_name

        # Arrange
        shape_xml = "<?xml version=\"1.0\"?>\n"\
                    "<detector-masking>\n" \
                    "<group>\n" \
                    "<detids>\n" \
                    "1313191-1313256\n" \
                    "</detids>\n" \
                    "</group>\n" \
                    "</detector-masking >"
        file_name = create_shape_xml_file(shape_xml)

        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()
        mask_builder = get_mask_builder(data_info)

        # Mask file
        # Got the spectra from the detector view
        expected_spectra = [x for x in range(31432, 31498)]
        mask_builder.set_mask_files([file_name])
        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)

        # Remove
        if os.path.exists(file_name):
            os.remove(file_name)

    def test_that_general_time_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()
        mask_builder = get_mask_builder(data_info)

        # Expected_spectra
        bin_mask_general_start = [30000., 67000.]
        bin_mask_general_stop = [35000., 75000.]

        # bin_mask_start = [14000]
        # bin_mask_stop = FloatListParameter()

        mask_builder.set_bin_mask_general_start(bin_mask_general_start)
        mask_builder.set_bin_mask_general_stop(bin_mask_general_stop)

        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        tof_spectra_10_original = workspace.getSpectrum(10).getTofs()
        tof_spectra_11_original = workspace.getSpectrum(11).getTofs()

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        # Confirm that everything in the ranges 30000-35000  and 67000-75000 is removed from the event list
        tof_spectra_10_masked = workspace.getSpectrum(10).getTofs()
        tof_spectra_11_masked = workspace.getSpectrum(11).getTofs()
        # Spectrum 10
        # Three events should have been removed
        self.assertEqual(len(tof_spectra_10_masked),  len(tof_spectra_10_original) - 3)
        # One event should have been removed
        self.assertEqual(len(tof_spectra_11_masked),  len(tof_spectra_11_original) - 1)

        # Make sure that there are no elements
        for start, stop in zip(bin_mask_general_start, bin_mask_general_stop):
            self.assertFalse(any(elements_in_range(start, stop, tof_spectra_10_masked)))
            self.assertFalse(any(elements_in_range(start, stop, tof_spectra_11_masked)))

    def test_that_detector_specific_time_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()
        mask_builder = get_mask_builder(data_info)

        # Expected_spectra
        bin_mask_start = [27000., 58000.]
        bin_mask_stop = [45000., 61000.]

        mask_builder.set_LAB_bin_mask_start(bin_mask_start)
        mask_builder.set_LAB_bin_mask_stop(bin_mask_stop)

        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Is part of LAB
        tof_spectra_23813_original = workspace.getSpectrum(23813).getTofs()

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        # Confirm that everything in the ranges 27000-45000  and 58000-61000 is removed from the event list
        tof_spectra_23813_masked = workspace.getSpectrum(23813).getTofs()

        # Spectrum 23813
        # Five events should have been removed
        self.assertEqual(len(tof_spectra_23813_masked),  len(tof_spectra_23813_original) - 5)

        # Make sure that there are no elements
        for start, stop in zip(bin_mask_start, bin_mask_stop):
            self.assertFalse(any(elements_in_range(start, stop, tof_spectra_23813_masked)))

    def test_that_angle_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()

        mask_builder = get_mask_builder(data_info)

        # Expected_spectra
        phi_mirror = False
        phi_min = 0.
        phi_max = 90.
        # This should mask everything except for the upper right quadrant
        # | 120              |-------------------|
        # |                 |---------------------|
        # | 60               |-------------------|
        # |                 |----------------------|
        # |
        # |
        # |-------------------|------------------|
        # 512                256                 0

        expected_spectra = []
        # The strange double pattern arises from the offset of the SANS2D tube geometry (see InstrumentView)
        for y in range(60, 120):
            if y % 2 == 0:
                expected_spectra.extend(((y * 512) + 9 + x for x in range(0, 255)))
            else:
                expected_spectra.extend(((y * 512) + 9 + x for x in range(0, 257)))
        expected_spectra.extend((x for x in range(92169, 122889)))  # HAB

        mask_builder.set_use_mask_phi_mirror(phi_mirror)
        mask_builder.set_phi_min(phi_min)
        mask_builder.set_phi_max(phi_max)

        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert_non_masked(workspace, expected_spectra)

    def test_that_beam_stop_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()
        mask_builder = get_mask_builder(data_info)

        beam_stop_arm_width = .01
        beam_stop_arm_angle = 180.0
        beam_stop_arm_pos1 = 0.0
        beam_stop_arm_pos2 = 0.0

        # Expected_spectra, again the tubes are shifted and that will produce the slightly strange masking
        expected_spectra = []
        expected_spectra.extend((512*59 + 9 + x for x in range(0, 257)))
        expected_spectra.extend((512*60 + 9 + x for x in range(0, 255)))

        mask_builder.set_beam_stop_arm_width(beam_stop_arm_width)
        mask_builder.set_beam_stop_arm_angle(beam_stop_arm_angle)
        mask_builder.set_beam_stop_arm_pos1(beam_stop_arm_pos1)
        mask_builder.set_beam_stop_arm_pos2(beam_stop_arm_pos2)

        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)

    def test_that_beam_stop_masking_is_applied_for_LOQ(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("LOQ74044")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()
        mask_builder = get_mask_builder(data_info)

        beam_stop_arm_width = .01
        beam_stop_arm_angle = 180.0
        beam_stop_arm_pos1 = 0.0
        beam_stop_arm_pos2 = 0.0

        # Expected_spectra, again the tubes are shifted and that will produce the slightly strange masking
        expected_spectra = []
        expected_spectra.extend((7811 + x for x in range(0, 63)))
        expected_spectra.extend((7939 + x for x in range(0, 63)))

        mask_builder.set_beam_stop_arm_width(beam_stop_arm_width)
        mask_builder.set_beam_stop_arm_angle(beam_stop_arm_angle)
        mask_builder.set_beam_stop_arm_pos1(beam_stop_arm_pos1)
        mask_builder.set_beam_stop_arm_pos2(beam_stop_arm_pos2)

        mask_info = mask_builder.build()

        move_builder = get_move_builder(data_info)
        move_builder.set_center_position(0.)
        move_info = move_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info, move_state=move_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)

    def test_that_cylinder_masking_is_applied(self):
        # Arrange
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00028827")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()
        mask_builder = get_mask_builder(data_info)

        # Radius Mask
        radius_min = 0.01
        radius_max = 10.

        expected_spectra = []
        expected_spectra.extend([30469, 30470, 30471, 30472, 30473, 30474, 30475, 30476, 30477, 30980,
                                 30981, 30982, 30983, 30984, 30985, 30986, 30987, 30988])
        mask_builder.set_radius_min(radius_min)
        mask_builder.set_radius_max(radius_max)

        mask_info = mask_builder.build()

        test_director = TestDirector()
        test_director.set_states(data_state=data_info, mask_state=mask_info)
        state = test_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        # Assert
        self._do_assert(workspace, expected_spectra)


class SANSMaskWorkspaceRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSMaskWorkspaceTest, 'test'))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success


if __name__ == '__main__':
    unittest.main()
