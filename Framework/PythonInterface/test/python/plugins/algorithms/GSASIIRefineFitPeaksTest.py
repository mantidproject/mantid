from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *

class GSASIIRefineFitPeaksTest(unittest.TestCase):
    """
    Very limited test, as executing this algorithm requires a modified
    version of GSASII. At least it does some check that the algorithm is
    registered succesfully and basic sanity checks.
    """

    def test_wrong_properties(self):
        """
        Handle in/out property issues appropriately.
        """
        ws_name = 'out_ws'
        peak = "name=BackToBackExponential, I=5000,A=1, B=1., X0=10000, S=150"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak,
                                    NumBanks=1, BankPixelWidth=1, XMin=5000, XMax=30000,
                                    BinWidth=5, OutputWorkspace=ws_name)
        instr_filename = 'inexistent_instr_par_file'

        # No InputWorkspace property (required)
        self.assertRaises(RuntimeError,
                          GSASIIRefineFitPeaks,
                          WorkspaceIndex=0, ExpectedPeaks='1.2, 3.1')

        # Wrong WorkspaceIndex value
        self.assertRaises(RuntimeError,
                          GSASIIRefineFitPeaks,
                          InputWorkspace=ws_name,
                          WorkspaceIndex=-3)

        # Wrong property
        self.assertRaises(RuntimeError,
                          GSASIIRefineFitPeaks,
                          InputWorkspace=ws_name, BankPixelFoo=33,
                          WorkspaceIndex=0)

        # missing instrument file property
        self.assertRaises(RuntimeError,
                          GSASIIRefineFitPeaks,
                          InputWorkspace=ws_name,
                          WorkspaceIndex=0)

        # Wrong InstrumentFile property name
        self.assertRaises(ValueError,
                          GSASIIRefineFitPeaks,
                          InputWorkspace=ws_name,
                          InstruMentFile=instr_filename,
                          WorkspaceIndex=0, ExpectedPeaks='a')

        # Missing file for InstrumentFile
        self.assertRaises(ValueError,
                          GSASIIRefineFitPeaks,
                          InputWorkspace=ws_name,
                          InstrumentFile=instr_filename,
                          WorkspaceIndex=0, ExpectedPeaks='a')

    def test_exec_import_fails(self):
        pass


if __name__ == '__main__':
    unittest.main()
