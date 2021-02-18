# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
# import numpy as np
# from numpy.testing import assert_allclose
# from os import path
#
from mantid import AnalysisDataService
# from mantid import config
# from mantid.kernel import V3D
# from mantid.simpleapi import (CreateEmptyTableWorkspace, DeleteWorkspaces, GroupWorkspaces, LoadEmptyInstrument,
#                               LoadNexusProcessed, mtd)
#
# from corelli.calibration.utils import (apply_calibration, preprocess_banks, bank_numbers, calculate_peak_y_table,
#                                        calibrate_tube, load_banks, trim_calibration_table, wire_positions)
from mantid.simpleapi import DeleteWorkspaces


class TestUtils(unittest.TestCase):

    @classmethod
    def setUpClass(cls) -> None:
        r"""
        Load the tests cases for calibrate_bank, consisting of data for only one bank
        CORELLI_124023_bank10, tube 13 has shadows at pixel numbers quite different from the rest
        """
        # config.appendDataSearchSubDir('CORELLI/calibration')
        # for directory in config.getDataSearchDirs():
        #     if 'UnitTest' in directory:
        #         data_dir = path.join(directory, 'CORELLI', 'calibration')
        #         break
        cls.workspaces_temporary = list()
        # cls.cases = dict()
        # for bank_case in ('124023_bank10',):
        #     workspace = 'CORELLI_' + bank_case
        #     LoadNexusProcessed(Filename=path.join(data_dir, workspace + '.nxs'), OutputWorkspace=workspace)
        #     cls.cases[bank_case] = workspace
        #     cls.workspaces_temporary.append(workspace)
        print(f'setUpClass')

    def setUp(self) -> None:
        # Neutron counts along a tube
        counts = [50, 122, 118, 197, 414, 545, 1026, 1767, 3242, 7132, 16232, 34663, 52842, 59767, 58638, 55250, 47723,
                  47400, 54642, 56830, 57664, 57563, 57398, 57438, 57370, 57386, 57447, 56951, 57258, 56195, 52775,
                  46106, 48802, 54698, 57072, 57876, 57754, 57712, 57800, 57837, 57606, 57398, 57038, 56504, 54408,
                  49406, 44668, 50911, 55364, 55906, 56175, 56612, 56375, 56179, 56265, 56329, 56497, 56131, 55034,
                  52618, 46515, 44466, 51293, 54234, 54961, 55219, 54601, 54359, 54866, 54839, 54474, 54031, 54064,
                  53431, 50619, 44013, 45435, 52264, 54087, 55002, 54589, 54568, 53516, 53285, 52656, 52676, 52420,
                  52375, 51837, 47664, 41643, 45292, 51215, 52851, 53284, 53421, 52999, 52985, 52596, 52799, 52658,
                  51894, 51504, 50154, 44435, 39820, 45640, 49871, 50920, 51190, 51091, 51448, 50937, 50927, 50773,
                  51173, 50819, 50110, 48138, 42389, 39823, 46671, 50037, 50013, 50840, 50400, 50138, 49500, 50616,
                  49890, 49853, 49548, 48731, 45211, 39185, 39270, 45409, 47628, 48348, 48652, 48025, 48518, 48915,
                  48351, 48486, 48391, 47793, 47079, 43590, 37528, 40016, 45604, 47259, 47710, 47252, 47687, 47145,
                  47453, 47409, 47294, 46735, 46170, 45028, 40783, 35924, 40234, 44613, 45660, 45921, 45125, 45544,
                  45482, 45261, 45326, 45547, 44914, 44966, 43711, 39302, 35101, 39600, 44185, 45054, 44538, 44443,
                  44617, 44509, 44589, 44806, 45078, 44265, 44053, 42055, 36702, 35192, 40304, 42712, 42910, 42992,
                  43593, 44157, 43675, 43442, 43348, 43668, 42447, 42193, 39349, 33887, 34026, 38630, 40354, 40798,
                  40866, 40592, 40593, 40645, 40507, 40316, 40256, 40068, 39481, 37928, 33034, 31521, 36298, 38938,
                  39451, 39544, 39479, 39624, 39471, 39300, 39197, 38777, 39288, 39163, 37350, 33507, 31309, 36124,
                  38921, 30633, 14932, 5370, 2069, 955, 545, 329, 216, 157, 82, 49, 54, 25, 10]
        # Calibrated Y-coordinates
        y = [-0.342, -0.338, -0.335, -0.331, -0.328, -0.324, -0.320, -0.317, -0.313, -0.310, -0.306, -0.303, -0.299,
             -0.296, -0.292, -0.289, -0.285, -0.282, -0.278, -0.275, -0.271, -0.268, -0.264, -0.261, -0.257, -0.254,
             -0.250, -0.247, -0.243, -0.240, -0.236, -0.232, -0.229, -0.225, -0.222, -0.218, -0.215, -0.211, -0.208,
             -0.204, -0.201, -0.197, -0.194, -0.190, -0.187, -0.183, -0.180, -0.176, -0.173, -0.169, -0.166, -0.162,
             -0.159, -0.155, -0.152, -0.148, -0.144, -0.141, -0.137, -0.134, -0.130, -0.127, -0.123, -0.120, -0.116,
             -0.113, -0.109, -0.106, -0.102, -0.099, -0.095, -0.092, -0.088, -0.085, -0.081, -0.078, -0.074, -0.071,
             -0.067, -0.064, -0.060, -0.056, -0.053, -0.049, -0.046, -0.042, -0.039, -0.035, -0.032, -0.028, -0.025,
             -0.021, -0.018, -0.014, -0.011, -0.007, -0.004, -0.000,  0.003,  0.007,  0.010,  0.014,  0.017,  0.021,
             0.024,  0.028,  0.032,  0.035,  0.039,  0.042,  0.046,  0.049,  0.053,  0.056,  0.060,  0.063,  0.067,
             0.070,  0.074,  0.077,  0.081,  0.084,  0.088,  0.091,  0.095,  0.098,  0.102,  0.105,  0.109,  0.112,
             0.116,  0.120,  0.123,  0.127,  0.130,  0.134,  0.137,  0.141,  0.144,  0.148,  0.151,  0.155,  0.158,
             0.162,  0.165,  0.169,  0.172,  0.176,  0.179,  0.183,  0.186,  0.190,  0.193,  0.197,  0.200,  0.204,
             0.208,  0.211,  0.215,  0.218,  0.222,  0.225,  0.229,  0.232,  0.236,  0.239,  0.243,  0.246,  0.250,
             0.253,  0.257,  0.260,  0.264,  0.267,  0.271,  0.274,  0.278,  0.281,  0.285,  0.288,  0.292,  0.296,
             0.299,  0.303,  0.306,  0.310,  0.313,  0.317,  0.320,  0.324,  0.327,  0.331,  0.334,  0.338,  0.341,
             0.345,  0.348,  0.352,  0.355,  0.359,  0.362,  0.366,  0.369,  0.373,  0.376,  0.380,  0.384,  0.387,
             0.391,  0.394,  0.398,  0.401,  0.405,  0.408,  0.412,  0.415,  0.419,  0.422,  0.426,  0.429,  0.433,
             0.436,  0.440,  0.443,  0.447,  0.450,  0.454,  0.457,  0.461,  0.464,  0.468,  0.472,  0.475,  0.479,
             0.482,  0.486,  0.489,  0.493,  0.496,  0.500,  0.503,  0.507,  0.510,  0.514,  0.517,  0.521,  0.524,
             0.528,  0.531,  0.535,  0.538,  0.542,  0.545,  0.549,  0.552,  0.556]
        # # Create a workspace with an uncalibrated tube bank42/sixteenpack/tube8
        # workspace = LoadEmptyInstrument(InstrumentName='CORELLI', MakeEventWorkspace=False,
        #                                 OutputWorkspace='uncalibrated')
        # # the workspace index for the first pixel in bank87/sixteenpack/tube12 is 355075
        # for pixel_index in range(256):
        #     workspace.dataY(355075 + pixel_index)[:] = counts[pixel_index]
        # self.workspace = 'uncalibrated'
        # self.table = 'calibTable'
        # self.calibrated_y = y
        assert len(counts) == len(y)

    @classmethod
    def tearDownClass(cls) -> None:
        r"""Delete temporary workspaces"""
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)

    def test_wire_positions(self):
        print(f'Hello World')
        # with self.assertRaises(AssertionError) as exception_info:
        #     wire_positions(units='mm')
        # assert 'units mm must be one of' in str(exception_info.exception)
        # expected = [-0.396, -0.343, -0.290, -0.238, -0.185, -0.132, -0.079, -0.026,
        #             0.026, 0.079, 0.132, 0.185, 0.238, 0.290, 0.343, 0.396]
        # assert_allclose(wire_positions(units='meters'), np.array(expected), atol=0.001)
        # expected = [15.4, 30.4, 45.4, 60.4, 75.4, 90.5, 105.5, 120.5,
        #             135.5, 150.5, 165.5, 180.6, 195.6, 210.6, 225.6, 240.6]
        # assert_allclose(wire_positions(units='pixels'), np.array(expected), atol=0.1)
        # assert 1 == 123, 'It shall fail'

    def tearDown(self) -> None:
        to_delete = [w for w in ['a', 'b'] if AnalysisDataService.doesExist(w)]
        if len(to_delete) > 0:
            DeleteWorkspaces(to_delete)


if __name__ == "__main__":
    unittest.main()
