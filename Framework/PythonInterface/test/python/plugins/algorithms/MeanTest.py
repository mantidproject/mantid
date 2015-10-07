import unittest
from mantid.simpleapi import *
from mantid.api import *

class MeanTest(unittest.TestCase):

    def test_throws_if_non_existing_names(self):
        a = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        try:
            c = Mean(Workspaces='a,b') # 'b' does not exist.
            self.fail("Should not have got here. Should throw without both workspace names indexing real workspaces in IDF.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(a)

    def test_throws_if_workspace_axis0_unequal(self):
        a = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        b = CreateWorkspace(DataX=[1,2,3,4],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF')
        try:
            c = Mean(Workspaces='a,b') # 'a' and 'b' are different sizes.
            self.fail("Should not have got here. Should throw as axis0 is unequal in size between a and b.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(a)
            DeleteWorkspace(b)

    def test_throws_if_workspace_axis1_unequal(self):
        a = CreateWorkspace(DataX=[1,2,3,4],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF',NSpec=1)
        b = CreateWorkspace(DataX=[1,2,3,4],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF',NSpec=2)
        try:
            c = Mean(Workspaces='a,b') # 'a' and 'b' are different sizes.
            self.fail("Should not have got here. Should throw as axis1 is unequal in size between a and b.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(a)
            DeleteWorkspace(b)

    def test_mean(self):
        a = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        b = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        c = Mean(Workspaces='a,b')
        d = (a + b) / 2 # Do algorithm work manually for purposes of comparison.
        message = CheckWorkspacesMatch(Workspace1=c, Workspace2=d)
        self.assertEquals("Success!", message)

        # Clean-up
        DeleteWorkspace(a)
        DeleteWorkspace(b)
        DeleteWorkspace(c)
        DeleteWorkspace(d)

if __name__ == '__main__':
    unittest.main()