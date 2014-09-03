import unittest,os
import mantid


class SuggestTibHYSPECTest(unittest.TestCase):
    def test_simple(self):
    	result=mantid.simpleapi.SuggestTibHYSPEC(5.)
    	self.assertAlmostEqual(result[0]*.1,3951.5,0)
    	self.assertAlmostEqual(result[1]*.1,4151.5,0)
    	result=mantid.simpleapi.SuggestTibHYSPEC(40.)
    	self.assertAlmostEqual(result[0]*.1,1189.8,0)
    	self.assertAlmostEqual(result[1]*.1,1389.8,0)

if __name__=="__main__":
    unittest.main()
