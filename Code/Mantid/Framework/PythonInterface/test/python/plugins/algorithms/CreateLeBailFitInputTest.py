import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class CreateLeBailFitInputTest(unittest.TestCase):

    def test_LoadHKLFile(self):
        """ Test to load a .hkl file
        """
        # Set up
        alg_test = run_algorithm("CreateLeBailFitInput",
                ReflectionsFile         = "LB4853b2.hkl",
                FullprofParameterFile   = "2011B_HR60b2.irf",
                Bank                    = 2,
                LatticeConstant         = 4.66,
                InstrumentParameterWorkspace    = "PG3_Bank2_Foo",
                BraggPeakParameterWorkspace     = "LaB6_Peaks"
                )

        # Execute
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        # Profile parameter workspace
        paramws = AnalysisDataService.retrieve("PG3_Bank2_Foo")

        paramname0 = paramws.cell(0, 0)

        if paramname0.lower() == "bank":
            numrowgood = 29
        else:
            numrowgood = 28
        print "Parameter name of first line = ", paramname0

        #self.assertEqual(numrowgood, paramws.rowCount())

        paramnames = []
        for i in xrange(paramws.rowCount()):
            paramname = paramws.cell(i, 0)
            paramnames.append(paramname)
        self.assertEqual(paramnames.count("LatticeConstant"), 1)


        # Bragg peak list
        braggws = AnalysisDataService.retrieve("LaB6_Peaks")
        self.assertEqual(145, braggws.rowCount())

        # 4. Delete the test hkl file
        AnalysisDataService.remove("PG3_Bank2_Foo")
        AnalysisDataService.remove("LaB6_Peaks")

        return

    def test_genHKLList(self):
        """ Test to load a .hkl file
        """
        # Set up
        alg_test = run_algorithm("CreateLeBailFitInput",
                ReflectionsFile         = "",
                MaxHKL                  = "12,12,12",
                FullprofParameterFile   = "2011B_HR60b2.irf",
                Bank                    = 2,
                LatticeConstant         = 4.66,
                GenerateBraggReflections        = True,
                InstrumentParameterWorkspace    = "PG3_Bank2_Foo2",
                BraggPeakParameterWorkspace     = "Arb_Peaks"
                )

        # Execute
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        # Profile parameter workspace
        paramws = AnalysisDataService.retrieve("PG3_Bank2_Foo2")

        paramname0 = paramws.cell(0, 0)

        if paramname0.lower() == "bank":
            numrowgood = 28
        else:
            numrowgood = 27
        #print "Parameter name of first line = ", paramname0

        #self.assertEqual(numrowgood, paramws.rowCount())

        paramnames = []
        for i in xrange(paramws.rowCount()):
            paramname = paramws.cell(i, 0)
            paramnames.append(paramname)
        self.assertEqual(paramnames.count("LatticeConstant"), 1)


        # Bragg peak list
        braggws = AnalysisDataService.retrieve("Arb_Peaks")
        self.assertEqual(braggws.rowCount() > 20, True)

        # 4. Delete the test hkl file
        AnalysisDataService.remove("PG3_Bank2_Foo2")
        AnalysisDataService.remove("Arb_Peaks")

        return

if __name__ == '__main__':
    unittest.main()
