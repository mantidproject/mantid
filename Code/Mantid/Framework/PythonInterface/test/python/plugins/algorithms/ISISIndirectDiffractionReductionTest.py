#pylint: disable=too-many-public-methods,invalid-name

import unittest
from mantid.simpleapi import *
from mantid.api import *


class ISISIndirectDiffractionReductionTest(unittest.TestCase):

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
        self.assertEqual(wks.getNames()[0], 'IRS26176_diffspec_red')

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
        self.assertEqual(wks.getNames()[0], 'IRS26176_diffspec_red')

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
        self.assertEqual(wks.getNames()[0], 'IRS26176_diffspec_red')
        self.assertEqual(wks.getNames()[1], 'IRS26173_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)


    def test_sum_files(self):
        """
        Test summing multiple runs.
        """

        wks = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                               SumFiles=True,
                                               Instrument='IRIS',
                                               Mode='diffspec',
                                               SpectraRange=[105, 112])

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'IRS26176_multi_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

        self.assertTrue('multi_run_numbers' in red_ws.getRun())
        self.assertEqual(red_ws.getRun().get('multi_run_numbers').value, '26176,26173')


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
        self.assertEqual(wks.getNames()[0], 'IRS26176_diffspec_red')

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
        self.assertEqual(wks.getNames()[0], 'IRS26176_diffspec_red')

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
        self.assertEqual(wks.getNames()[0], 'IRS26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)


if __name__ == '__main__':
    unittest.main()
