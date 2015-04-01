import unittest
import testhelpers
import numpy as np
from mantid.simpleapi import *
from mantid.api import AlgorithmManager, IMDHistoWorkspace, IMDEventWorkspace


class CreateMDTest(unittest.TestCase):

    def test_init(self):
        alg = AlgorithmManager.create("CreateMD")
        alg.initialize()

    def test_must_have_more_than_one_input_workspace(self):
        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "InputWorkspaces", [])

    def test_set_up_madatory(self):

        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.initialize()
        alg.setPropertyValue("OutputWorkspace", "mdworkspace")
        alg.setProperty("InputWorkspaces", ['a', 'b'])
        alg.setProperty("Emode", "Direct")
        alg.setProperty("Alatt", [1,1,1])
        alg.setProperty("Angdeg", [90,90,90])
        alg.setProperty("U", [0,0,1])
        alg.setProperty("V", [1,0,0])
        
    def test_psi_right_size(self):
    
        input_workspace = CreateSampleWorkspace(NumBanks=1, BinWidth=2000)
        AddSampleLog(input_workspace, LogName='Ei', LogText='12.0', LogType='Number')
        
        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.initialize()
        alg.setPropertyValue("OutputWorkspace", "mdworkspace")
        alg.setProperty("InputWorkspaces", ['input_workspace'])
        alg.setProperty("Emode", "Direct")
        alg.setProperty("Alatt", [1,1,1])
        alg.setProperty("Angdeg", [90,90,90])
        alg.setProperty("U", [0,0,1])
        alg.setProperty("V", [1,0,0])
        alg.setProperty("Psi", [0, 0, 0]) # Too large
        alg.setProperty("Gl", [0]) # Right size
        alg.setProperty("Gs", [0]) # Right size
        self.assertRaises(RuntimeError, alg.execute)
        DeleteWorkspace(input_workspace)
        
    def test_gl_right_size(self):
    
        input_workspace = CreateSampleWorkspace(NumBanks=1, BinWidth=2000)
        AddSampleLog(input_workspace, LogName='Ei', LogText='12.0', LogType='Number')
        
        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.initialize()
        alg.setPropertyValue("OutputWorkspace", "mdworkspace")
        alg.setProperty("InputWorkspaces", ['input_workspace'])
        alg.setProperty("Emode", "Direct")
        alg.setProperty("Alatt", [1,1,1])
        alg.setProperty("Angdeg", [90,90,90])
        alg.setProperty("U", [0,0,1])
        alg.setProperty("V", [1,0,0])
        alg.setProperty("Psi", [0]) # Right size
        alg.setProperty("Gl", [0, 0]) # Too many
        alg.setProperty("Gs", [0]) # Right size
        self.assertRaises(RuntimeError, alg.execute)
        DeleteWorkspace(input_workspace)
        
    def test_gs_right_size(self):
    
        input_workspace = CreateSampleWorkspace(NumBanks=1, BinWidth=2000)
        AddSampleLog(input_workspace, LogName='Ei', LogText='12.0', LogType='Number')
        
        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.initialize()
        alg.setPropertyValue("OutputWorkspace", "mdworkspace")
        alg.setProperty("InputWorkspaces", ['input_workspace'])
        alg.setProperty("Emode", "Direct")
        alg.setProperty("Alatt", [1,1,1])
        alg.setProperty("Angdeg", [90,90,90])
        alg.setProperty("U", [0,0,1])
        alg.setProperty("V", [1,0,0])
        alg.setProperty("Psi", [0]) # Right size
        alg.setProperty("Gl", [0]) # Right size
        alg.setProperty("Gs", [0,0]) # Too large
        self.assertRaises(RuntimeError, alg.execute)
        DeleteWorkspace(input_workspace)
        

    def test_execute_single_workspace(self):
        
        input_workspace = CreateSampleWorkspace(NumBanks=1, BinWidth=2000)
        AddSampleLog(input_workspace, LogName='Ei', LogText='12.0', LogType='Number')

        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.initialize()
        alg.setPropertyValue("OutputWorkspace", "mdworkspace")
        alg.setProperty("InputWorkspaces", ['input_workspace'])
        alg.setProperty("Alatt", [1,1,1])
        alg.setProperty("Angdeg", [90,90,90])
        alg.setProperty("U", [0,0,1])
        alg.setProperty("V", [1,0,0])
        alg.execute()
        out_ws = AnalysisDataService.retrieve("mdworkspace")

        self.assertTrue(isinstance(out_ws, IMDEventWorkspace), "Expected an MDEventWorkspace back")
        DeleteWorkspace(input_workspace)















                

if __name__ == '__main__':
    unittest.main()