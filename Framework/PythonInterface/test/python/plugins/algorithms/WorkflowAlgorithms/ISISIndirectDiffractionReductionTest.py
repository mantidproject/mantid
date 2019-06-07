# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name

from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import config


class ISISIndirectDiffractionReductionTest(unittest.TestCase):

    def setUp(self):
        self._oldFacility = config['default.facility']
        if self._oldFacility.strip() == '':
            self._oldFacility = 'TEST_LIVE'
        config.setFacility('ISIS')

    def tearDown(self):
        config.setFacility(self._oldFacility)
        AnalysisDataService.clear()

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_rebin_param(self):
        """
        Tests rebinning.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112],
                                               RebinParam='3,0.1,4')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

        self.assertEqual(red_ws.blocksize(), 10)
        data_x = red_ws.dataX(0)
        self.assertAlmostEqual(data_x[0], 3.0)
        self.assertAlmostEqual(data_x[-1], 4.0)

    def test_multi_files(self):
        """
        Test reducing multiple files.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 2)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')
        self.assertEqual(wks.getNames()[1], 'iris26173_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_sum_files(self):
        """
        Test summing multiple runs.
        """
        cs = FileFinder.Instance().getCaseSensitive()

        FileFinder.Instance().setCaseSensitive(False)
        wks = ISISIndirectDiffractionReduction(InputFiles=['26173-26176'],
                                               SumFiles=True,
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])
        FileFinder.Instance().setCaseSensitive(cs)

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26173_multi_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

        self.assertTrue('multi_run_numbers' in red_ws.getRun())
        self.assertEqual(red_ws.getRun().get('multi_run_numbers').value, '26173,26174,26175,26176')

    def test_grouping_individual(self):
        """
        Test setting individual grouping, one spectrum per detector.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112],
                                               GroupingPolicy='Individual')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 8)

    def test_reduction_with_container_completes(self):
        """
        Test to ensure that reduction with container subtraction works.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                               ContainerFiles=['IRS26173.RAW'],
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_reduction_with_container_and_scale_completes(self):
        """
        Test to ensure that reduction with container subtraction works.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                               ContainerFiles=['IRS26173.RAW'],
                                               ContainerScaleFactor=0.1,
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_reduction_with_cal_file_osiris_diffspec(self):
        """
        Test to ensure that cal file is used correctly for osiris diffspec runs with cal files
        """
        wks = ISISIndirectDiffractionReduction(InputFiles=['osi89813.raw'],
                                               Instrument='OSIRIS',
                                               Mode='diffspec',
                                               CalFile='osiris_041_RES10.cal',
                                               SpectraRange=[100, 150])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'osiris89813_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_reduction_with_vanadium_iris(self):
        """
        Test to ensure that reduction with normalisation by vanadium works
        """
        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                               VanadiumFiles=['IRS26173.RAW'],
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)
        self.assertEqual(round(red_ws.readY(0)[1], 7), 0.0215684)
        self.assertEqual(round(red_ws.readY(0)[-1], 7), 0.0022809)

    def test_that_a_reduction_with_a_vanadium_file_containing_zeros_for_osiris_diffspec_produces_a_workspace_with_the_correct_name(self):
        output_group = ISISIndirectDiffractionReduction(InputFiles=['OSI137793.RAW'],
                                                        VanadiumFiles=['OSI137713.RAW'],
                                                        Instrument='OSIRIS',
                                                        Mode='diffspec',
                                                        SpectraRange=[3, 962])

        self.assertTrue(isinstance(output_group, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(output_group), 1)
        self.assertEqual(output_group.getNames()[0], 'osiris137793_diffspec_red')

    def test_that_a_reduction_with_a_vanadium_file_containing_zeros_for_osiris_diffspec_produces_the_correct_output_workspace(self):
        output_group = ISISIndirectDiffractionReduction(InputFiles=['OSI137793.RAW'],
                                                        VanadiumFiles=['OSI137713.RAW'],
                                                        Instrument='OSIRIS',
                                                        Mode='diffspec',
                                                        SpectraRange=[3, 962])

        workspace = output_group[0]

        reference = LoadNexus(Filename='OSIRIS_Diffspec_Diffraction_with_Vanadium.nxs')
        self.assertTrue(CompareWorkspaces(Workspace1=reference, Workspace2=workspace, Tolerance=0.001)[0])

    def test_that_a_reduction_with_a_manual_grouping_for_osiris_diffspec_produces_the_correct_output_workspace(self):
        CreateGroupingWorkspace(FixedGroupCount=2,
                                InstrumentName='OSIRIS',
                                ComponentName='bank',
                                OutputWorkspace='__grouping')
        output_group = ISISIndirectDiffractionReduction(InputFiles=['OSI137793.RAW'],
                                                        Instrument='OSIRIS',
                                                        Mode='diffspec',
                                                        SpectraRange=[3, 962],
                                                        GroupingPolicy='Workspace',
                                                        GroupingWorkspace='__grouping')

        workspace = output_group[0]

        reference = LoadNexus(Filename='OSIRIS_Diffspec_Diffraction_with_Manual_Grouping.nxs')
        self.assertTrue(CompareWorkspaces(Workspace1=reference, Workspace2=workspace, Tolerance=0.001)[0])

    # ------------------------------------------ Vesuvio ----------------------------------------------

    def test_vesuvio_basic_reduction(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['29244'],
                                               InstrumentParFile='IP0005.dat',
                                               Instrument='VESUVIO',
                                               mode='diffspec',
                                               SpectraRange=[3, 198])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'vesuvio29244_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_vesuvio_individual(self):
        """
        Test setting individual grouping, one spectrum per detector.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['29244'],
                                               GroupingPolicy='Individual',
                                               InstrumentParFile='IP0005.dat',
                                               Instrument='VESUVIO',
                                               mode='diffspec',
                                               SpectraRange=[3, 198])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 196)

    # ------------------------------------------Failure cases------------------------------------------

    def test_reduction_with_cal_file_osiris_diffonly_fails(self):
        """
        Test to ensure cal file can not be used in diffonly mode
        """
        self.assertRaises(RuntimeError, ISISIndirectDiffractionReduction, InputFiles=['osi89813.raw'],
                          Instrument='OSIRIS',
                          Mode='diffonly',
                          CalFile='osi_041_RES10.cal',
                          OutputWorkspace='wks',
                          SpectraRange=[100, 150])

    def test_reduction_with_cal_file_iris_fail(self):
        """
        Test to ensure cal file can not be used on a different instrument other than OSIRIS
        """
        self.assertRaises(RuntimeError, ISISIndirectDiffractionReduction, InputFiles=['IRS26176.RAW'],
                          Instrument='IRIS',
                          Mode='diffspec',
                          CalFile='osi_041_RES10.cal',
                          OutputWorkspace='wks',
                          SpectraRange=[105, 112])

    def test_that_reduction_fails_if_provided_a_range_of_run_numbers_which_goes_from_high_to_low(self):
        with self.assertRaises(RuntimeError):
            ISISIndirectDiffractionReduction(InputFiles=['137793-137796', '137813-814'],
                                             Instrument='OSIRIS',
                                             Mode='diffspec',
                                             SpectraRange=[105, 112])

    def test_that_reduction_fails_if_provided_a_range_of_run_numbers_which_is_not_a_range(self):
        with self.assertRaises(RuntimeError):
            ISISIndirectDiffractionReduction(InputFiles=['137793-137796', '137813-137813'],
                                             Instrument='OSIRIS',
                                             Mode='diffspec',
                                             SpectraRange=[105, 112])


if __name__ == '__main__':
    unittest.main()
