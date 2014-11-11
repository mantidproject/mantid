import unittest
import testhelpers

from mantid.simpleapi import CutMD, CreateMDWorkspace, SetSpecialCoordinates, CompareMDWorkspaces, FakeMDEventData


class CutMDTest(unittest.TestCase):
    

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        # Create a workspace
        data_ws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Add some data to the workspace
        FakeMDEventData(InputWorkspace=data_ws, PeakParams=[10000,0,0,0,1])
        self.__in_md  = data_ws
        
    def __del__(self):
        DeleteWorkspace(self.__in_md )

    def test_exec_throws_if_not_a_hkl_workspace(self):
        test_md = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Explicitly set the coordinate system to lab Q.
        SetSpecialCoordinates(InputWorkspace=test_md, SpecialCoordinates='Q (lab frame)')
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws")
        
    def test_slice_to_original(self):
        
        SetSpecialCoordinates(InputWorkspace=self.__in_md, SpecialCoordinates='HKL')
        out_md = CutMD(self.__in_md)
        equals, result = CompareMDWorkspaces(Workspace1=self.__in_md, Workspace2=out_md)
        self.assertTrue(equals, "Input and output workspaces should be identical" )
        
    def test_run_it(self):
        pass

if __name__ == '__main__':
    unittest.main()