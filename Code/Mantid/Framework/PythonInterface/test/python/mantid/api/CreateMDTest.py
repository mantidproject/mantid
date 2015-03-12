import unittest
import testhelpers
import numpy as np
from mantid.simpleapi import *
from mantid.api import AlgorithmManager, IMDHistoWorkspace, IMDEventWorkspace


class CreateMDTest(unittest.TestCase):
    

    def setUp(self):
        pass
        
    def tearDown(self):
        pass
        #DeleteWorkspace(self.__in_md )

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
        alg.setProperty("u", [0,0,1])
        alg.setProperty("v", [1,0,0])

    def test_execute_single_workspace(self):
        input_workspace = CreateSampleWorkspace()
        AddSampleLog(input_workspace, LogName='Ei', LogText='12.0', LogType='Number')

        alg = AlgorithmManager.create("CreateMD")
        alg.setRethrows(True)
        alg.setChild(True)
        alg.initialize()
        alg.setPropertyValue("OutputWorkspace", "mdworkspace")
        alg.setProperty("InputWorkspaces", ['input_workspace'])
        alg.setProperty("Alatt", [1,1,1])
        alg.setProperty("Angdeg", [90,90,90])
        alg.setProperty("u", [0,0,1])
        alg.setProperty("v", [1,0,0])
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, IMDEventWorkspace), "Expected an MDEventWorkspace back")








                

if __name__ == '__main__':
    unittest.main()