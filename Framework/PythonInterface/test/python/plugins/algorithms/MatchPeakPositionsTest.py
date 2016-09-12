from __future__ import (absolute_import, division, print_function)

import unittest
import mantid
from mantid.simpleapi import ILLIN16BCalibration
from testhelpers.tempfile_wrapper import TemporaryFileHelper


class MatchPeakPositionsTest(unittest.TestCase):

    def setUp(self):

        func0 = "name=LinearBackground, A0=0.3;name=Gaussian, PeakCentre=6, Height=10, Sigma=0.3"
        func1 = "name=LinearBackground, A0=0.3;name=Gaussian, PeakCentre=8, Height=10, Sigma=0.3"
        func2 = "name=LinearBackground, A0=0.3;name=Gaussian, PeakCentre=4, Height=10, Sigma=0.001"

        self._ws_shift = CreateSampleWorkspace(Function='User Defined',
                                         WorkspaceType='Histogram',
                                         UserDefinedFunction=func0,
                                         NumBanks=1, BankPixelWidth=1,
                                         XUnit='DeltaE', XMin=0, XMax=10, BinWidth=0.1)
        spectrum_1 = CreateSampleWorkspace(Function='User Defined',
                                           WorkspaceType='Histogram',
                                           UserDefinedFunction=func1,
                                           NumBanks=1, BankPixelWidth=1,
                                           XUnit='DeltaE', XMin=0, XMax=10, BinWidth=0.1)
        spectrum_2 = CreateSampleWorkspace(Function='User Defined',
                                           WorkspaceType='Histogram',
                                           UserDefinedFunction=func2,
                                           NumBanks=1, BankPixelWidth=1,
                                           XUnit='DeltaE', XMin=0, XMax=10, BinWidth=0.1)
        spectrum_3 = CreateSampleWorkspace(Function='Flat background',
                                           WorkspaceType='Histogram',
                                           NumBanks=1, BankPixelWidth=1,
                                           XUnit='DeltaE', XMin=0, XMax=10, BinWidth=0.1)

        self._ws_shift = AppendSpectra(InputWorkspace1=self._ws_shift, InputWorkspace2=spectrum_1)
        self._ws_shift = AppendSpectra(InputWorkspace1=self._ws_shift, InputWorkspace2=spectrum_2)
        self._ws_shift = AppendSpectra(InputWorkspace1=self._ws_shift, InputWorkspace2=spectrum_3)

    def tearDown(self):
        DeleteWorkspace(self._ws_shift)

    def test(self):
        __fit_table = FindEPP(ws_shift)

        bin0 = ws_shift.binIndexOf(__fit_table.row(0)["PeakCentre"])
        bin1 = int(ws_shift.blocksize() / 2)
        bin2 = np.argmax(ws_shift.readY(2))
        bin3 = int(ws_shift.blocksize() / 2)

        expected_peak_position = np.array([bin0, bin1, bin2, bin3])
        print(expected_peak_position)

        expected_peak_value = [ws_shift.readY(0)[bin0], ws_shift.readY(1)[bin1], ws_shift.readY(2)[bin2],
                               ws_shift.readY(3)[bin3]]
        print(expected_peak_value)

        if ws_shift.getHistory() is not None:
            print(ws_shift.getHistory())


            # import IndirectILLReduction
            # import IndirectILLReduction

            # print(IndirectILLReduction)
            # print(dir(IndirectILLReduction))
            # for i in range(ws_shift.getNumberHistograms()):
            #    peak_position = IndirectILLReduction._get_peak_position(ws_shift, i)


if __name__=="__main__":
    unittest.main()
