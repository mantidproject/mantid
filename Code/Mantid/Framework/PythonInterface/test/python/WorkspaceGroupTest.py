import unittest
from testhelpers import run_algorithm
from mantid import mtd

class WorkspaceGroupTest(unittest.TestCase):
  
    def test_group_interface(self):
        run_algorithm('CreateWorkspace', OutputWorkspace='First',DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        run_algorithm('CreateWorkspace', OutputWorkspace='Second',DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        run_algorithm('GroupWorkspaces',InputWorkspaces='First,Second',OutputWorkspace='grouped')
        grouped = mtd['grouped']

        self.assertEquals(2, grouped.getNumberOfEntries())
        # Matches operator
        self.assertEquals(len(grouped), grouped.getNumberOfEntries())
        # Matches length of name list
        names = grouped.getNames()
        self.assertEquals(len(grouped), len(names))
        expected = ['First', 'Second']
        for i in range(len(names)):
            self.assertEquals(expected[i], names[i])
            
        # Clearing the data should leave the handle unusable
        mtd.clear()
        try:
            grouped.getNames()
            self.fail("WorkspaceGroup handle is still usable after ADS has been cleared, it should be a weak reference and raise an error.")
        except RuntimeError, exc:
            self.assertEquals(str(exc), 'Variable invalidated, data has been deleted.')
