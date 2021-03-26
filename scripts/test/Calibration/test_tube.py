# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# Standard and third-party imports
from os import path
import unittest

# Mantid imports
from mantid import config
from mantid.api import AnalysisDataService, mtd
from mantid.simpleapi import DeleteWorkspaces, LoadNexusProcessed
from corelli.calibration.utils import wire_positions

# Calibration imports
from Calibration.tube import calibrate
from Calibration.tube_calib_fit_params import TubeCalibFitParams


class TestTube(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workspaces_temporary = list()  # workspaces to be deleted at tear-down
        # Load a CORELLI file containing data for bank number 20 (16 tubes)
        config.appendDataSearchSubDir('CORELLI/calibration')
        for directory in config.getDataSearchDirs():
            if 'UnitTest' in directory:
                data_dir = path.join(directory, 'CORELLI', 'calibration')
                break
        wire_positions_pixels = [
            0.03043, 0.04544, 0.06045, 0.07546, 0.09047, 0.10548, 0.12049, 0.13550, 0.15052, 0.16553, 0.18054, 0.19555,
            0.21056, 0.22557
        ]  # in mili-meters, 14 wires
        workspace = 'CORELLI_123455_bank20'
        LoadNexusProcessed(Filename=path.join(data_dir, workspace + '.nxs'), OutputWorkspace=workspace)
        cls.workspaces_temporary.append(workspace)  # delete workspace at tear-down
        wire_positions_pixels = wire_positions(units='pixels')[1:-1]
        wire_count = len(wire_positions_pixels)
        fit_parameters = TubeCalibFitParams(wire_positions_pixels, height=-1000, width=4, margin=7)
        fit_parameters.setAutomatic(True)
        cls.corelli = {
            'workspace': workspace,
            'bank_name': 'bank20',
            'wire_positions': wire_positions(units='meters')[1:-1],
            'peaks_form': [1] * wire_count,  # signals we'll be fitting dips (peaks with negative heights)
            'fit_parameters': fit_parameters,
        }

    @classmethod
    def tearDown(cls) -> None:
        r"""Delete the workspaces associated to the test cases"""
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)

    def test_calibrate(self):
        data = self.corelli
        calibrate(data['workspace'],
                  data['bank_name'],
                  data['wire_positions'],
                  data['peaks_form'],
                  fitPar=data['fit_parameters'],
                  outputPeak=True,
                  parameters_table_group='parameters_table_group')
        # Check the table workspaces containing the polynomial coefficients
        assert AnalysisDataService.doesExist('parameters_table_group')
        workspace = mtd['parameters_table_group']
        for tube_number, name in enumerate(workspace.getNames()):
            assert name == f'parameters_table_group_{tube_number}'
            assert AnalysisDataService.doesExist(name)
        # Check the values of the coefficients for the first and last tube
        for tube_index, expected in [(0, {
                'A0': -0.446804,
                'A1': 0.003513,
                'A2': 0.0
        }), (15, {
                'A0': -0.452107,
                'A1': 0.003528,
                'A2': 0.00
        })]:
            workspace = mtd[f'parameters_table_group_{tube_index}']
            for row in workspace:
                if row['Name'] in expected:
                    self.assertAlmostEqual(expected[row['Name']], row['Value'], delta=1.e-6)
        DeleteWorkspaces(['CalibTable', 'parameters_table_group', 'PeakTable'])


if __name__ == '__main__':
    unittest.main()
