import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *
from string import *

class GetEiT0atSNSTest(unittest.TestCase):

    def testGETS(self):
        w=Load('ADARAMonitors.nxs')
        LoadInstrument(Workspace=w,InstrumentName='SEQUOIA',RewriteSpectraMap='0')
        AddSampleLog(Workspace=w,LogName='vChTrans',LogText='1',LogType='Number Series')
        AddSampleLog(Workspace=w,LogName='EnergyRequest',LogText='20',LogType='Number Series')
        res=GetEiT0atSNS(w)
        self.assertAlmostEqual(res[0],20.09,places=2)
        self.assertAlmostEqual(res[1],30.415,places=2)
        try:
            res=GetEiT0atSNS(w,0.1)
        except Exception as e:
            s="Could not get Ei, and this is not a white beam run\nNo peak found for the monitor with spectra num: 2"
            self.assertEquals(find(e.message,s),0)
        DeleteWorkspace(w)

if __name__ == '__main__':
    unittest.main()
