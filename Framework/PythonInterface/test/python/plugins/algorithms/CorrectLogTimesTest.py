import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *

class CorrectLogTimesTest(unittest.TestCase):

    def testCLTWrongLog(self):
        w=CreateSingleValuedWorkspace(DataValue='1',ErrorValue='1')
        LoadNexusLogs(Workspace=w,Filename='CNCS_7860_event.nxs')

        try:
            CorrectLogTimes(Workspace=w,LogNames="s1")
            self.fail("Should not have got here. Should throw because wrong instrument.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(w)

    def testCLTsingle(self):
        w=CreateSingleValuedWorkspace(DataValue='1',ErrorValue='1')
        LoadNexusLogs(Workspace=w,Filename='CNCS_7860_event.nxs')
        self.assertFalse(w.getRun()['proton_charge'].firstTime()==w.getRun()['Speed4'].firstTime())
        CorrectLogTimes(Workspace=w,LogNames="Speed4")
        self.assertTrue(w.getRun()['proton_charge'].firstTime()==w.getRun()['Speed4'].firstTime())
        self.assertFalse(w.getRun()['proton_charge'].firstTime()==w.getRun()['Speed5'].firstTime())
        DeleteWorkspace(w)

    def testCLTall(self):
        w=CreateSingleValuedWorkspace(DataValue='1',ErrorValue='1')
        LoadNexusLogs(Workspace=w,Filename='CNCS_7860_event.nxs')
        self.assertFalse(w.getRun()['proton_charge'].firstTime()==w.getRun()['Speed4'].firstTime())
        CorrectLogTimes(Workspace=w,LogNames="")
        self.assertTrue(w.getRun()['proton_charge'].firstTime()==w.getRun()['Speed4'].firstTime())
        self.assertTrue(w.getRun()['proton_charge'].firstTime()==w.getRun()['Speed5'].firstTime())
        DeleteWorkspace(w)
if __name__ == '__main__':
    unittest.main()
