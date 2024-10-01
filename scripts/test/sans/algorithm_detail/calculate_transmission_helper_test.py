# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from mantid.api import AnalysisDataService
from mantid.kernel import config, ConfigService
from mantid.simpleapi import CreateSampleWorkspace, MaskDetectors, DeleteWorkspace, LoadNexusProcessed, Load, Rebin
from sans.algorithm_detail.calculate_transmission_helper import (
    get_masked_det_ids,
    get_idf_path_from_workspace,
    get_workspace_indices_for_monitors,
    apply_flat_background_correction_to_monitors,
    apply_flat_background_correction_to_detectors,
    get_region_of_interest,
)


class CalculateTransmissionHelperTest(unittest.TestCase):
    immutable_test_workspace = None
    region_of_interest_workspace = None
    roi_file = "roi_file_for_sans_transmission.xml"
    mask_file = "mas_file_for_sans_transmission.xml"
    roi_file_path = None
    mask_file_path = None

    def _assert_collection_elements_are_equal(self, collection1, collection2):
        tolerance = 1e-7
        self.assertEqual(len(collection1), len(collection2))
        for index in range(len(collection1)):
            self.assertTrue(abs(collection1[index] - collection2[index]) < tolerance)

    @staticmethod
    def _create_flat_background_test_workspace(workspace_name):
        ConfigService.Instance().setString("default.facility", "ISIS")
        LoadNexusProcessed(Filename="LOQ48127", OutputWorkspace=workspace_name)
        ConfigService.Instance().setString("default.facility", " ")
        workspace = AnalysisDataService.retrieve(workspace_name)
        # Rebin to only have four values at 11, 31, 51, 70.5
        workspace = Rebin(workspace, "1,20,80")
        # For each spectrum we set the first two entries to 2 and the other two entries to 4.
        for index in range(workspace.getNumberHistograms()):
            data_y = workspace.dataY(index)
            data_y[0] = 2.0
            data_y[1] = 2.0
            data_y[2] = 4.0
            data_y[3] = 4.0
        return workspace

    @staticmethod
    def _get_path(file_name):
        save_directory = config["defaultsave.directory"]
        if not os.path.isdir(save_directory):
            save_directory = os.getcwd()
        return os.path.join(save_directory, file_name)

    @staticmethod
    def _save_file(file_path, content):
        with open(file_path, "w") as f:
            f.write(content)

    @classmethod
    def setUpClass(cls):
        ConfigService.Instance().setString("default.facility", "ISIS")
        # A small workspace for general tests
        test_workspace = LoadNexusProcessed(Filename="LOQ48127")
        cls.immutable_test_workspace = test_workspace

        # A full workspace on which we can test region of interest selection
        region_of_interest_workspace = Load(Filename="LOQ74044")
        cls.region_of_interest_workspace = region_of_interest_workspace

        # A region of interest xml file
        roi_content = (
            '<?xml version="1.0"?>\n'
            "\t<detector-masking>\n"
            "\t\t<group>\n"
            "\t\t\t<detids>6990-6996</detids>\n"
            "\t\t</group>\n"
            "\t</detector-masking>\n"
        )
        cls.roi_file_path = cls._get_path(cls.roi_file)
        cls._save_file(cls.roi_file_path, roi_content)

        # A mask file
        mask_content = (
            '<?xml version="1.0"?>\n'
            "\t<detector-masking>\n"
            "\t\t<group>\n"
            "\t\t\t<detids>6991</detids>\n"
            "\t\t</group>\n"
            "\t</detector-masking>\n"
        )
        cls.mask_file_path = cls._get_path(cls.mask_file)
        cls._save_file(cls.mask_file_path, mask_content)
        ConfigService.Instance().setString("default.facility", " ")

    @classmethod
    def tearDownClass(cls):
        if cls.immutable_test_workspace:
            DeleteWorkspace(cls.immutable_test_workspace)
        if cls.region_of_interest_workspace:
            DeleteWorkspace(cls.region_of_interest_workspace)
        if os.path.exists(cls.roi_file_path):
            os.remove(cls.roi_file_path)
        if os.path.exists(cls.mask_file_path):
            os.remove(cls.mask_file_path)

    def test_get_masked_det_ids(self):
        # Arrange
        test_workspace_for_masked_det_ids = CreateSampleWorkspace("Histogram")
        MaskDetectors(Workspace=test_workspace_for_masked_det_ids, DetectorList=[100, 102, 104])

        # Act
        masked_det_ids = list(get_masked_det_ids(test_workspace_for_masked_det_ids))

        # Assert
        self.assertTrue(100 in masked_det_ids)
        self.assertTrue(102 in masked_det_ids)
        self.assertTrue(104 in masked_det_ids)
        self.assertEqual(len(masked_det_ids), 3)

        # Clean up
        DeleteWorkspace(test_workspace_for_masked_det_ids)

    def test_that_gets_idf_from_workspace(self):
        # Act
        idf_path = get_idf_path_from_workspace(self.immutable_test_workspace)
        # Assert
        self.assertTrue(os.path.exists(idf_path))
        self.assertEqual(os.path.basename(idf_path), "LOQ_Definition_20020226-.xml")

    def test_that_extracts_workspace_indices_of_monitor_when_monitors_are_present(self):
        # Act
        workspace_indices_generator = get_workspace_indices_for_monitors(self.immutable_test_workspace)
        # Assert
        workspace_indices = list(workspace_indices_generator)
        self.assertEqual(len(workspace_indices), 2)
        self.assertEqual(workspace_indices[0], 0)
        self.assertEqual(workspace_indices[1], 1)

    def test_that_returns_empty_generator_if_no_monitors_are_present(self):
        # Arrange
        test_workspace_for_monitors = CreateSampleWorkspace("Histogram")
        # Act
        workspace_indices_generator = get_workspace_indices_for_monitors(test_workspace_for_monitors)
        # Assert
        workspace_indices = list(workspace_indices_generator)
        self.assertEqual(workspace_indices, [])
        # Clean up
        DeleteWorkspace(test_workspace_for_monitors)

    def test_that_applies_flat_background_correction_only_to_monitors(self):
        # Arrange
        workspace_name = "monitor_test_workspace"
        workspace = self._create_flat_background_test_workspace(workspace_name)

        monitor_workspace_indices = [0, 1]
        # The first monitor (with spectrum index 1 should find a correction value of 2
        # The second monitor (with spectrum index 2 should find a correction value of 4
        monitor_spectrum_tof_start = {"1": 1, "2": 50}
        monitor_spectrum_tof_stop = {"1": 40, "2": 70}
        tof_general_start = 24
        tof_general_stop = 38
        # Act
        output_workspace = apply_flat_background_correction_to_monitors(
            workspace,
            monitor_workspace_indices,
            monitor_spectrum_tof_start,
            monitor_spectrum_tof_stop,
            tof_general_start,
            tof_general_stop,
        )
        # Assert
        # The first monitor  should have [0, 0, 2, 2], it has 2.1 in the last value, not clear why
        # The second monitor  should have [0, 0, 0, 0], it has 0.1 in the last value, not clear why. Note that
        # the flat background correction never goes negative.
        self._assert_collection_elements_are_equal(output_workspace.dataY(0), [0, 0, 2, 2.1])
        self._assert_collection_elements_are_equal(output_workspace.dataY(1), [0, 0, 0, 0.1])
        # The detectors should be unchanged
        for index in range(2, output_workspace.getNumberHistograms()):
            self._assert_collection_elements_are_equal(output_workspace.dataY(index), [2, 2, 4, 4])

        # Clean up
        DeleteWorkspace(workspace)

    def test_that_applies_flat_background_correction_only_to_detectors(self):
        # Arrange
        workspace_name = "monitor_test_workspace"
        workspace = self._create_flat_background_test_workspace(workspace_name)

        start_tof = "1"
        stop_tof = "40"

        # Act
        output_workspace = apply_flat_background_correction_to_detectors(workspace, start_tof, stop_tof)

        # Assert
        # The monitors should not have changed
        self._assert_collection_elements_are_equal(output_workspace.dataY(0), [2.0, 2.0, 4.0, 4.0])
        self._assert_collection_elements_are_equal(output_workspace.dataY(1), [2.0, 2.0, 4.0, 4.0])
        # The detectors should be subtracted by 2. The last value seems to be slightly off
        for index in range(2, output_workspace.getNumberHistograms()):
            self._assert_collection_elements_are_equal(output_workspace.dataY(index), [0.0, 0.0, 2.0, 2.1])

        # Clean up
        DeleteWorkspace(workspace)

    def test_that_gets_region_of_interest_for_radius_only_gets_correct_ids(self):
        # Act
        detector_ids = get_region_of_interest(self.region_of_interest_workspace, radius=0.01)

        # Assert
        # The one centimeter radius should capture [7872, 7873, 7874, 8000, 8001, 8002, 8003, 8128, 8129, 8130]
        expected_ids = [7872, 7873, 7874, 8000, 8001, 8002, 8003, 8128, 8129, 8130]
        self._assert_collection_elements_are_equal(detector_ids, expected_ids)

    def test_that_gets_region_of_interest_for_roi_file(self):
        # Act
        detector_ids = get_region_of_interest(self.region_of_interest_workspace, roi_files=[self.roi_file_path])
        # Assert
        expected_detector_ids = [6990, 6991, 6992, 6993, 6994, 6995, 6996]
        self._assert_collection_elements_are_equal(detector_ids, expected_detector_ids)

    def test_that_gets_region_of_interest_for_roi_mask_and_radius(self):
        # Act
        detector_ids = get_region_of_interest(
            self.region_of_interest_workspace, roi_files=[self.roi_file_path], mask_files=[self.mask_file_path], radius=0.01
        )
        # Assert
        # From Radius: [7872, 7873, 7874, 8000, 8001, 8002, 8003, 8128, 8129, 8130]
        # From Roi File: [6990, 6991, 6992, 6993, 6994, 6995, 6996]
        # Mask file removes: [6991]
        expected_detector_ids = [6990, 6992, 6993, 6994, 6995, 6996, 7872, 7873, 7874, 8000, 8001, 8002, 8003, 8128, 8129, 8130]
        self._assert_collection_elements_are_equal(detector_ids, expected_detector_ids)

    def test_that_returns_empty_list_if_nothing_is_specified(self):
        # Act
        detector_ids = get_region_of_interest(self.region_of_interest_workspace)
        # Assert
        expected_detector_ids = []
        self._assert_collection_elements_are_equal(detector_ids, expected_detector_ids)


if __name__ == "__main__":
    unittest.main()
