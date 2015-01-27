import unittest,os
import mantid
import numpy


class SuggestTibCNCSTest(unittest.TestCase):
    def test_simple(self):
        result=mantid.simpleapi.SuggestTibCNCS(3.)
        self.assertAlmostEqual(result[0]*0.1,4491.5,0)
        self.assertAlmostEqual(result[1]*0.1,4731.5,0)
        result=mantid.simpleapi.SuggestTibCNCS(1.)
        self.assertAlmostEqual(result[0]*0.1,9562.1,0)
        self.assertAlmostEqual(result[1]*0.1,9902.1,0)
        result=mantid.simpleapi.SuggestTibCNCS(6.)
        self.assertAlmostEqual(result[0]*0.1,2983.3,0)
        self.assertAlmostEqual(result[1]*0.1,3323.3,0)

    def test_someresult(self):
        for en in numpy.arange(1.,30.,0.1):
            result=mantid.simpleapi.SuggestTibCNCS(en)
            self.assertGreater(result[1]-result[0],1000.)

if __name__=="__main__":
    unittest.main()
