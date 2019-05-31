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

from mantid.kernel import config
from mantid.api import AnalysisDataService
from mantid.simpleapi import (CreateSampleWorkspace, DeleteWorkspace)
from math import sqrt

from sans.algorithm_detail.q_resolution_calculator import (QResolutionCalculatorFactory, NullQResolutionCalculator,
                                                           QResolutionCalculatorISIS, get_aperture_diameters,
                                                           load_sigma_moderator_workspace, create_q_resolution_workspace)
from sans.common.enums import (SANSInstrument, SANSFacility)


class MockContainer:
    convert_to_q = None
    data = None


class QResolutionCalculatorTest(unittest.TestCase):
    moderator_file_name = "test_moderator_file.txt"
    data_workspace = None
    data_workspace_name = "q_resolution_test_workspace"

    def _assert_collection_elements_are_equal(self, collection1, collection2):
        tolerance = 1e-7
        self.assertEqual(len(collection1),  len(collection2))
        for index in range(len(collection1)):
            self.assertTrue(abs(collection1[index] - collection2[index]) < tolerance)

    @staticmethod
    def _get_path(file_name):
        save_directory = config['defaultsave.directory']
        if not os.path.isdir(save_directory):
            save_directory = os.getcwd()
        return os.path.join(save_directory, file_name)

    @staticmethod
    def _save_file(file_path, content):
        with open(file_path, "w") as f:
            f.write(content)

    @classmethod
    def setUpClass(cls):
        # Create a moderator file
        moderator_content = ("  Thu 30-JUL-2015 15:04 Workspace: TS1_H2_resultStddev_hist \n"
                             "\n"
                             "   4    0    0    0    1   4    0\n"
                             "         0         0         0         0\n"
                             " 3 (F12.5,2E16.6)\n"
                             "     1.0      1.000000e+00    0.000000e+00\n"
                             "     4.0      2.000000e+00    0.000000e+00\n"
                             "     7.0      3.000000e+00    0.000000e+00\n"
                             "     10.0     4.000000e+00    0.000000e+00\n")
        cls.moderator_file_name = cls._get_path(cls.moderator_file_name)
        cls._save_file(cls.moderator_file_name, moderator_content)

        # Create a data workspace with 4 histograms and 4 bins with centres at 2, 4, 6, 8 Angstrom.
        if cls.data_workspace is None:
            CreateSampleWorkspace(OutputWorkspace=cls.data_workspace_name, NumBanks=1, BankPixelWidth=2,
                                  XUnit='Wavelength', XMin=1, XMax=10, BinWidth=2)
            cls.data_workspace = AnalysisDataService.retrieve(cls.data_workspace_name)

    @classmethod
    def tearDownClass(cls):
        if cls.data_workspace:
            DeleteWorkspace(cls.data_workspace)
        if os.path.exists(cls.moderator_file_name):
            os.remove(cls.moderator_file_name)

    @staticmethod
    def _provide_mock_state(use_q_resolution, **kwargs):
        mock_state = MockContainer()
        mock_state.data = MockContainer()
        mock_state.data.instrument = SANSInstrument.LARMOR
        mock_state.data.facility = SANSFacility.ISIS
        convert_to_q = MockContainer()
        convert_to_q.use_q_resolution = use_q_resolution
        for key, value in kwargs.items():
            setattr(convert_to_q, key, value)
        mock_state.convert_to_q = convert_to_q
        return mock_state

    def test_that_raises_when_unknown_facility_is_chosen(self):
        # Arrange
        mock_state = MockContainer()
        mock_state.data = MockContainer()
        mock_state.data.facility = None
        factory = QResolutionCalculatorFactory()

        # Act + Assert
        args = [mock_state]
        self.assertRaises(RuntimeError, factory.create_q_resolution_calculator, *args)

    def test_that_provides_null_q_resolution_calculator_when_is_turned_off(self):
        # Arrange
        mock_state = QResolutionCalculatorTest._provide_mock_state(use_q_resolution=False)
        factory = QResolutionCalculatorFactory()
        # Act
        calculator = factory.create_q_resolution_calculator(mock_state)
        # Assert
        self.assertTrue(isinstance(calculator, NullQResolutionCalculator))

    def test_that_provides_isis_q_resolution_calculator_when_is_turned_on(self):
        # Arrange
        mock_state = QResolutionCalculatorTest._provide_mock_state(use_q_resolution=True)
        factory = QResolutionCalculatorFactory()
        # Act
        calculator = factory.create_q_resolution_calculator(mock_state)
        # Assert
        self.assertTrue(isinstance(calculator, QResolutionCalculatorISIS))

    def test_that_calculates_the_aperture_diameters_for_circular_settings(self):
        # Arrange
        q_options = {"q_resolution_a1": 2., "q_resolution_a2": 3., "q_resolution_h1": None, "q_resolution_h2": None,
                     "q_resolution_w1": None, "q_resolution_w2": None}
        mock_state = QResolutionCalculatorTest._provide_mock_state(use_q_resolution=True, **q_options)
        # Act
        a1, a2 = get_aperture_diameters(mock_state.convert_to_q)
        # Assert
        self.assertEqual(a1,  2.)
        self.assertEqual(a2,  3.)

    def test_that_calculates_the_aperture_diameters_for_rectangular_settings(self):
        # Arrange
        q_options = {"q_resolution_a1": 2., "q_resolution_a2": 3., "q_resolution_h1": 4., "q_resolution_h2": 6.,
                     "q_resolution_w1": 7., "q_resolution_w2": 8.}
        mock_state = QResolutionCalculatorTest._provide_mock_state(use_q_resolution=True, **q_options)
        # Act
        a1, a2 = get_aperture_diameters(mock_state.convert_to_q)
        # Assert
        expected_a1 = 2*sqrt((16. + 49.) / 6)
        expected_a2 = 2 * sqrt((36. + 64.) / 6)
        self.assertEqual(a1,  expected_a1)
        self.assertEqual(a2,  expected_a2)

    def test_that_defaults_to_circular_if_not_all_rectangular_are_specified(self):
        q_options = {"q_resolution_a1": 2., "q_resolution_a2": 3., "q_resolution_h1": 4., "q_resolution_h2": None,
                     "q_resolution_w1": 7., "q_resolution_w2": 8.}
        mock_state = QResolutionCalculatorTest._provide_mock_state(use_q_resolution=True, **q_options)
        # Act
        a1, a2 = get_aperture_diameters(mock_state.convert_to_q)
        # Assert
        self.assertEqual(a1,  2.)
        self.assertEqual(a2,  3.)

    def test_that_moderator_workspace_is_histogram(self):
        # Arrange
        file_name = self._get_path(QResolutionCalculatorTest.moderator_file_name)
        # Act
        workspace = load_sigma_moderator_workspace(file_name)
        # Assert
        self.assertEqual(len(workspace.dataX(0)),  len(workspace.dataY(0)) + 1)

    def test_that_executes_the_q_resolution_calculation_without_issues(self):
        # Arrange
        q_options = {"q_resolution_a1": 2., "q_resolution_a2": 3., "q_resolution_h1": None, "q_resolution_h2": None,
                     "q_resolution_w1": None, "q_resolution_w2": None}
        mock_state = QResolutionCalculatorTest._provide_mock_state(use_q_resolution=True, **q_options)
        file_name = self._get_path(QResolutionCalculatorTest.moderator_file_name)
        mock_state.convert_to_q.moderator_file = file_name
        mock_state.convert_to_q.q_resolution_collimation_length = 3.
        mock_state.convert_to_q.q_resolution_delta_r = 0.002
        mock_state.convert_to_q.use_gravity = False
        mock_state.convert_to_q.gravity_extra_length = 0.

        # Act
        q_resolution_workspace = create_q_resolution_workspace(mock_state.convert_to_q,
                                                               QResolutionCalculatorTest.data_workspace)
        # Assert
        self.assertTrue(q_resolution_workspace is not None)
        self.assertEqual(len(q_resolution_workspace.dataX(0)),  len(QResolutionCalculatorTest.data_workspace.dataX(0)))
        self.assertEqual(len(q_resolution_workspace.dataY(0)),  len(QResolutionCalculatorTest.data_workspace.dataY(0)))
        for e1, e2 in zip(q_resolution_workspace.dataX(0), QResolutionCalculatorTest.data_workspace.dataX(0)):
            self.assertEqual(e1,  e2)

if __name__ == "__main__":
    unittest.main()
