#pylint: disable=too-many-public-methods,invalid-name

import unittest
from mantid.simpleapi import *
from mantid.api import *

import numpy as np


def _generate_calibration_workspace(instrument, ws_name='__fake_calib'):
    inst = CreateSimulationWorkspace(Instrument=instrument,
                                     BinParams='0,1,2')
    n_hist = inst.getNumberHistograms()

    fake_calib = CreateWorkspace(OutputWorkspace=ws_name,
                                 DataX=np.ones(1),
                                 DataY=np.ones(n_hist),
                                 NSpec=n_hist,
                                 ParentWorkspace=inst)

    DeleteWorkspace(inst)

    return fake_calib


class ISISIndirectEnergyTransferTest(unittest.TestCase):

    def test_basic_reduction(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'IRS26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)


    def test_reduction_with_range(self):
        """
        Sanity test to ensure a reduction with a spectra range completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[35, 40])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'IRS26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 6)


    def test_reduction_with_detailed_balance(self):
        """
        Sanity test to ensure a reduction using detailed balance option
        completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         DetailedBalance='300')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'IRS26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)


    def test_reduction_with_map_file(self):
        """
        Sanity test to ensure a reduction using a mapping/grouping file
        completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['OSI97919.raw'],
                                         Instrument='OSIRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[963, 1004],
                                         GroupingMethod='File',
                                         MapFile='osi_002_14Groups.map')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'OSI97919_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 14)


    def test_reduction_with_calibration(self):
        """
        Sanity test to ensure a reduction using a calibration workspace
        completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         CalibrationWorkspace=_generate_calibration_workspace('IRIS'))

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'IRS26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)


    def test_reduction_with_calibration_and_range(self):
        """
        Sanity test to ensure a reduction using a calibration workspace
        completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[35, 40],
                                         CalibrationWorkspace=_generate_calibration_workspace('IRIS'))

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'IRS26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 6)


    def test_instrument_validation_failure(self):
        """
        Tests that an invalid instrument configuration causes the validation to
        fail.
        """

        self.assertRaises(RuntimeError,
                          ISISIndirectEnergyTransfer,
                          OutputWorkspace='__ISISIndirectEnergyTransferTest_ws',
                          InputFiles=['IRS26176.RAW'],
                          Instrument='IRIS',
                          Analyser='graphite',
                          Reflection='006',
                          SpectraRange=[3, 53])


    def test_group_workspace_validation_failure(self):
        """
        Tests that validation fails when Workspace is selected as the
        GroupingMethod but no workspace is provided.
        """

        self.assertRaises(RuntimeError,
                          ISISIndirectEnergyTransfer,
                          OutputWorkspace='__ISISIndirectEnergyTransferTest_ws',
                          InputFiles=['IRS26176.RAW'],
                          Instrument='IRIS',
                          Analyser='graphite',
                          Reflection='002',
                          SpectraRange=[3, 53],
                          GroupingMethod='Workspace')


if __name__ == '__main__':
    unittest.main()
