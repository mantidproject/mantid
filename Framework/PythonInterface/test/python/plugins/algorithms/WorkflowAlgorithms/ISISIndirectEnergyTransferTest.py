#pylint: disable=too-many-public-methods,invalid-name
from __future__ import (absolute_import, division, print_function)

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
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'DeltaE')


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
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 6)


    def test_grouping_all(self):
        """
        Tests setting the grouping policy to all.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         GroupingMethod='All')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 1)


    def test_grouping_individual(self):
        """
        Tests setting the grouping policy to individual.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         GroupingMethod='Individual')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)


    def test_reduction_with_background_subtraction(self):
        """
        Tests running a reduction with a background subtraction.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         BackgroundRange=[70000, 75000])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')


    def test_reduction_with_output_unit(self):
        """
        Tests creating reduction in different X units.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         UnitX='DeltaE_inWavenumber')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'DeltaE_inWavenumber')


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
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

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
        self.assertEqual(wks.getNames()[0], 'osiris97919_graphite002_red')

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
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

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
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 6)


    def test_multi_files(self):
        """
        Test reducing multiple files.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 2)
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')
        self.assertEqual(wks.getNames()[1], 'iris26173_graphite002_red')


    def test_sum_files(self):
        """
        Test summing multiple runs.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                         SumFIles=True,
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_multi_graphite002_red')

        red_ws = wks[0]
        self.assertTrue('multi_run_numbers' in red_ws.getRun())
        self.assertEqual(red_ws.getRun().get('multi_run_numbers').value, '26176,26173')


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


    def test_reduction_with_manual_efixed(self):
        """
        Sanity test to ensure a reduction with a manual Efixed value completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         Efixed=1.9)

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)


    def test_reduction_with_manual_efixed_same_as_default(self):
        """
        Sanity test to ensure that manually setting the default Efixed value
        gives the same results as not providing it.
        """

        ref = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53])

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         Efixed=1.845)

        self.assertTrue(CompareWorkspaces(ref, wks)[0])


    def test_reduction_with_can_scale(self):
        """
        Sanity check tio ensure a reduction with can scale value completes.
        """

        wks = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                         Instrument='IRIS',
                                         Analyser='graphite',
                                         Reflection='002',
                                         SpectraRange=[3, 53],
                                         ScaleFactor=0.5)

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)


if __name__ == '__main__':
    unittest.main()
