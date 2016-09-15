from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *
import numpy as np
from mantid.simpleapi import *
from testhelpers import run_algorithm


class MatchPeaksTest(unittest.TestCase):

    _args = {}

    def setUp(self):

        func0 = "name=Gaussian, PeakCentre=3.2, Height=10, Sigma=0.3"
        func1 = "name=Gaussian, PeakCentre=6, Height=10, Sigma=0.3"
        func2 = "name=Gaussian, PeakCentre=4, Height=10000, Sigma=0.01"

        _input_ws_0 = 'spectrum0'  # Gaussian
        _input_ws_1 = 'spectrum1'  # Gaussian outside tolerance interval
        _input_ws_2 = 'spectrum2'  # Gaussian, too narrow peak
        self._input_ws_3 = 'spectrum3'  # Flat background

        self._ws_shift = 'to_be_shifted'

        spectrum_0 = CreateSampleWorkspace(Function='User Defined',
                                               WorkspaceType='Histogram',
                                               UserDefinedFunction=func0,
                                               NumBanks=1, BankPixelWidth=1,
                                               XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099,
                                               OutputWorkspace=_input_ws_0)
        spectrum_1 = CreateSampleWorkspace(Function='User Defined',
                                           WorkspaceType='Histogram',
                                           UserDefinedFunction=func1,
                                           NumBanks=1, BankPixelWidth=1,
                                           XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099,
                                           OutputWorkspace=_input_ws_1)
        spectrum_2 = CreateSampleWorkspace(Function='User Defined',
                                           WorkspaceType='Histogram',
                                           UserDefinedFunction=func2,
                                           NumBanks=1, BankPixelWidth=1,
                                           XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099,
                                           OutputWorkspace=_input_ws_2)
        spectrum_3 = CreateSampleWorkspace(Function='Flat background',
                                           WorkspaceType='Histogram',
                                           NumBanks=1, BankPixelWidth=1,
                                           XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099,
                                           OutputWorkspace=self._input_ws_3)

        AppendSpectra(InputWorkspace1=spectrum_0, InputWorkspace2=spectrum_1, OutputWorkspace=self._ws_shift)
        AppendSpectra(InputWorkspace1=self._ws_shift, InputWorkspace2=spectrum_2, OutputWorkspace=self._ws_shift)
        AppendSpectra(InputWorkspace1=self._ws_shift, InputWorkspace2=spectrum_3, OutputWorkspace=self._ws_shift)

        # Input workspace 2
        self._ws_in_2 = 'in_2'
        func3 = "name=LinearBackground, A0=0.3; name=Gaussian, PeakCentre=4.2, Height=10, Sigma=0.3"
        CreateSampleWorkspace(Function='User Defined',
                              WorkspaceType='Histogram',
                              UserDefinedFunction=func3,
                              NumBanks=4, BankPixelWidth=1,
                              XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099,
                              OutputWorkspace=self._ws_in_2)

        # Input workspaces that are incompatible
        self._in1 = 'wrong_number_of_histograms'
        CreateSampleWorkspace(Function='Flat background',
                              WorkspaceType='Histogram',
                              NumBanks=1, BankPixelWidth=1,
                              XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.1,
                              OutputWorkspace=self._in1)
        self._in2 = 'wrong_number_of_bins'
        CreateSampleWorkspace(Function='Flat background',
                              WorkspaceType='Histogram',
                              NumBanks=4, BankPixelWidth=1,
                              XUnit='DeltaE', XMin=0, XMax=8, BinWidth=0.1,
                              OutputWorkspace=self._in2)


        # mtd[self._ws_shift].blocksize() = 70
        # mid = 35

        # Details:
        # workspace has peak positions at : [32 35(mid) 40 35(mid)]
        # the corresponding Y-values are (rounded)  : [10 0 3.4 1.0]
        #
        # -> test shifting to the right and to the left
        # -> test options to use FindEPP, maximum peak position or no shifting

    def tearDown(self):
        if AnalysisDataService.doesExist('to_be_shifted'):
            DeleteWorkspace(self._ws_shift)
        if AnalysisDataService.doesExist('in_2'):
            DeleteWorkspace(self._ws_in_2 )
        if AnalysisDataService.doesExist('output'):
            DeleteWorkspace(mtd['output'])
        if AnalysisDataService.doesExist('wrong_number_of_histograms'):
            DeleteWorkspace(self._in1)
        if AnalysisDataService.doesExist('wrong_number_of_bins'):
            DeleteWorkspace(self._in2)

    def testValidatorInput(self):
        self._args['OutputWorkspace'] = 'output'
        # Test if incompatible workspaces will fail
        with self.assertRaises(RuntimeError):
            self._args['InputWorkspace'] = self._in1
            run_algorithm('MatchPeaks', **self._args)

            self._args['InputWorkspace'] = self._in2
            run_algorithm('MatchPeaks', **self._args)

        # Test if compatible workspaces will be accepted (size, X-values, E-values)
        self._args['InputWorkspace'] = self._ws_shift
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        self._FindEPPtables_deleted

    def testValidatorInput2(self):
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = 'output'
        # Test if incompatible workspaces will fail
        with self.assertRaises(RuntimeError):
            self._args['InputWorkspace2'] = self._in1
            run_algorithm('MatchPeaks', **self._args)

            self._args['InputWorkspace2'] = self._in2
            run_algorithm('MatchPeaks', **self._args)

        # Test if compatible workspaces will be accepted (size, X-values, E-values)
        self._args['InputWorkspace2'] = self._ws_in_2
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        self._FindEPPtables_deleted

    #def testValidatorSpecialValues(self):

    def testMatchCenter(self):
        # Input workspace should match its center
        # Bin ranges of each spectrum:
        # spectrum 0 : [(32-35), 69] => [0, 67]      right shift
        # spectrum 1 : [0, 70]                       no shift
        # spectrum 2 : [0, 69 - (40-35)] => [5, 70]  left shift
        # spectrum 3 : [0, 70]                       no shift
        # Final bin range is [3, 65]
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = 'output'
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        shifted = AnalysisDataService.retrieve('output')
        fit_table = FindEPP(shifted)
        self.assertEqual(35, shifted.binIndexOf(fit_table.row(0)["PeakCentre"]))
        self.assertEqual(35, np.argmax(shifted.readY(2)))
        self._workspace_properties(shifted)
        self._FindEPPtables_deleted
        DeleteWorkspace(shifted)
        DeleteWorkspace(fit_table)

    def testBinRangeTable(self):
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = 'output'
        self._args['BinRangeTable'] = 'bin_range'
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        bin_range_table = AnalysisDataService.retrieve('bin_range')
        # Size of the table and its column names
        self.assertEqual(1, bin_range_table.rowCount())
        self.assertEqual(2, bin_range_table.columnCount())
        columns = ['MinBin', 'MaxBin']
        self.assertEqual(columns, bin_range_table.getColumnNames())
        # Bin range
        self.assertEqual(5, bin_range_table.row(0)["MinBin"])
        self.assertEqual(66, bin_range_table.row(0)["MaxBin"])
        DeleteWorkspace(bin_range_table)
        self._FindEPPtables_deleted

    def testMasking(self):
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = 'output'
        self._args['MaskBins'] = True
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        masked = AnalysisDataService.retrieve('output')
        for i in range(4):
            for k in range(5):
                self.assertEqual(0.0, masked.readY(i)[k], 'Mask spectrum {} bin {} failed'.format(i, k))
            for k in range(67, 70):
                self.assertEqual(0.0, masked.readY(i)[k], 'Mask spectrum {} bin {} failed'.format(i, k))
        DeleteWorkspace(masked)
        self._FindEPPtables_deleted

    def testNoMasking(self):
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = 'output'
        self._args['MaskBins'] = False # this is the default behaviour
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        not_masked = AnalysisDataService.retrieve('output')
        self.assertTrue(1.0, not_masked.readY(2)[0])
        DeleteWorkspace(not_masked)

    def testMatchInput2(self):
        # Input workspace should match the peak of input workspace 2:
        # has its peaks at bin 42
        # shifts: 32-42 = -10
        #         35-42 = -7
        #         40-42 = -2
        #         35-42 = -7
        # new bin range [0, 60]
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = 'output'
        self._args['InputWorkspace2'] = self._ws_in_2
        self._args['BinRangeTable'] = 'bin_range'
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        shifted = AnalysisDataService.retrieve('output')
        bin_range_table = AnalysisDataService.retrieve('bin_range')
        fit_table = FindEPP(shifted)
        self.assertEqual(42, shifted.binIndexOf(fit_table.row(0)["PeakCentre"]))
        self.assertEqual(42, np.argmax(shifted.readY(2)))
        # Bin range
        self.assertEqual(0, bin_range_table.row(0)["MinBin"])
        self.assertEqual(59, bin_range_table.row(0)["MaxBin"])
        self._workspace_properties(shifted)
        self._FindEPPtables_deleted
        DeleteWorkspace(shifted)
        DeleteWorkspace(fit_table)
        DeleteWorkspace(bin_range_table)

    # An equivalent test can be performed for an input table created using
    # FindEPP(InputWorkspace=self._ws_in_2, OutputWorkspace='input_table')
    # self._args['InputTable'] = 'input_table'

    def testMatchInput2MatchOption(self):
        # match option true:
        #               right shifts
        # spectrum 0:   42 - 35 = 7
        # spectrum 1:   42 - 35 = 7
        # spectrum 2:   42 - 35 = 7
        # spectrum 3:   42 - 35 = 7
        # new bin range [7, 70]
        self._args['InputWorkspace'] = self._ws_shift
        self._args['InputWorkspace2'] = self._ws_in_2
        self._args['MatchInput2ToCenter'] = True
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        shifted = AnalysisDataService.retrieve('output')
        bin_range_table = AnalysisDataService.retrieve('bin_range')
        fit_table = FindEPP(shifted)
        self.assertEqual(32-7, shifted.binIndexOf(fit_table.row(0)["PeakCentre"]))
        self.assertEqual(40-7, np.argmax(shifted.readY(2)))
        # Bin range
        self.assertEqual(7, bin_range_table.row(0)["MinBin"])
        self.assertEqual(70, bin_range_table.row(0)["MaxBin"])
        self._workspace_properties(shifted)
        self._FindEPPtables_deleted
        DeleteWorkspace(shifted)
        DeleteWorkspace(fit_table)
        DeleteWorkspace(bin_range_table)

    def testOverride(self):
        self._args['InputWorkspace'] = self._ws_shift
        self._args['OutputWorkspace'] = self._ws_shift
        alg_test = run_algorithm('MatchPeaks', **self._args)
        self.assertTrue(alg_test.isExecuted())
        shifted = AnalysisDataService.retrieve('to_be_shifted')
        self.assertFalse(np.all(mtd['to_be_shifted'].extractY() - shifted.extractY()))
        self._FindEPPtables_deleted
        DeleteWorkspace(shifted)

    def _FindEPPtables_deleted(self):
        with self.assertRaises(ValueError):
            run_algorithm('DeleteWorkspace', Workspace='EPPfit_Parameters')
            DeleteWorkspace()
        with self.assertRaises(ValueError):
            run_algorithm('DeleteWorkspace', Workspace='EPPfit_NormalisedCovarianceMatrix')
        with self.assertRaises(ValueError):
            run_algorithm('DeleteWorkspace', Workspace='fit_table')

    def _workspace_properties(self, test_ws):
        self.assertTrue(isinstance(test_ws, MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(test_ws.getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(test_ws.getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__=="__main__":
    unittest.main()
