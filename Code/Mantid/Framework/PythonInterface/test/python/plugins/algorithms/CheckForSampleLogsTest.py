import unittest,os
import mantid


class CheckForSampleLogsTest(unittest.TestCase):
    def test_simple(self):
    	w=mantid.simpleapi.Load('CNCS_7860_event.nxs')
    	result=mantid.simpleapi.CheckForSampleLogs(w)
    	self.assertEquals(result,'')
    	result=mantid.simpleapi.CheckForSampleLogs(w,'Phase1')
    	self.assertEquals(result,'')
    	result=mantid.simpleapi.CheckForSampleLogs(w,'Phrase1')
    	self.assertNotEquals(result,'')

if __name__=="__main__":
    unittest.main()
