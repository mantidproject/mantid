# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from numpy.testing import assert_allclose
from os import path
import unittest

from mantid import AnalysisDataService, config
from mantid.kernel import V3D
from mantid.simpleapi import (CreateEmptyTableWorkspace, DeleteWorkspaces, GroupWorkspaces, LoadEmptyInstrument,
                              LoadNexusProcessed, mtd)

from corelli.calibration.utils import (apply_calibration, preprocess_banks, bank_numbers, calculate_peak_y_table,
                                       calibrate_tube, load_banks, trim_calibration_table, wire_positions)


class TestUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        r"""
        Load the tests cases for calibrate_bank, consisting of data for only one bank
        CORELLI_124023_bank10, tube 13 has shadows at pixel numbers quite different from the rest
        """
        config.appendDataSearchSubDir('CORELLI/calibration')
        for directory in config.getDataSearchDirs():
            if 'UnitTest' in directory:
                data_dir = path.join(directory, 'CORELLI', 'calibration')
                break
        cls.workspaces_temporary = list()
        cls.cases = dict()
        for bank_case in ('124023_bank10', ):
            workspace = 'CORELLI_' + bank_case
            LoadNexusProcessed(Filename=path.join(data_dir, workspace + '.nxs'), OutputWorkspace=workspace)
            cls.cases[bank_case] = workspace
            cls.workspaces_temporary.append(workspace)

    def setUp(self) -> None:
        # Neutron counts along a tube
        counts = [
            50, 122, 118, 197, 414, 545, 1026, 1767, 3242, 7132, 16232, 34663, 52842, 59767, 58638, 55250, 47723, 47400,
            54642, 56830, 57664, 57563, 57398, 57438, 57370, 57386, 57447, 56951, 57258, 56195, 52775, 46106, 48802,
            54698, 57072, 57876, 57754, 57712, 57800, 57837, 57606, 57398, 57038, 56504, 54408, 49406, 44668, 50911,
            55364, 55906, 56175, 56612, 56375, 56179, 56265, 56329, 56497, 56131, 55034, 52618, 46515, 44466, 51293,
            54234, 54961, 55219, 54601, 54359, 54866, 54839, 54474, 54031, 54064, 53431, 50619, 44013, 45435, 52264,
            54087, 55002, 54589, 54568, 53516, 53285, 52656, 52676, 52420, 52375, 51837, 47664, 41643, 45292, 51215,
            52851, 53284, 53421, 52999, 52985, 52596, 52799, 52658, 51894, 51504, 50154, 44435, 39820, 45640, 49871,
            50920, 51190, 51091, 51448, 50937, 50927, 50773, 51173, 50819, 50110, 48138, 42389, 39823, 46671, 50037,
            50013, 50840, 50400, 50138, 49500, 50616, 49890, 49853, 49548, 48731, 45211, 39185, 39270, 45409, 47628,
            48348, 48652, 48025, 48518, 48915, 48351, 48486, 48391, 47793, 47079, 43590, 37528, 40016, 45604, 47259,
            47710, 47252, 47687, 47145, 47453, 47409, 47294, 46735, 46170, 45028, 40783, 35924, 40234, 44613, 45660,
            45921, 45125, 45544, 45482, 45261, 45326, 45547, 44914, 44966, 43711, 39302, 35101, 39600, 44185, 45054,
            44538, 44443, 44617, 44509, 44589, 44806, 45078, 44265, 44053, 42055, 36702, 35192, 40304, 42712, 42910,
            42992, 43593, 44157, 43675, 43442, 43348, 43668, 42447, 42193, 39349, 33887, 34026, 38630, 40354, 40798,
            40866, 40592, 40593, 40645, 40507, 40316, 40256, 40068, 39481, 37928, 33034, 31521, 36298, 38938, 39451,
            39544, 39479, 39624, 39471, 39300, 39197, 38777, 39288, 39163, 37350, 33507, 31309, 36124, 38921, 30633,
            14932, 5370, 2069, 955, 545, 329, 216, 157, 82, 49, 54, 25, 10
        ]
        # Calibrated Y-coordinates
        y = [
            -0.342, -0.338, -0.335, -0.331, -0.328, -0.324, -0.320, -0.317, -0.313, -0.310, -0.306, -0.303, -0.299,
            -0.296, -0.292, -0.289, -0.285, -0.282, -0.278, -0.275, -0.271, -0.268, -0.264, -0.261, -0.257, -0.254,
            -0.250, -0.247, -0.243, -0.240, -0.236, -0.232, -0.229, -0.225, -0.222, -0.218, -0.215, -0.211, -0.208,
            -0.204, -0.201, -0.197, -0.194, -0.190, -0.187, -0.183, -0.180, -0.176, -0.173, -0.169, -0.166, -0.162,
            -0.159, -0.155, -0.152, -0.148, -0.144, -0.141, -0.137, -0.134, -0.130, -0.127, -0.123, -0.120, -0.116,
            -0.113, -0.109, -0.106, -0.102, -0.099, -0.095, -0.092, -0.088, -0.085, -0.081, -0.078, -0.074, -0.071,
            -0.067, -0.064, -0.060, -0.056, -0.053, -0.049, -0.046, -0.042, -0.039, -0.035, -0.032, -0.028, -0.025,
            -0.021, -0.018, -0.014, -0.011, -0.007, -0.004, -0.000, 0.003, 0.007, 0.010, 0.014, 0.017, 0.021, 0.024,
            0.028, 0.032, 0.035, 0.039, 0.042, 0.046, 0.049, 0.053, 0.056, 0.060, 0.063, 0.067, 0.070, 0.074, 0.077,
            0.081, 0.084, 0.088, 0.091, 0.095, 0.098, 0.102, 0.105, 0.109, 0.112, 0.116, 0.120, 0.123, 0.127, 0.130,
            0.134, 0.137, 0.141, 0.144, 0.148, 0.151, 0.155, 0.158, 0.162, 0.165, 0.169, 0.172, 0.176, 0.179, 0.183,
            0.186, 0.190, 0.193, 0.197, 0.200, 0.204, 0.208, 0.211, 0.215, 0.218, 0.222, 0.225, 0.229, 0.232, 0.236,
            0.239, 0.243, 0.246, 0.250, 0.253, 0.257, 0.260, 0.264, 0.267, 0.271, 0.274, 0.278, 0.281, 0.285, 0.288,
            0.292, 0.296, 0.299, 0.303, 0.306, 0.310, 0.313, 0.317, 0.320, 0.324, 0.327, 0.331, 0.334, 0.338, 0.341,
            0.345, 0.348, 0.352, 0.355, 0.359, 0.362, 0.366, 0.369, 0.373, 0.376, 0.380, 0.384, 0.387, 0.391, 0.394,
            0.398, 0.401, 0.405, 0.408, 0.412, 0.415, 0.419, 0.422, 0.426, 0.429, 0.433, 0.436, 0.440, 0.443, 0.447,
            0.450, 0.454, 0.457, 0.461, 0.464, 0.468, 0.472, 0.475, 0.479, 0.482, 0.486, 0.489, 0.493, 0.496, 0.500,
            0.503, 0.507, 0.510, 0.514, 0.517, 0.521, 0.524, 0.528, 0.531, 0.535, 0.538, 0.542, 0.545, 0.549, 0.552,
            0.556
        ]
        # Create a workspace with an uncalibrated tube bank42/sixteenpack/tube8
        workspace = LoadEmptyInstrument(InstrumentName='CORELLI',
                                        MakeEventWorkspace=False,
                                        OutputWorkspace='uncalibrated')
        # the workspace index for the first pixel in bank87/sixteenpack/tube12 is 355075
        for pixel_index in range(256):
            workspace.dataY(355075 + pixel_index)[:] = counts[pixel_index]
        self.workspace = 'uncalibrated'
        self.table = 'calibTable'
        self.calibrated_y = y

    @classmethod
    def tearDownClass(cls) -> None:
        r"""Delete temporary workspaces"""
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)

    def test_wire_positions(self):
        with self.assertRaises(AssertionError) as exception_info:
            wire_positions(units='mm')
        assert 'units mm must be one of' in str(exception_info.exception)
        expected = [
            -0.396, -0.343, -0.290, -0.238, -0.185, -0.132, -0.079, -0.026, 0.026, 0.079, 0.132, 0.185, 0.238, 0.290,
            0.343, 0.396
        ]
        assert_allclose(wire_positions(units='meters'), np.array(expected), atol=0.001)
        expected = [
            15.4, 30.4, 45.4, 60.4, 75.4, 90.5, 105.5, 120.5, 135.5, 150.5, 165.5, 180.6, 195.6, 210.6, 225.6, 240.6
        ]
        assert_allclose(wire_positions(units='pixels'), np.array(expected), atol=0.1)

    def test_bank_numbers(self):
        assert bank_numbers(' 42 ') == ['42']
        assert bank_numbers(' 32 , 42 ') == ['32', '42']
        assert bank_numbers(' 32 - 34 ') == ['32', '33', '34']
        assert bank_numbers(' 32 - 34 , 37 ') == ['32', '33', '34', '37']
        assert bank_numbers(' 32 - 34 , 37 -  38 ') == ['32', '33', '34', '37', '38']
        assert bank_numbers(' 32 - 34 , 37 , 39 - 41 ') == ['32', '33', '34', '37', '39', '40', '41']

    def test_preprocess_banks(self):
        ref = np.array([
            16406.101884326443,  # mean
            1028.9536367012543,  # std
            16811.476562499956,  # median
            67199393.31820111,  # sum
        ])
        intputws = TestUtils.cases['124023_bank10']  # try to use existing data
        preprocess_banks(intputws, "_ws")
        _ws = mtd["_ws"]
        sig = np.array([_ws.readY(i) for i in range(_ws.getNumberHistograms())])
        rst = np.array([sig.mean(), sig.std(), np.median(sig), sig.sum()])
        np.testing.assert_allclose(rst, ref)

    def test_load_banks(self):
        # loading a non-existing file
        with self.assertRaises(AssertionError) as exception_info:
            load_banks('I_am_no_here', '58', output_workspace='jambalaya')
        assert 'File I_am_no_here does not exist' in str(exception_info.exception)

        # loading an event nexus file will take too much time, so it's left as a system test.
        # loading a nexus processed file
        for directory in config.getDataSearchDirs():
            if 'UnitTest' in directory:
                data_dir = path.join(directory, 'CORELLI', 'calibration')
                config.appendDataSearchDir(path.join(directory, 'CORELLI', 'calibration'))
                break
        workspace = load_banks(path.join(data_dir, 'CORELLI_123454_bank58.nxs'), '58', output_workspace='jambalaya')
        self.assertAlmostEqual(workspace.readY(42)[0], 13297.0)
        DeleteWorkspaces(['jambalaya'])

    def test_trim_calibration_table(self):
        # create a table with detector id and detector XYZ positions
        table = CreateEmptyTableWorkspace(OutputWorkspace='CalibTable')
        table.addColumn(type='int', name='Detector ID')
        table.addColumn(type='V3D', name='Detector Position')
        table.addRow([0, V3D(0, 1, 2)])  # add two detectors with ID's 0 and 1
        table.addRow([1, V3D(3, 4, 5)])
        y_values = [1, 4]
        # call trim_calibration_table and save to new table
        table_calibrated = trim_calibration_table(table, output_workspace='table_calibrated')
        # assert the Y-coordinate hasn't changed
        assert_allclose(table_calibrated.column(1), y_values, atol=0.0001)
        # call trim_calibration_table and overwrite the table
        table_calibrated = trim_calibration_table(table)
        assert table_calibrated.name() == 'CalibTable'  # table workspace has been overwritten with the calibrated one
        assert_allclose(table_calibrated.column(1), y_values, atol=0.0001)

    def test_peak_y_table(self) -> None:
        # Mock PeakTable with two tubes and three peaks. Simple, integer values
        def peak_pixels_table(table_name, peak_count, tube_names=None, pixel_positions=None):
            table = CreateEmptyTableWorkspace(OutputWorkspace=table_name)
            table.addColumn(type='str', name='TubeId')
            for i in range(peak_count):
                table.addColumn(type='float', name='Peak%d' % (i + 1))
            if tube_names is not None and pixel_positions is not None:
                assert len(tube_names) == len(pixel_positions), 'tube_names and pixel_positions have different length'
                for tube_index in range(len(tube_names)):
                    # tube_names is a list of str values; pixel_positions is a list of lists of float values
                    table.addRow([tube_names[tube_index]] + pixel_positions[tube_index])
            return table

        # Create a peak table with only one tube
        peak_table = peak_pixels_table('PeakTable', 3, ['tube1'], [[0, 1, 2]])

        # Mock ParametersTableGroup with one parameter table. Simple parabola
        def parameters_optimized_table(table_name, values=None, errors=None):
            table = CreateEmptyTableWorkspace(OutputWorkspace=table_name)
            for column_type, column_name in [('str', 'Name'), ('float', 'Value'), ('float', 'Error')]:
                table.addColumn(type=column_type, name=column_name)
            if values is not None and errors is not None:
                assert len(values) == 4 and len(errors) == 4  # A0, A1, A2, 'Cost function value'
                for index, row_name in enumerate(['A0', 'A1', 'A2', 'Cost function value']):
                    table.addRow([row_name, values[index], errors[index]])
            return table

        # Create two tables with optimized polynomial coefficients, then group them
        parameters_optimized_table('parameters_table_0', [0, 0, 1, 1.3], [0, 0, 0, 0])  # first tube
        parameters_optimized_table('parameters_table_1', [1, 1, 1, 2.3], [0, 0, 0, 0])  # second tube
        parameters_table = GroupWorkspaces(InputWorkspaces=['parameters_table_0', 'parameters_table_1'],
                                           OutputWorkspace='parameters_table')

        # Check we raise an assertion error since the number of tubes is different than number of tables
        with self.assertRaises(AssertionError) as exception_info:
            calculate_peak_y_table(peak_table, parameters_table, output_workspace='PeakYTable')
        assert 'number of rows in peak_table different than' in str(exception_info.exception)
        # Add another parameter table to ParametersTableGroup, the create peak_vertical_table
        peak_table.addRow(['tube2', 0, 1, 2])
        table = calculate_peak_y_table(peak_table, parameters_table, output_workspace='PeakYTable')
        assert AnalysisDataService.doesExist('PeakYTable')
        assert_allclose(list(table.row(0).values())[1:], [0, 1, 4])
        assert_allclose(list(table.row(1).values())[1:], [1, 3, 7])

    def test_calibrate_tube(self) -> None:
        # Check the validators are doing what they're supposed to do
        with self.assertRaises(AssertionError) as exception_info:
            calibrate_tube(None, 'bank42/sixteenpack/tube8')
        assert 'Cannot process workspace' in str(exception_info.exception)
        with self.assertRaises(AssertionError) as exception_info:
            calibrate_tube(self.workspace, 'tube42')
        assert 'tube42 does not uniquely specify one tube' in str(exception_info.exception)
        with self.assertRaises(AssertionError) as exception_info:
            calibrate_tube(self.workspace, 'bank42/sixteenpack/tube8', fit_domain=20)
        assert 'The fit domain cannot be larger than the distance between consecutive' in str(exception_info.exception)

        table = calibrate_tube(self.workspace, 'bank42/sixteenpack/tube8')
        calibrated_y = table.column(1)
        assert_allclose(calibrated_y, self.calibrated_y, atol=1e-3)

        # Check for existence of all table workspaces
        calibrate_tube(self.cases['124023_bank10'], 'bank10/sixteenpack/tube13')
        DeleteWorkspaces(['CalibTable', 'PeakTable', 'ParametersTable', 'PeakYTable'])

    def test_apply_calibration(self) -> None:
        table = calibrate_tube(self.workspace, 'bank42/sixteenpack/tube8')
        apply_calibration(self.workspace, table)
        assert AnalysisDataService.doesExist('uncalibrated_calibrated')
        DeleteWorkspaces(['uncalibrated_calibrated', str(table)])

    def tearDown(self) -> None:
        to_delete = [w for w in [self.workspace, self.table] if AnalysisDataService.doesExist(w)]
        if len(to_delete) > 0:
            DeleteWorkspaces(to_delete)


if __name__ == "__main__":
    unittest.main()
