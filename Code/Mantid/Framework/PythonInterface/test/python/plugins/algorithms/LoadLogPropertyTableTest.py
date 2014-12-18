import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class LoadLogPropertyTableTest(unittest.TestCase):
    def test_LoadValidFilesComments(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test1"

        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = "MUSR00015189.nxs",
                LastFile = "MUSR00015193.nxs", LogNames="comment", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(5, tablews.rowCount())
        self.assertEqual(2, tablews.columnCount())

        self.assertEqual("18.95MHz 100W", tablews.cell(0,1))
        self.assertEqual(15189, tablews.cell(0,0))
        self.assertEqual(15193, tablews.cell(4,0))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)

        return

    def test_LoadPartiallyValidFilesMultipleLogValues(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test2"

        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = "emu00006473.nxs",
                LastFile = "emu00006475.nxs", LogNames="Temp_Sample,dur", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(2, tablews.rowCount())
        self.assertEqual(3, tablews.columnCount())

        self.assertEqual(6473, tablews.cell(0,0))
        self.assertAlmostEqual(200.078, tablews.cell(0,1),2)
        self.assertEqual("8697", tablews.cell(0,2))
        self.assertEqual(6475, tablews.cell(1,0))
        self.assertAlmostEqual(283.523, tablews.cell(1,1),2)
        self.assertEqual("5647", tablews.cell(1,2))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)

        return

    def test_LoadValidFilesBlankLog(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test3"

        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = "MUSR00015189.nxs",
                LastFile = "MUSR00015193.nxs", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(5, tablews.rowCount())
        self.assertEqual(1, tablews.columnCount())

        self.assertEqual(15189, tablews.cell(0,0))
        self.assertEqual(15193, tablews.cell(4,0))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)

        return

    def test_LoadInValidValues(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test4"

        #invalid log name
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = "emu00006473.nxs",
                LastFile = "emu00006475.nxs", LogNames="WrongTemp", OutputWorkspace = outputWorskapceName)
        self.assertFalse(alg_test.isExecuted())

        #invalid first file
        ExecptionThrownOnBadFileParameter = False
        try:
            alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = "emu0000000.nxs",
                LastFile = "emu00006475.nxs", LogNames="Temp_Sample", OutputWorkspace = outputWorskapceName)
            self.assertFalse(alg_test.isExecuted())
        except:
            ExecptionThrownOnBadFileParameter = True
        self.assertEqual(True,ExecptionThrownOnBadFileParameter)

        #invalid last file
        ExecptionThrownOnBadFileParameter = False
        try:
            alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = "emu00006473.nxs",
                LastFile = "emu9999999.nxs", LogNames="Temp_Sample", OutputWorkspace = outputWorskapceName)
            self.assertFalse(alg_test.isExecuted())
        except:
            ExecptionThrownOnBadFileParameter = True
        self.assertEqual(True,ExecptionThrownOnBadFileParameter)

        return


if __name__ == '__main__':
    unittest.main()
