import unittest
import testhelpers

from mantid.simpleapi import CutMD, CreateMDWorkspace, SetSpecialCoordinates, CompareMDWorkspaces


class CutMDTest(unittest.TestCase):

    def test_exec_throws_if_not_a_hkl_workspace(self):
        test_md = CreateMDWorkspace(Dimensions=3, Extents="-10,10,-10,10,-10,10", Names="A,B,C", Units="U,U,U")
        SetSpecialCoordinates(InputWorkspace=test_md, SpecialCoordinates='Q (lab frame)')
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws")
        
    def test_slice_to_original(self):
        in_md = CreateMDWorkspace(Dimensions=3, Extents="-10,10,-10,10,-10,10", Names="A,B,C", Units="U,U,U")
        SetSpecialCoordinates(InputWorkspace=in_md, SpecialCoordinates='HKL')
        out_md = CutMD(in_md)
        comparison = CompareMDWorkspaces(Workspace1=in_md, Workspace2=out_md)
        self.assertTrue(comparison, "Input and output workspaces should be identical" )
        
    def test_run_it(self):
        pass

if __name__ == '__main__':
    unittest.main()