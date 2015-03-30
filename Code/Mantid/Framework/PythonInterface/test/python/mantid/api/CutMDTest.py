import unittest
import testhelpers
import numpy as np
from mantid.simpleapi import *
from mantid.api import IMDHistoWorkspace, IMDEventWorkspace


class CutMDTest(unittest.TestCase):
    

    def setUp(self):
        # Create a workspace
        data_ws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Mark the workspace as being in HKL
        SetSpecialCoordinates(InputWorkspace=data_ws, SpecialCoordinates='HKL')
        # Set the UB
        SetUB(Workspace=data_ws, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        # Add some data to the workspace
        FakeMDEventData(InputWorkspace=data_ws, PeakParams=[10000,0,0,0,1])
        self.__in_md  = data_ws
        
    def tearDown(self):
        DeleteWorkspace(self.__in_md )
        
if __name__ == '__main__':
    unittest.main()
