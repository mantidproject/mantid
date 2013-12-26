import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *
from string import *

class GetEiT0atSNSTest(unittest.TestCase):
            
    def testGETS(self):
        w=Load('ADARAMonitors.nxs')
        res=GetEiT0atSNS(w)
        self.assertAlmostEqual(res[0],20.118,delta=0.01)
        self.assertAlmostEqual(res[1],36.056,delta=0.01)
        try:
            res=GetEiT0atSNS(w,0.1)
        except Exception as e:
            s="Could not get Ei, and this is not a white beam run\nNo peak found for the monitor1"
            self.assertEquals(find(e.message,s),0)
        DeleteWorkspace(w)           
   
if __name__ == '__main__':
    unittest.main()
