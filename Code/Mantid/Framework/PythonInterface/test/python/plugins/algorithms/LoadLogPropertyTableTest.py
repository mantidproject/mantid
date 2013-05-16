import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class LoadLogPropertyTableTest(unittest.TestCase):

    def test_LoadValidFilesComments(self):
        autoTestDir = "../../../../../Test/AutoTestData/"
        outputWorskapceName = "LoadLogPropertyTableTest_Test1"
        
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = autoTestDir + "MUSR00015189.nxs", 
                LastFile = autoTestDir + "MUSR00015194.nxs", LogNames="comment", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(6, tablews.rowCount())
        self.assertEqual(2, tablews.columnCount())
        
        self.assertEqual("18.95MHz 100W", output.cell(0,1))
        self.assertEqual("15189", output.cell(0,0))
        self.assertEqual("15194", output.cell(5,0))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)
        
        return
        
    def test_LoadPartiallyValidFilesLogValues(self):
        autoTestDir="../../../../../Test/AutoTestData/"
        outputWorskapceName = "LoadLogPropertyTableTest_Test2"
        
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = autoTestDir + "argus0026287.nxs", 
                LastFile = autoTestDir + "argus0026577.nxs", LogNames="temperature_1_log", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(2, tablews.rowCount())
        self.assertEqual(2, tablews.columnCount())
        
        self.assertEqual("26287", output.cell(0,0))
        self.assertEqual("180", output.cell(0,1))
        self.assertEqual("26287", output.cell(1,0))
        self.assertEqual("7.32055", output.cell(1,1))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)
        
        return


if __name__ == '__main__':
    unittest.main()
