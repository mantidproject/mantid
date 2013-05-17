import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class LoadLogPropertyTableTest(unittest.TestCase):
    _autotestDir=None
    def setUp(self):
        self._autotestDir="../../../Test/AutoTestData/"
    def test_LoadValidFilesComments(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test1"
        
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = self._autotestDir + "MUSR00015189.nxs", 
                LastFile = self._autotestDir + "MUSR00015194.nxs", LogNames="comment", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(6, tablews.rowCount())
        self.assertEqual(2, tablews.columnCount())
        
        self.assertEqual("18.95MHz 100W", tablews.cell(0,1))
        self.assertEqual(15189, tablews.cell(0,0))
        self.assertEqual(15194, tablews.cell(5,0))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)
        
        return
        
    def test_LoadPartiallyValidFilesLogValues(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test2"
        
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = self._autotestDir + "emu00006473.nxs", 
                LastFile = self._autotestDir + "emu00006475.nxs", LogNames="Temp_Sample", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(2, tablews.rowCount())
        self.assertEqual(2, tablews.columnCount())
        
        self.assertEqual(6473, tablews.cell(0,0))
        self.assertAlmostEqual(200.078, tablews.cell(0,1),3)
        self.assertEqual(6475, tablews.cell(1,0))
        self.assertAlmostEqual(283.523, tablews.cell(1,1),3)

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)
        
        return
        
    def test_LoadValidFilesBlankLog(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test3"
        
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = self._autotestDir + "MUSR00015189.nxs", 
                LastFile = self._autotestDir + "MUSR00015194.nxs", OutputWorkspace = outputWorskapceName)

        self.assertTrue(alg_test.isExecuted())

        #Verify some values
        tablews = AnalysisDataService.retrieve(outputWorskapceName)
        self.assertEqual(6, tablews.rowCount())
        self.assertEqual(1, tablews.columnCount())
        
        self.assertEqual(15189, tablews.cell(0,0))
        self.assertEqual(15194, tablews.cell(5,0))

        run_algorithm("DeleteWorkspace", Workspace = outputWorskapceName)
        
        return
        
    def test_LoadInValidValues(self):
        outputWorskapceName = "LoadLogPropertyTableTest_Test4"
        
        #invalid log name
        alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = self._autotestDir + "emu00006473.nxs", 
                LastFile = self._autotestDir + "emu00006475.nxs", LogNames="WrongTemp", OutputWorkspace = outputWorskapceName)
        self.assertFalse(alg_test.isExecuted())
        
        #invalid first file
        ExecptionThrownOnBadFileParameter = False
        try:
            alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = self._autotestDir + "emu0000000.nxs", 
                LastFile = self._autotestDir + "emu00006475.nxs", LogNames="Temp_Sample", OutputWorkspace = outputWorskapceName)
            self.assertFalse(alg_test.isExecuted())
        except:
            ExecptionThrownOnBadFileParameter = True
        self.assertEqual(True,ExecptionThrownOnBadFileParameter)
        
        #invalid last file
        ExecptionThrownOnBadFileParameter = False
        try:
            alg_test = run_algorithm("LoadLogPropertyTable", FirstFile = self._autotestDir + "emu00006473.nxs", 
                LastFile = self._autotestDir + "emu9999999.nxs", LogNames="Temp_Sample", OutputWorkspace = outputWorskapceName)
            self.assertFalse(alg_test.isExecuted())
        except:
            ExecptionThrownOnBadFileParameter = True
        self.assertEqual(True,ExecptionThrownOnBadFileParameter)
        
        return


if __name__ == '__main__':
    unittest.main()
