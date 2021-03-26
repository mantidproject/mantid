# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# Standard and third-party
import numpy as np
from numpy.testing import assert_allclose
from os import path
import unittest

# Mantid import
from mantid import config
from mantid.api import AnalysisDataService, mtd
from mantid.simpleapi import DeleteWorkspaces, LoadNexusProcessed

# Calibration imports
from Calibration.tube_calib import correct_tube_to_ideal_tube, getCalibratedPixelPositions


class TestTubeCalib(unittest.TestCase):
    @classmethod
    def setUpClass(cls):  # called only before running all tests in the test case
        cls.workspaces_temporary = list()  # workspaces to be deleted at tear-down

        # Single tube data. Tube dimensions appropriate for a CORELLI tube
        def y_quad(n: float) -> float:
            r"""
            Example quadratic function, returning the Y-coordinate (meters) versus pixel index `i

            y_quad(n) = c0 + c1 * n + c2 * n^2.
            Coefficients c0, c1, and c2 obtained by solving the following equations:
                y(0) = -0.502
                y(128) = 0.001
                y(255) = 0.393  # assume a tube with 256 pixels
            Obtaining:
                c0 = -0.502
                c1 = 0.00435287724834028
                c2 = -3.306169908908442e-06

            :param n: pixel coordinate
            """
            return -0.502 + 0.00435287724834028 * n - 3.306169908908442e-06 * n * n

        # assume 11 slits(wires) casting 11 peaks(shadows) onto the tube at the following pixel numbers
        tube_points = np.linspace(5, 245, 11, endpoint=True)  # 5, 29, 53,...,221, 245
        # assume the Y-coordinates of the peaks(shadows) given by our quadratic example function
        ideal_tube_coordinates = [y_quad(n) for n in tube_points]
        cls.y_quad_data = {
            'detector_count': 256,
            'peak_count': 11,
            'y_quad': y_quad,
            'coefficients': {
                'A0': -0.502,
                'A1': 0.00435287724834028,
                'A2': -3.306169908908442e-06
            },
            'tube_points': tube_points,
            'ideal_tube_coordinates': ideal_tube_coordinates
        }

        # Load a CORELLI file containing data for bank number 20 (16 tubes)
        config.appendDataSearchSubDir('CORELLI/calibration')
        for directory in config.getDataSearchDirs():
            if 'UnitTest' in directory:
                data_dir = path.join(directory, 'CORELLI', 'calibration')
                break
        workspace = 'CORELLI_123455_bank20'
        LoadNexusProcessed(Filename=path.join(data_dir, workspace + '.nxs'), OutputWorkspace=workspace)
        assert AnalysisDataService.doesExist(workspace)
        cls.workspaces_temporary.append(workspace)  # delete workspace at tear-down
        cls.corelli = {
            'tube_length': 0.900466,  # in meters
            'pixels_per_tube': 256,
            'workspace': workspace
        }

    @classmethod
    def tearDownClass(cls) -> None:  # called only after all tests in the test case have run
        r"""Delete the workspaces associated to the test cases"""
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)

    def test_correct_tube_to_ideal_tube(self):
        # Verify the quadratic fit works
        data = self.y_quad_data

        # fit the Y-coordinates to the pixel positions with a default quadratic function
        fitted_coordinates = correct_tube_to_ideal_tube(data['tube_points'],
                                                        data['ideal_tube_coordinates'],
                                                        data['detector_count'],
                                                        parameters_table='parameters')

        # Verify the fitted coordinates are the ideal_tube_coordinates
        assert_allclose([fitted_coordinates[int(n)] for n in data['tube_points']],
                        data['ideal_tube_coordinates'],
                        atol=0.0001)
        # Compare fitting coefficients
        assert AnalysisDataService.doesExist('parameters')
        # here retrieve the fitting coefficients from the 'parameters' table and compare to the expected values
        expected = data['coefficients']
        for row in mtd['parameters']:
            if row['Name'] in expected:
                self.assertAlmostEqual(row['Value'], expected[row['Name']], places=6)
        # a bit of clean-up
        DeleteWorkspaces(
            ['PolyFittingWorkspace', 'QF_NormalisedCovarianceMatrix', 'QF_Parameters', 'QF_Workspace', 'parameters'])

    def test_getCalibratedPixelPositions(self):
        data = self.y_quad_data
        # calibrate the first tube of bank 20 in the corelli input workspace
        detector_ids, detector_positions = \
            getCalibratedPixelPositions(self.corelli['workspace'],
                                        data['tube_points'],
                                        data['ideal_tube_coordinates'],
                                        range(0, self.corelli['pixels_per_tube']),  # first 256 workspace indexes
                                        parameters_table='parameters')

        # 77824 is the detector ID for the first pixel in bank 20
        self.assertEqual(detector_ids, list(range(77824, 77824 + self.corelli['pixels_per_tube'])))

        # Assert the detector positions were adjusted to the input quadratic function
        y_quad = data['y_quad']
        expected_y = (-1.30686 - y_quad(0)) + np.array([y_quad(n) for n in range(0, self.corelli['pixels_per_tube'])])
        assert_allclose([xyz[1] for xyz in detector_positions], expected_y, atol=0.0001)

        # here retrieve the fitting coefficients from the 'parameters' table and compare to the expected values
        assert AnalysisDataService.doesExist('parameters')
        expected = data['coefficients']
        for row in mtd['parameters']:
            if row['Name'] in expected:
                self.assertAlmostEqual(row['Value'], expected[row['Name']], places=6)

        # a bit of clean-up
        DeleteWorkspaces(
            ['parameters', 'PolyFittingWorkspace', 'QF_NormalisedCovarianceMatrix', 'QF_Parameters', 'QF_Workspace'])


if __name__ == '__main__':
    unittest.main()
